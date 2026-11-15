"""API minima de la pagina de Voxel World: comentarios y correos de testers.

Solo biblioteca estandar y SQLite. Vive en la red interna del proyecto; nginx
(el contenedor web) le pasa /api/. No es alcanzable directo desde Traefik.

    python servidor.py              arranca en :8000
    python servidor.py exportar     vuelca las dos tablas en CSV a la salida

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
    return con


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
            fecha = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
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
            else:
                correo = texto(datos, "correo", 254)
                if not correo_valido(correo):
                    return self.responder(400, {"error": "El correo no es valido."})
                if datos.get("acepta") is not True:
                    return self.responder(400, {"error": "Falta aceptar el aviso de privacidad."})
                fila = ("INSERT INTO descargas (fecha, correo) VALUES (?,?)", (fecha, correo))
        except (ValueError, json.JSONDecodeError):
            return self.responder(400, {"error": "Datos no validos."})
        con = conectar()
        try:
            with con:
                con.execute(*fila)
        finally:
            con.close()
        self.responder(201, {"ok": True})


def exportar():
    con = conectar()
    salida = csv.writer(sys.stdout)
    for tabla in ("comentarios", "descargas"):
        cur = con.execute(f"SELECT * FROM {tabla} ORDER BY id")
        salida.writerow([f"# {tabla}"])
        salida.writerow([c[0] for c in cur.description])
        salida.writerows(cur)
    con.close()


if __name__ == "__main__":
    if sys.argv[1:] == ["exportar"]:
        exportar()
    else:
        conectar().close()
        ThreadingHTTPServer(("0.0.0.0", 8000), Manejador).serve_forever()
