// LA CUENTA DEL JUGADOR (src/Cuenta.h). PEDIDO: "anade un sistema de cuentas:
// antes de que inicies el juego te pedira [...] tu nombre, cuenta, contrasena,
// autenticador y si todo esta en orden te deja jugar".
//
// La criptografia se comprueba contra los vectores de sus normas: si una
// funcion de estas falla, falla en silencio (deja entrar a quien no debe o no
// deja entrar a nadie), asi que no se da por buena "porque compila".
#include "doctest/doctest.h"

#include "Cuenta.h"

#include <fstream>
#include <iterator>
#include <string>

using namespace Cuenta;

namespace {
Bytes de(const std::string& s) { return Bytes(s.begin(), s.end()); }
std::string hexDe(const Bytes& b) { return aHex(b); }
const Bytes SAL = deHex("000102030405060708090a0b0c0d0e0f");
const Bytes SAL2 = deHex("f0e0d0c0b0a090807060504030201000");
const Bytes SECRETO = de("12345678901234567890");          // el de las RFC 4226 y 6238
constexpr int POCAS = 1000;                                 // vueltas, para que las pruebas vuelen
std::string leerDelRepo(const char* relativo) {
    std::string r;
#ifdef RAIZ_PROYECTO
    r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
#endif
    std::ifstream f(r + relativo, std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
} // namespace

TEST_CASE("Cuenta: SHA-256 y SHA-1 dan los valores de la norma") {
    CHECK(hexDe(sha256((const uint8_t*)"abc", 3)) ==
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(hexDe(sha256(nullptr, 0)) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    const std::string largo = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";    // cruza de bloque
    CHECK(hexDe(sha256((const uint8_t*)largo.data(), largo.size())) ==
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    CHECK(hexDe(sha1((const uint8_t*)"abc", 3)) == "a9993e364706816aba3e25717850c26c9cd0d89d");
    CHECK(hexDe(sha1((const uint8_t*)largo.data(), largo.size())) == "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
}

TEST_CASE("Cuenta: HMAC y PBKDF2 dan los valores de la norma") {
    // RFC 4231, caso 2.
    const std::string d = "what do ya want for nothing?";
    CHECK(hexDe(hmacSha256(de("Jefe"), (const uint8_t*)d.data(), d.size())) ==
          "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
    // RFC 2202, caso 2 (SHA-1).
    CHECK(hexDe(hmacSha1(de("Jefe"), (const uint8_t*)d.data(), d.size())) ==
          "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79");
    // PBKDF2-HMAC-SHA256, "password" / "salt".
    CHECK(hexDe(pbkdf2("password", de("salt"), 1, 32)) ==
          "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b");
    CHECK(hexDe(pbkdf2("password", de("salt"), 2, 32)) ==
          "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43");
    CHECK(hexDe(pbkdf2("password", de("salt"), 4096, 32)) ==
          "c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a");
    // Mas de un bloque de salida.
    CHECK(hexDe(pbkdf2("passwordPASSWORDpassword", de("saltSALTsaltSALTsaltSALTsaltSALTsalt"), 4096, 40)) ==
          "348c89dbcbd32b2f32d814b8116e84cf2b17347ebc1800181c4e2a1fb8dd53e1c635518c7dac47e9");
}

TEST_CASE("Cuenta: el autenticador da los codigos de las RFC 4226 y 6238") {
    static const int HOTP[10] = { 755224, 287082, 359152, 969429, 338314, 254676, 287922, 162583, 399871, 520489 };
    for (int c = 0; c < 10; ++c) CHECK(hotp(SECRETO, (uint64_t)c) == HOTP[c]);
    // Los de la RFC 6238 son de 8 digitos; aqui, sus 6 ultimos.
    CHECK(totp(SECRETO, 59) == 287082);
    CHECK(totp(SECRETO, 1111111109LL) == 81804);
    CHECK(totp(SECRETO, 1111111111LL) == 50471);
    CHECK(totp(SECRETO, 1234567890LL) == 5924);
    CHECK(totp(SECRETO, 2000000000LL) == 279037);
    // Dura 30 segundos, y se aceptan el paso de antes y el de despues (los relojes no van iguales).
    const long long t = 1234567890LL;
    const int ahora = totp(SECRETO, t);
    CHECK(totpVale(SECRETO, ahora, t));
    CHECK(totpVale(SECRETO, ahora, t + 30));
    CHECK(totpVale(SECRETO, ahora, t - 30));
    CHECK_FALSE(totpVale(SECRETO, ahora, t + 90));
    CHECK_FALSE(totpVale(SECRETO, ahora, t - 90));
    CHECK_FALSE(totpVale(SECRETO, (ahora + 1) % 1000000, t));
    // Como se escribe el secreto en la aplicacion.
    CHECK(aBase32(de("foobar")) == "MZXW6YTBOI");
    CHECK(aBase32(de("f")) == "MY");
    CHECK(aBase32(SECRETO) == "GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ");
    CHECK(secretoLegible(SECRETO) == "GEZD GNBV GY3T QOJQ GEZD GNBV GY3T QOJQ");
}

TEST_CASE("Cuenta: se entra con la cuenta, la contrasena Y el codigo; con menos, no") {
    const Registro r = crear("Iker", "IkerCortes", "tomate-rojo-9", SAL, SAL2, SECRETO, POCAS);
    REQUIRE(r.valido());
    CHECK(r.cuenta == "ikercortes@voxelworld");
    const long long t = 1800000000LL;
    const int codigo = totp(SECRETO, t);
    CHECK(entrar(r, "ikercortes", "tomate-rojo-9", codigo, t) == Entrada::DENTRO);
    CHECK(entrar(r, "IKERCORTES", "tomate-rojo-9", codigo, t) == Entrada::DENTRO);              // la cuenta no distingue mayusculas
    CHECK(entrar(r, "ikercortes@voxelworld", "tomate-rojo-9", codigo, t) == Entrada::DENTRO);   // con o sin arroba
    // La contrasena si distingue, y un caracter de menos no vale.
    CHECK(entrar(r, "ikercortes", "Tomate-rojo-9", codigo, t) == Entrada::CREDENCIALES);
    CHECK(entrar(r, "ikercortes", "tomate-rojo-", codigo, t) == Entrada::CREDENCIALES);
    CHECK(entrar(r, "ikercortes", "", codigo, t) == Entrada::CREDENCIALES);
    CHECK(entrar(r, "otro", "tomate-rojo-9", codigo, t) == Entrada::CREDENCIALES);
    // El mismo aviso si falla la cuenta o la contrasena: no se dice cual.
    CHECK(std::string(mensaje(Entrada::CREDENCIALES)) == "CUENTA O CONTRASENA INCORRECTAS");
    // Con todo bien pero sin el codigo de ahora, tampoco.
    CHECK(entrar(r, "ikercortes", "tomate-rojo-9", (codigo + 7) % 1000000, t) == Entrada::CODIGO);
    CHECK(entrar(r, "ikercortes", "tomate-rojo-9", codigo, t + 600) == Entrada::CODIGO);        // uno viejo
    // Sin cuenta creada.
    CHECK(entrar(Registro(), "ikercortes", "tomate-rojo-9", codigo, t) == Entrada::SIN_CUENTA);
}

TEST_CASE("Cuenta: en el archivo no esta la contrasena ni el secreto del autenticador") {
    const std::string clave = "tomate-rojo-9";
    const Registro r = crear("Iker Cortes", "iker.cortes", clave, SAL, SAL2, SECRETO, POCAS);
    const std::string texto = aTexto(r);
    CHECK(texto.find(clave) == std::string::npos);
    CHECK(texto.find(aHex(SECRETO)) == std::string::npos);
    CHECK(texto.find(aBase32(SECRETO)) == std::string::npos);
    CHECK(r.totp != SECRETO);                                    // va tapado
    CHECK(taparSecreto(r.totp, clave, r.salTotp, r.vueltas) == SECRETO);          // con la contrasena, sale
    CHECK(taparSecreto(r.totp, "otra-clave-1", r.salTotp, r.vueltas) != SECRETO); // sin ella, no
    // Otra sal, otro hash: dos cuentas con la misma contrasena no se parecen.
    const Registro otra = crear("Iker Cortes", "iker.cortes", clave, SAL2, SAL, SECRETO, POCAS);
    CHECK(otra.hash != r.hash);
    // Ida y vuelta por el archivo.
    const Registro leido = deTexto(texto);
    REQUIRE(leido.valido());
    CHECK(leido.nombre == "Iker Cortes");
    CHECK(leido.cuenta == "iker.cortes@voxelworld");
    CHECK(leido.hash == r.hash);
    CHECK(leido.totp == r.totp);
    CHECK(leido.vueltas == POCAS);
    const long long t = 1700000123LL;
    CHECK(entrar(leido, "iker.cortes", clave, totp(SECRETO, t), t) == Entrada::DENTRO);
    // Con saltos de linea de Windows tambien.
    std::string crlf;
    for (char c : texto) { if (c == '\n') crlf += '\r'; crlf += c; }
    CHECK(deTexto(crlf).valido());
}

TEST_CASE("Cuenta: un archivo roto o retocado no se da por bueno") {
    const Registro r = crear("Iker", "iker", "tomate-rojo-9", SAL, SAL2, SECRETO, POCAS);
    const std::string texto = aTexto(r);
    CHECK_FALSE(deTexto("").valido());
    CHECK_FALSE(deTexto("hola\nque tal\n").valido());
    auto sin = [&](const std::string& clave) {
        std::string s;
        size_t i = 0;
        while (i < texto.size()) {
            const size_t f = texto.find('\n', i);
            const std::string l = texto.substr(i, f - i + 1);
            if (l.rfind(clave + "=", 0) != 0) s += l;
            i = f + 1;
        }
        return s;
    };
    for (const char* k : { "nombre", "cuenta", "sal", "hash", "saltotp", "totp", "vueltas" }) {
        INFO("sin " << k);
        CHECK_FALSE(deTexto(sin(k)).valido());
    }
    // Un hash recortado, una cuenta sin dominio, vueltas absurdas.
    std::string mal = texto;
    mal.replace(mal.find("hash=") + 5, 8, "");
    CHECK_FALSE(deTexto(mal).valido());
    mal = texto;
    mal.replace(mal.find("@voxelworld"), 11, "@otrositio");
    CHECK_FALSE(deTexto(mal).valido());
    mal = sin("vueltas") + "vueltas=1\n";
    CHECK_FALSE(deTexto(mal).valido());
    mal = sin("vueltas") + "vueltas=99999999999999\n";
    CHECK_FALSE(deTexto(mal).valido());
    // Cambiar el hash por otro no abre la cuenta.
    mal = texto;
    const size_t h = mal.find("hash=") + 5;
    mal[h] = mal[h] == 'a' ? 'b' : 'a';
    const Registro tocado = deTexto(mal);
    const long long t = 1700000000LL;
    CHECK(entrar(tocado, "iker", "tomate-rojo-9", totp(SECRETO, t), t) != Entrada::DENTRO);
}

TEST_CASE("Cuenta: que nombres, cuentas y contrasenas valen") {
    CHECK(nombreVale("Iker"));
    CHECK(nombreVale("Iker Cortes 01"));
    CHECK_FALSE(nombreVale(""));
    CHECK_FALSE(nombreVale(" Iker"));
    CHECK_FALSE(nombreVale("Iker "));
    CHECK_FALSE(nombreVale("Iker\nCortes"));
    CHECK(nombreVale("nombre=otro"));                       // los signos valen (ver test_texto_utf8)
    CHECK_FALSE(nombreVale("Iker\ncuenta=otro"));           // pero no se cuela otra linea del archivo
    CHECK_FALSE(nombreVale(std::string(NOMBRE_MAX + 1, 'a')));
    CHECK(usuarioVale("iker"));
    CHECK(usuarioVale("iker.cortes_01-x"));
    CHECK_FALSE(usuarioVale("ik"));
    CHECK_FALSE(usuarioVale("Iker"));                       // se guarda en minusculas
    CHECK_FALSE(usuarioVale("iker cortes"));
    CHECK_FALSE(usuarioVale("iker@voxelworld"));
    CHECK_FALSE(usuarioVale(std::string(USUARIO_MAX + 1, 'a')));
    CHECK(normalizarUsuario("IkerCortes@voxelworld") == "ikercortes");
    CHECK(claveVale("tomate-rojo-9"));
    CHECK(claveVale("con espacios y $imbolos!"));
    CHECK_FALSE(claveVale("corta"));
    CHECK_FALSE(claveVale(std::string(CLAVE_MAX + 1, 'a')));
    CHECK_FALSE(claveVale("con\ttabulador"));
    CHECK(VUELTAS >= 100000);
    // aHex y deHex, ida y vuelta, y deHex no se traga basura.
    CHECK(deHex(aHex(SECRETO)) == SECRETO);
    CHECK(deHex("zz").empty());
    CHECK(deHex("abc").empty());
    CHECK(iguales(SAL, SAL));
    CHECK_FALSE(iguales(SAL, SAL2));
    CHECK_FALSE(iguales(SAL, Bytes(SAL.begin(), SAL.end() - 1)));
}

TEST_CASE("Cuenta: la pagina web hace las mismas cuentas que el juego") {
    // adquirir.html crea el archivo en el navegador con WebCrypto. Tiene que
    // usar los mismos parametros o una cuenta hecha alli no entraria aqui.
    const std::string web = leerDelRepo("adquirir.html");
    REQUIRE_FALSE(web.empty());
    CHECK(web.find("PBKDF2") != std::string::npos);
    CHECK(web.find("SHA-256") != std::string::npos);
    CHECK(web.find("const VUELTAS = " + std::to_string(VUELTAS) + ";") != std::string::npos);
    CHECK(web.find("@voxelworld") != std::string::npos);
    CHECK(web.find("SHA-1") != std::string::npos);          // el autenticador
    for (const char* campo : { "nombre=", "cuenta=", "vueltas=", "sal=", "hash=", "saltotp=", "totp=" })
        CHECK(web.find(campo) != std::string::npos);
    // La contrasena no sale del navegador.
    CHECK(web.find("fetch(\"http") == std::string::npos);
    CHECK(web.find("XMLHttpRequest") == std::string::npos);
    CHECK(web.find("localStorage.setItem(\"clave") == std::string::npos);
}

TEST_CASE("Cuenta: con las vueltas de verdad, el juego calcula lo mismo que la web y que la referencia") {
    // Valores sacados con hashlib.pbkdf2_hmac (Python) y con el propio codigo
    // de adquirir.html ejecutado en Node (WebCrypto): los tres coinciden. Si
    // este test falla, una cuenta creada en la pagina no entraria en el juego.
    CHECK(hexDe(pbkdf2("tomate-rojo-9", SAL, VUELTAS, 32)) ==
          "42916edf2494fb6e12df521e786af872ad3aa81db0481b98881fe492a882dd79");
    CHECK(hexDe(pbkdf2("tomate-rojo-9", SAL2, VUELTAS, 20)) == "940d37d489cd3721820a6f10c7aeb12b897d2d23");
    // Un archivo tal como lo escribe la pagina (mismas lineas, mismo orden).
    const Bytes hash = pbkdf2("tomate-rojo-9", SAL, VUELTAS, 32);
    const Bytes mascara = pbkdf2("tomate-rojo-9", SAL2, VUELTAS, 20);
    Bytes tapado(20);
    for (size_t i = 0; i < 20; ++i) tapado[i] = SECRETO[i] ^ mascara[i];
    const std::string deLaWeb =
        "# Cuenta de Voxel World. La contrasena NO esta aqui: solo su hash\n"
        "# (PBKDF2-HMAC-SHA256). El secreto del autenticador va tapado con ella.\n"
        "version=1\n"
        "nombre=Iker\n"
        "cuenta=iker@voxelworld\n"
        "vueltas=120000\n"
        "sal=" + aHex(SAL) + "\n"
        "hash=" + aHex(hash) + "\n"
        "saltotp=" + aHex(SAL2) + "\n"
        "totp=" + aHex(tapado) + "\n";
    const Registro r = deTexto(deLaWeb);
    REQUIRE(r.valido());
    const long long t = 1800000000LL;
    CHECK(entrar(r, "iker", "tomate-rojo-9", totp(SECRETO, t), t) == Entrada::DENTRO);
    CHECK(entrar(r, "iker", "tomate-rojo-8", totp(SECRETO, t), t) == Entrada::CREDENCIALES);
    // Y lo que escribe el juego es, linea por linea, lo mismo.
    CHECK(aTexto(crear("Iker", "iker", "tomate-rojo-9", SAL, SAL2, SECRETO)) == deLaWeb);
}

TEST_CASE("Cuenta: el juego no deja pasar al menu sin entrar") {
    const std::string src = leerDelRepo("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    // El menu principal: sin cuenta abierta solo se dibuja el formulario, y vuelve.
    const size_t m = src.find("void renderMainMenu(GameState* state, int screenWidth, int screenHeight, GLFWwindow* window) {");
    REQUIRE(m != std::string::npos);
    const size_t puerta = src.find("if (!g_cuentaUI.dentro) {", m);
    const size_t botones = src.find("boton(state->btnMundosSolitarios", m);
    REQUIRE(puerta != std::string::npos);
    REQUIRE(botones != std::string::npos);
    CHECK(puerta < botones);
    CHECK(src.substr(puerta, botones - puerta).find("return;") != std::string::npos);
    // Teclado y raton: mientras no se entre, son del formulario.
    CHECK(src.find("if (cuentaPideEntrada()) { cuentaChar(codepoint); return; }") != std::string::npos);
    CHECK(src.find("if (cuentaPideEntrada()) { cuentaTecla(key, action); return; }") != std::string::npos);
    CHECK(src.find("if (cuentaPideEntrada()) { cuentaClic((float)xpos, (float)ypos, width, height); return; }") !=
          std::string::npos);
    // Se entra solo por Cuenta::entrar o al crear confirmando el codigo.
    size_t veces = 0;
    for (size_t p = src.find("c.dentro = true;"); p != std::string::npos; p = src.find("c.dentro = true;", p + 1)) ++veces;
    CHECK(veces == 2);
    CHECK(src.find("Cuenta::entrar(c.reg, c.usuario, c.clave, codigo, (long long)time(nullptr))") != std::string::npos);
    CHECK(src.find("Cuenta::totpVale(c.secreto, codigo, (long long)time(nullptr))") != std::string::npos);
    // El dialogo de archivos no mueve la carpeta de trabajo (ahi estan las partidas).
    CHECK(src.find("OFN_NOCHANGEDIR") != std::string::npos);
    // La insignia: abajo a la derecha, con el circulo y el nombre.
    CHECK(src.find("dibujarInsigniaDeCuenta(screenWidth, screenHeight);") != std::string::npos);
    CHECK(src.find("\"CUENTA REGISTRADA\"") != std::string::npos);
}

TEST_CASE("Cuenta: la pagina deja entrar con la cuenta de Ludora y manda el codigo a sus notificaciones") {
    // PEDIDO: "anade dos botones: (Tienes cuenta de Ludora Engine?) [...] te
    // lleva a otra seccion que se sobrepone [...] y otro boton (comprobar por
    // codigo) que le mandara al usuario, en una nueva seccion de
    // notificaciones, un codigo de 6 numeros".
    const std::string web = leerDelRepo("adquirir.html");
    REQUIRE_FALSE(web.empty());
    CHECK(web.find("id=\"c-ludora\"") != std::string::npos);
    CHECK(web.find("Tienes cuenta de Ludora Engine?") != std::string::npos);
    CHECK(web.find("id=\"ludora-caja\"") != std::string::npos);
    CHECK(web.find("@ludora.engine") != std::string::npos);
    CHECK(web.find("Comprobar por c&oacute;digo") != std::string::npos);
    CHECK(web.find("id=\"notificaciones\"") != std::string::npos);
    // La contrasena se comprueba como en Ludora: SHA-256(salt + contrasena).
    CHECK(web.find("sha256Hex(cuenta.salt + clave)") != std::string::npos);
    // El codigo: al azar del sistema, caduca y tiene intentos contados.
    CHECK(web.find("crypto.getRandomValues(n)") != std::string::npos);
    CHECK(web.find("CADUCA_MS = 5 * 60 * 1000") != std::string::npos);
    CHECK(web.find("INTENTOS = 5") != std::string::npos);
    CHECK(web.find("Math.random") == std::string::npos);
    // Las notificaciones no se bloquean con el formulario delante.
    CHECK(web.find("e.id !== \"notificaciones\"") != std::string::npos);
    const std::string css = leerDelRepo("estilo.css");
    CHECK(css.find("#ludora-caja") != std::string::npos);
    CHECK(css.find("#notificaciones") != std::string::npos);
}
