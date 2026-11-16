"""API minima de la pagina de Voxel World: comentarios y correos de testers.

Solo biblioteca estandar y SQLite. Vive en la red interna del proyecto; nginx
(el contenedor web) le pasa /api/. No es alcanzable directo desde Traefik.

    python servidor.py                         arranca en :8000
    python servidor.py exportar                vuelca las tablas en CSV a la salida
    python servidor.py invitar a@x.com b@y.com invita a probar el juego (con "-"
                                               lee los correos de la entrada, uno
                                               por linea; --repetir reenvia)
    python servidor.py baja a@x.com            no le vuelve a escribir
    python servidor.py probar-correo           manda un correo de prueba a NOTIFICAR_A

Rutas:
    GET  /health
    POST /api/comentarios   {"nombre"?, "correo"?, "comentario", "sitio"}
    POST /api/descargas     {"correo", "acepta": true, "sitio"}
"sitio" es un campo trampa: el formulario lo deja vacio y los bots lo llenan.
"""
import csv
import json
import os
import re
import sqlite3
import sys
import threading
import time
from collections import defaultdict, deque
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

import correo as correos

DB = os.environ.get("VOXEL_DB", "/datos/voxelworld.db")
MAX_CUERPO = 8 * 1024
CORREO = re.compile(r"^[^@\s]{1,64}@[^@\s]+\.[^@\s]{2,}$")
# Por IP: 10 envios cada 10 minutos. La IP solo se usa aqui, no se guarda.
LIMITE, VENTANA = 10, 600
_envios = defaultdict(deque)
_candado = threading.Lock()


def conectar():
    con = sqlite3.connect(DB, timeout=5)
    con.execute("""CREATE TABLE IF NOT EXISTS comentarios (
        id INTEGER PRIMARY KEY, fecha TEXT NOT NULL,
        nombre TEXT, correo TEXT, comentario TEXT NOT NULL)""")
    con.execute("""CREATE TABLE IF NOT EXISTS descargas (
        id INTEGER PRIMARY KEY, fecha TEXT NOT NULL, correo TEXT NOT NULL)""")
    con.execute("""CREATE TABLE IF NOT EXISTS invitaciones (
        id INTEGER PRIMARY KEY, fecha TEXT NOT NULL, correo TEXT NOT NULL)""")
    con.execute("""CREATE TABLE IF NOT EXISTS bajas (
        correo TEXT PRIMARY KEY, fecha TEXT NOT NULL)""")
    return con


def ahora():
    return time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())


def de_baja(con, correo):
    return con.execute("SELECT 1 FROM bajas WHERE correo = ? COLLATE NOCASE",
                       (correo,)).fetchone() is not None


def permitido(ip):
    ahora = time.monotonic()
    with _candado:
        q = _envios[ip]
        while q and ahora - q[0] > VENTANA:
            q.popleft()
        if len(q) >= LIMITE:
            return False
        q.append(ahora)
        return True


def texto(datos, clave, maximo):
    valor = datos.get(clave)
    if valor is None:
        return ""
    if not isinstance(valor, str):
        raise ValueError(clave)
    valor = valor.strip()
    if len(valor) > maximo:
        raise ValueError(clave)
    return valor


def correo_valido(valor):
    return len(valor) <= 254 and bool(CORREO.match(valor))


class Manejador(BaseHTTPRequestHandler):
    server_version = "voxel-world-api"
    sys_version = ""

    def responder(self, codigo, cuerpo):
        datos = json.dumps(cuerpo).encode()
        self.send_response(codigo)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(datos)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(datos)

    def log_message(self, formato, *args):
        # Sin IPs ni contenido en el log: solo ruta y codigo.
        sys.stderr.write("%s %s\n" % (self.command, args[1] if len(args) > 1 else ""))

    def do_GET(self):
        if self.path == "/health":
            try:
                conectar().close()
                return self.responder(200, {"status": "ok"})
            except sqlite3.Error:
                return self.responder(503, {"status": "error"})
        self.responder(404, {"error": "no existe"})

    def do_POST(self):
        if self.path not in ("/api/comentarios", "/api/descargas"):
            return self.responder(404, {"error": "no existe"})
        ip = (self.headers.get("CF-Connecting-IP")
              or self.headers.get("X-Forwarded-For", "").split(",")[0].strip()
              or self.client_address[0])
        if not permitido(ip):
            return self.responder(429, {"error": "Demasiados envios. Intenta en unos minutos."})
        try:
            largo = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            largo = -1
        if not 0 < largo <= MAX_CUERPO:
            return self.responder(413, {"error": "Mensaje demasiado largo."})
        try:
            datos = json.loads(self.rfile.read(largo))
            if not isinstance(datos, dict):
                raise ValueError("json")
            if texto(datos, "sitio", 200):
                return self.responder(201, {"ok": True})   # bot: se ignora en silencio
            fecha = ahora()
            if self.path == "/api/comentarios":
                nombre = texto(datos, "nombre", 80)
                correo = texto(datos, "correo", 254)
                comentario = texto(datos, "comentario", 2000)
                if not comentario:
                    return self.responder(400, {"error": "Escribe un comentario."})
                if correo and not correo_valido(correo):
                    return self.responder(400, {"error": "El correo no es valido."})
                fila = ("INSERT INTO comentarios (fecha, nombre, correo, comentario) VALUES (?,?,?,?)",
                        (fecha, nombre, correo, comentario))
                aviso = lambda con: correos.avisar_comentario(nombre, correo, comentario)
            else:
                correo = texto(datos, "correo", 254)
                if not correo_valido(correo):
                    return self.responder(400, {"error": "El correo no es valido."})
                if datos.get("acepta") is not True:
                    return self.responder(400, {"error": "Falta aceptar el aviso de privacidad."})
                fila = ("INSERT INTO descargas (fecha, correo) VALUES (?,?)", (fecha, correo))

                def aviso(con):
                    # Una bienvenida por direccion cada 24 h, y nunca a quien
                    # pidio la baja: el formulario no sirve para bombardear a
                    # nadie con correos nuestros.
                    hace_un_dia = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(time.time() - 86400))
                    previas = con.execute(
                        "SELECT COUNT(*) FROM descargas WHERE correo = ? COLLATE NOCASE AND fecha > ?",
                        (correo, hace_un_dia)).fetchone()[0]
                    correos.avisar_tester(correo, bienvenida=previas <= 1 and not de_baja(con, correo))
        except (ValueError, json.JSONDecodeError):
            return self.responder(400, {"error": "Datos no validos."})
        con = conectar()
        try:
            with con:
                con.execute(*fila)
            aviso(con)
        finally:
            con.close()
        self.responder(201, {"ok": True})


def exportar():
    con = conectar()
    salida = csv.writer(sys.stdout)
    for tabla in ("comentarios", "descargas", "invitaciones", "bajas"):
        cur = con.execute(f"SELECT * FROM {tabla} ORDER BY id")
        salida.writerow([f"# {tabla}"])
        salida.writerow([c[0] for c in cur.description])
        salida.writerows(cur)
    con.close()


def invitar(argumentos):
    if not correos.configurado():
        sys.exit("Falta SMTP_USUARIO o SMTP_CLAVE en api.env: no se puede enviar.")
    repetir = "--repetir" in argumentos
    lista = [a for a in argumentos if a != "--repetir"]
    if lista == ["-"]:
        lista = [l.strip() for l in sys.stdin if l.strip()]
    con = conectar()
    try:
        for c in dict.fromkeys(x.strip() for x in lista):   # sin repetidos
            if not correo_valido(c):
                print(f"{c}: no es un correo valido")
            elif de_baja(con, c):
                print(f"{c}: pidio la baja, no se envia")
            elif not repetir and con.execute(
                    "SELECT 1 FROM invitaciones WHERE correo = ? COLLATE NOCASE", (c,)).fetchone():
                print(f"{c}: ya invitado (usa --repetir para reenviar)")
            elif correos.invitacion(c):
                with con:
                    con.execute("INSERT INTO invitaciones (fecha, correo) VALUES (?,?)", (ahora(), c))
                print(f"{c}: enviada")
                time.sleep(1)   # sin rafagas: Gmail las penaliza
            else:
                print(f"{c}: NO se pudo enviar")
    finally:
        con.close()


def baja(argumentos):
    con = conectar()
    with con:
        for c in argumentos:
            con.execute("INSERT OR IGNORE INTO bajas (correo, fecha) VALUES (?,?)", (c.strip().lower(), ahora()))
            print(f"{c}: de baja")
    con.close()


if __name__ == "__main__":
    orden, resto = (sys.argv[1], sys.argv[2:]) if len(sys.argv) > 1 else ("", [])
    if orden == "exportar":
        exportar()
    elif orden == "invitar":
        invitar(resto)
    elif orden == "baja":
        baja(resto)
    elif orden == "probar-correo":
        ok = correos.enviar(correos.NOTIFICAR_A, "[Voxel World] Prueba de correo",
                            "Si lees esto, la API de Voxel World ya envía correos por Gmail.\n",
                            con_pie=False)
        sys.exit(0 if ok else "No se pudo enviar: revisa SMTP_USUARIO y SMTP_CLAVE en api.env.")
    elif orden:
        sys.exit(__doc__)
    else:
        conectar().close()
        ThreadingHTTPServer(("0.0.0.0", 8000), Manejador).serve_forever()
