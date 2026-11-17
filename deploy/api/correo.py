"""Correos de la API de Voxel World, por Gmail (SMTP), con biblioteca estandar.

La cuenta y su contrasena de aplicacion vienen del entorno, que en el servidor
sale de /srv/voxel-world/api.env (NUNCA del repositorio, que es publico):

    SMTP_USUARIO   la cuenta de Gmail que envia
    SMTP_CLAVE     su contrasena de aplicacion (16 letras, sin espacios)
    NOTIFICAR_A    a quien llegan los avisos, separados por coma (por defecto,
                   SMTP_USUARIO); el primero recibe las respuestas
    SITIO          https://voxelworld.scram.com.mx

Sin SMTP_USUARIO o SMTP_CLAVE no se envia nada y la API sigue funcionando:
los comentarios y los correos se guardan igual.
"""
import os
import queue
import smtplib
import sys
import threading
import time
from email.message import EmailMessage
from email.utils import formataddr, make_msgid

USUARIO = os.environ.get("SMTP_USUARIO", "")
CLAVE = os.environ.get("SMTP_CLAVE", "").replace(" ", "")
SERVIDOR = os.environ.get("SMTP_SERVIDOR", "smtp.gmail.com")
PUERTO = int(os.environ.get("SMTP_PUERTO", "465"))
NOTIFICAR_A = os.environ.get("NOTIFICAR_A", "") or USUARIO
# Las respuestas de testers y clientes van a una sola persona: la primera.
RESPONDER_A = NOTIFICAR_A.split(",")[0].strip()
SITIO = os.environ.get("SITIO", "https://voxelworld.scram.com.mx").rstrip("/")
# Tope de correos automaticos al dia (avisos y bienvenidas): si alguien abusa
# del formulario, el servidor no se convierte en una fuente de spam ni quema
# la cuenta de Gmail (que tiene sus propios limites diarios).
MAX_DIARIO = int(os.environ.get("CORREOS_MAX_DIARIO", "300"))

PIE = ("\n--\nVoxel World · {sitio}\n"
       "Si no quieres recibir más correos de Voxel World, responde con la palabra «baja».\n")

BIENVENIDA = """Hola:

Gracias por descargar la versión de prueba de Voxel World.

Para instalarlo, abre VoxelWorld-Instalador.exe desde tus Descargas. Si Windows
avisa de que el programa es desconocido, pulsa «Más información» y «Ejecutar de
todas formas».

Cuando lo hayas jugado, cuéntanos qué te pareció: qué te gustó, qué falló y qué
le falta. Puedes responder a este correo o dejar tu comentario en
{sitio}/#comentarios

Gracias por ayudarnos a mejorarlo.
"""

INVITACION = """Hola:

Te invitamos a probar Voxel World, un juego de mundo abierto hecho de bloques,
ambientado en México y hecho en su propio motor: biomas, clima, flora y fauna
mexicanas.

La versión de prueba es gratis. Descárgala aquí:
{sitio}/adquirir.html

Cuando lo juegues, nos encantará saber qué te pareció. Responde a este correo o
deja tus comentarios en {sitio}/#comentarios
"""


def configurado():
    return bool(USUARIO and CLAVE)


def _mensaje(para, asunto, cuerpo, responder_a=None):
    m = EmailMessage()
    m["From"] = formataddr(("Voxel World", USUARIO))
    m["To"] = para
    m["Subject"] = asunto
    m["Message-ID"] = make_msgid(domain=USUARIO.split("@")[-1] or None)
    if responder_a:
        m["Reply-To"] = responder_a
    m.set_content(cuerpo)
    return m


def enviar(para, asunto, cuerpo, responder_a=None, con_pie=True):
    """Envia un correo y devuelve True si salio. Sin datos en el log."""
    if not configurado():
        return False
    if con_pie:
        cuerpo += PIE.format(sitio=SITIO)
    try:
        with smtplib.SMTP_SSL(SERVIDOR, PUERTO, timeout=20) as s:
            s.login(USUARIO, CLAVE)
            s.send_message(_mensaje(para, asunto, cuerpo, responder_a))
        return True
    except (smtplib.SMTPException, OSError) as e:
        sys.stderr.write(f"correo: no se pudo enviar ({type(e).__name__})\n")
        return False


# --- Envio en segundo plano, para que la pagina no espere al SMTP ---
_cola = queue.Queue(maxsize=500)
_hoy = {"dia": "", "enviados": 0}
_candado = threading.Lock()


def _trabajador():
    while True:
        para, asunto, cuerpo, responder_a, con_pie = _cola.get()
        enviar(para, asunto, cuerpo, responder_a, con_pie)
        _cola.task_done()


def encolar(para, asunto, cuerpo, responder_a=None, con_pie=True):
    if not configurado() or not para:
        return
    dia = time.strftime("%Y-%m-%d", time.gmtime())
    with _candado:
        if _hoy["dia"] != dia:
            _hoy.update(dia=dia, enviados=0)
        if _hoy["enviados"] >= MAX_DIARIO:
            return
        _hoy["enviados"] += 1
    try:
        _cola.put_nowait((para, asunto, cuerpo, responder_a, con_pie))
    except queue.Full:
        pass


threading.Thread(target=_trabajador, daemon=True).start()


# --- Los correos concretos ---

def avisar_comentario(nombre, correo, comentario):
    cuerpo = (f"Nuevo comentario en la web de Voxel World.\n\n"
              f"Nombre: {nombre or 'sin nombre'}\n"
              f"Correo: {correo or 'no lo dejó'}\n\n{comentario}\n")
    if correo:
        cuerpo += "\nResponde a este correo para contestarle directamente.\n"
    encolar(NOTIFICAR_A, "[Voxel World] Nuevo comentario", cuerpo,
            responder_a=correo or None, con_pie=False)


def avisar_tester(correo, bienvenida):
    encolar(NOTIFICAR_A, "[Voxel World] Nuevo tester",
            f"Un tester dejó su correo para descargar el juego: {correo}\n",
            responder_a=correo, con_pie=False)
    if bienvenida:
        encolar(correo, "Gracias por probar Voxel World",
                BIENVENIDA.format(sitio=SITIO), responder_a=RESPONDER_A)


def avisar_registro(tipo, nombre, cuenta):
    juego = "Ludora" if tipo == "ludora" else "Voxel World"
    encolar(NOTIFICAR_A, f"[{juego}] Nueva cuenta: {cuenta}",
            f"Alguien creó una cuenta de {juego} en la web.\n\n"
            f"Nombre: {nombre}\nCuenta: {cuenta}\n\n"
            "La contraseña no llega al servidor: se queda en el equipo de la persona.\n",
            con_pie=False)


def invitacion(correo):
    """Para el comando invitar: sincrono, para saber si salio cada una."""
    return enviar(correo, "Te invitamos a probar Voxel World",
                  INVITACION.format(sitio=SITIO), responder_a=RESPONDER_A)
