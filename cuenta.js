// ============================================================================
// LA CUENTA DEL PORTAL: UNA SOLA PARA VOXEL WORLD Y PARA LUDORA
// ============================================================================
// No hay servidor de cuentas: la cuenta vive en archivos en el equipo de cada
// uno (cuenta.dat para Ludora, cuenta.vwcuenta para Voxel World) y la sesion
// en este navegador (localStorage). Las dos paginas comparten la misma sesion,
// y al crear la cuenta o al entrar se preparan las credenciales de LOS DOS
// juegos a partir de la misma contrasena, asi una cuenta sirve en ambos:
//
//   Ludora       hash = SHA-256(salt + contrasena)            (cuenta.dat)
//   Voxel World  hash = PBKDF2-HMAC-SHA256(contrasena, sal,   (cuenta.vwcuenta)
//                       120000 vueltas)
//
// La contrasena nunca se guarda ni sale del navegador. Al servidor solo va el
// nombre y la cuenta, cuando se crea (api/registros), para avisar del registro.
//
// Uso desde cada pagina:
//   Portal.sesion()                 la cuenta de este navegador, o null
//   await Portal.crear({...})       crea la cuenta y la guarda
//   Portal.archivoLudora(s) / Portal.archivoVW(s) / Portal.descargar(...)
//   Portal.pintarInsignia()         la insignia de abajo a la derecha
//   Portal.alCambiar(fn)            fn(sesion) al entrar o cerrar sesion
//   Portal.abrirInicio()            "ya tengo cuenta": entrar con el archivo
//   await Portal.reducirFoto(file)  la foto a 128 px, para que quepa
(function () {
    const CLAVE = "portal-cuenta";
    const VIEJAS = ["ludora-cuenta", "vw-cuenta", "vw-foto"];
    const DOMINIO = "@ludora.engine";
    const VUELTAS = 120000;
    const NOMBRE = /^[\x20-\x7e\xa1-\xff]{1,24}$/;
    const LIMPIO = /[^\x20-\x7e\xa1-\xff]/g;

    const el = id => document.getElementById(id);
    const hex = b => [...new Uint8Array(b)].map(v => v.toString(16).padStart(2, "0")).join("");
    const desdeHex = h => new Uint8Array((h.match(/../g) || []).map(x => parseInt(x, 16)));
    const utf8 = t => new TextEncoder().encode(t);
    const latin1 = t => Uint8Array.from(t, c => c.charCodeAt(0));   // como Cuenta.h del juego
    let oyentes = [];

    // ---------------------------------------------------------------- datos
    function guardar(s) {
        try { localStorage.setItem(CLAVE, JSON.stringify(s)); return true; }
        catch (e) { return false; }      // sin sitio o sin almacenamiento
    }
    // Lo que guardaban antes cada pagina por su lado pasa a la sesion comun.
    function migrar() {
        let s = null;
        try {
            const l = JSON.parse(localStorage.getItem("ludora-cuenta") || "null");
            const v = JSON.parse(localStorage.getItem("vw-cuenta") || "null");
            const base = (l && l.nombre && l.cuenta) ? l : (v && v.nombre && v.cuenta) ? v : null;
            if (base) {
                s = { nombre: base.nombre, cuenta: base.cuenta, desc: l?.desc || "", estado: l?.estado || "",
                      juego: l?.juego || "", color: l?.color || "",
                      foto: l?.foto || localStorage.getItem("vw-foto") || "",
                      ludora: l && /^[0-9a-f]{32}$/.test(l.salt || "") && /^[0-9a-f]{64}$/.test(l.hash || "")
                              ? { salt: l.salt, hash: l.hash } : null,
                      vw: null };
                if (s.foto.length > 400000) s.foto = "";
                if (guardar(s)) for (const k of VIEJAS) localStorage.removeItem(k);
            }
        } catch (e) { s = null; }
        return s;
    }
    function sesion() {
        let s = null;
        try { s = JSON.parse(localStorage.getItem(CLAVE) || "null"); } catch (e) { s = null; }
        if (!s) s = migrar();
        if (!s || !s.nombre || !s.cuenta) return null;
        s.nombre = String(s.nombre).replace(LIMPIO, "").slice(0, 24) || "?";
        return s;
    }

    // --------------------------------------------------------- credenciales
    async function sha256Hex(t) { return hex(await crypto.subtle.digest("SHA-256", utf8(t))); }
    async function pbkdf2(clave, sal) {
        const k = await crypto.subtle.importKey("raw", latin1(clave), "PBKDF2", false, ["deriveBits"]);
        return new Uint8Array(await crypto.subtle.deriveBits(
            { name: "PBKDF2", hash: "SHA-256", salt: sal, iterations: VUELTAS }, k, 256));
    }
    function iguales(a, b) {          // sin cortar en el primer caracter distinto
        let d = a.length ^ b.length;
        for (let i = 0; i < a.length; ++i) d |= a.charCodeAt(i) ^ b.charCodeAt(i % (b.length || 1));
        return d === 0;
    }
    // Las credenciales de los dos juegos, cada una con sus reglas: Ludora pide
    // la contrasena en ASCII; Voxel World, la cuenta en minusculas.
    async function credenciales(local, clave) {
        const r = { ludora: null, vw: null };
        if (/^[A-Za-z0-9._-]{1,24}$/.test(local) && /^[\x20-\x7e]{6,64}$/.test(clave)) {
            const salt = hex(crypto.getRandomValues(new Uint8Array(16)));
            r.ludora = { salt, hash: await sha256Hex(salt + clave) };
        }
        const vwLocal = local.toLowerCase();
        if (/^[a-z0-9._-]{3,24}$/.test(vwLocal) && /^[\x20-\x7e\xa1-\xff]{6,64}$/.test(clave)) {
            const sal = crypto.getRandomValues(new Uint8Array(16));
            r.vw = { cuenta: vwLocal + DOMINIO, vueltas: VUELTAS, sal: hex(sal), hash: hex(await pbkdf2(clave, sal)) };
        }
        return r;
    }
    async function comprobar(s, clave) {
        if (s.ludora) return iguales(await sha256Hex(s.ludora.salt + clave), s.ludora.hash);
        if (s.vw) return iguales(hex(await pbkdf2(clave, desdeHex(s.vw.sal))), s.vw.hash);
        return null;                      // sin credenciales guardadas: no se puede
    }

    // -------------------------------------------------------------- archivos
    function archivoLudora(s) {
        return "# Cuenta de Ludora Engine.\r\n" +
               "# La contrasena NO se guarda: solo SHA-256(salt + contrasena).\r\n" +
               "email=" + s.cuenta + "\r\n" + "salt=" + s.ludora.salt + "\r\n" + "hash=" + s.ludora.hash + "\r\n";
    }
    function archivoVW(s) {
        return "# Cuenta de Voxel World. La contrasena NO esta aqui: solo su hash\n" +
               "# (PBKDF2-HMAC-SHA256).\n" +
               "version=2\n" + "nombre=" + s.nombre + "\n" + "cuenta=" + s.vw.cuenta + "\n" +
               "vueltas=" + s.vw.vueltas + "\n" + "sal=" + s.vw.sal + "\n" + "hash=" + s.vw.hash + "\n";
    }
    function descargar(nombre, contenido) {
        const a = document.createElement("a");
        a.href = URL.createObjectURL(new Blob([contenido], { type: "text/plain" }));
        a.download = nombre;
        a.click();
        setTimeout(() => URL.revokeObjectURL(a.href), 60000);
    }
    function leerArchivo(texto) {
        const r = {};
        for (const linea of texto.split(/\r?\n/)) {
            if (!linea || linea[0] === "#") continue;
            const i = linea.indexOf("=");
            if (i > 0) r[linea.slice(0, i).trim()] = linea.slice(i + 1).trim();
        }
        if (r.email && /^[0-9a-f]{32}$/.test(r.salt || "") && /^[0-9a-f]{64}$/.test(r.hash || ""))
            return { tipo: "ludora", cuenta: r.email, ludora: { salt: r.salt, hash: r.hash } };
        if (r.cuenta && r.sal && r.hash && +r.vueltas > 0)
            return { tipo: "vw", cuenta: r.cuenta, nombre: r.nombre || "",
                     vw: { cuenta: r.cuenta, vueltas: +r.vueltas, sal: r.sal, hash: r.hash } };
        return null;
    }

    function reducirFoto(archivo) {
        return new Promise((bien, mal) => {
            const img = new Image();
            img.onload = () => {
                const L = 128, c = document.createElement("canvas");
                c.width = c.height = L;
                const m = Math.min(img.width, img.height);
                c.getContext("2d").drawImage(img, (img.width - m) / 2, (img.height - m) / 2, m, m, 0, 0, L, L);
                URL.revokeObjectURL(img.src);
                bien(c.toDataURL("image/jpeg", 0.85));
            };
            img.onerror = () => mal(new Error("Esa imagen no se pudo leer."));
            img.src = URL.createObjectURL(archivo);
        });
    }

    // ---------------------------------------------------------- crear y salir
    async function crear({ nombre, local, clave, foto, origen }) {
        const cred = await credenciales(local, clave);
        if (!cred.ludora && !cred.vw) throw new Error("Esa cuenta o contraseña no sirve para ningún juego.");
        const s = { nombre, cuenta: local + DOMINIO, desc: "", estado: "", juego: "", color: "",
                    foto: foto || "", ludora: cred.ludora, vw: cred.vw };
        guardar(s);
        // Aviso de cuenta nueva: SOLO el nombre y la cuenta.
        fetch("api/registros", {
            method: "POST", headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ tipo: origen, nombre, cuenta: s.cuenta }), keepalive: true,
        }).catch(() => {});
        pintarInsignia();
        return s;
    }
    function cerrarSesion() {
        try { for (const k of [CLAVE, ...VIEJAS]) localStorage.removeItem(k); } catch (e) {}
        el("cuenta-insignia").hidden = true;
        for (const fn of oyentes) fn(null);
    }

    // -------------------------------------------------------------- insignia
    function pintarFoto(donde, foto, nombre) {
        donde.textContent = foto ? "" : (nombre[0] || "?").toUpperCase();
        donde.style.backgroundImage = foto ? "url(" + foto + ")" : "";
    }
    function pintarInsignia() {
        const s = sesion(), ins = el("cuenta-insignia");
        if (!s || !ins) { if (ins) ins.hidden = true; return; }
        el("insignia-nombre").textContent = s.nombre;
        el("insignia-cuenta").textContent = s.estado ? s.estado + " · " + s.cuenta : s.cuenta;
        pintarFoto(el("insignia-foto"), s.foto, s.nombre);
        ins.style.borderColor = s.color || "";
        ins.title = s.desc || "";
        ins.hidden = false;
    }

    // ------------------------------------------------------ menu y dialogos
    // Se crean la primera vez que se usan: las paginas marcan como "fondo"
    // (inerte tras el velo) lo que hay en el body al cargar, y esto no debe
    // quedar ahi.
    let menu = null;
    function crearMenu() {
        menu = document.createElement("div");
        menu.id = "cuenta-menu";
        menu.setAttribute("role", "menu");
        menu.hidden = true;
        menu.innerHTML =
            '<button type="button" role="menuitem" data-accion="perfil">Editar perfil</button>' +
            '<button type="button" role="menuitem" data-accion="clave">Cambiar contraseña</button>' +
            '<button type="button" role="menuitem" data-accion="ludora">Descargar mi cuenta de Ludora (cuenta.dat)</button>' +
            '<button type="button" role="menuitem" data-accion="vw">Descargar mi cuenta de Voxel World (cuenta.vwcuenta)</button>' +
            '<button type="button" role="menuitem" data-accion="salir" class="peligro">Cerrar sesión</button>';
        document.body.append(menu);
        menu.addEventListener("click", (ev) => {
            const b = ev.target.closest("button");
            if (!b) return;
            abrirMenu(false);
            const s = sesion();
            if (!s) return;
            const accion = b.dataset.accion;
            if (accion === "perfil") abrirPerfil();
            else if (accion === "clave") abrirClave();
            else if (accion === "salir") abrirSalir();
            else if (accion === "ludora" || accion === "vw") {
                const hay = accion === "ludora" ? s.ludora : s.vw;
                if (!hay) return avisoGeneral("Tu cuenta es de antes y este navegador no guardó sus datos de " +
                    (accion === "ludora" ? "Ludora" : "Voxel World") + ". Cierra sesión y vuelve a entrar con tu archivo y tu contraseña: así se preparan.");
                if (accion === "ludora") descargar("cuenta.dat", archivoLudora(s));
                else descargar("cuenta.vwcuenta", archivoVW(s));
            }
        });
        document.addEventListener("click", (ev) => {
            if (!menu.hidden && !menu.contains(ev.target) && ev.target.id !== "insignia-cambiar") abrirMenu(false);
        });
        document.addEventListener("keydown", (ev) => { if (ev.key === "Escape") abrirMenu(false); });
    }
    function abrirMenu(si) {
        if (!menu) crearMenu();
        menu.hidden = !si;
        el("insignia-cambiar").setAttribute("aria-expanded", String(si));
        if (si) menu.querySelector("button").focus();
    }

    function dialogo(id, html) {
        let d = el(id);
        if (d) return d;
        d = document.createElement("dialog");
        d.id = id;
        d.className = "dialogo-cuenta";
        d.innerHTML = html;
        document.body.append(d);
        d.querySelectorAll("[data-cerrar]").forEach(b => b.addEventListener("click", () => d.close()));
        return d;
    }
    function avisarEn(d, t, bueno) {
        const p = d.querySelector(".aviso");
        p.textContent = t;
        p.className = "aviso " + (bueno ? "ok" : "error");
    }
    function avisoGeneral(t) {
        const d = dialogo("dlg-aviso",
            '<h2>Tu cuenta</h2><p class="aviso"></p>' +
            '<div class="fila"><button type="button" class="boton" data-cerrar><span>Entendido</span></button></div>');
        avisarEn(d, t, true);
        d.showModal();
    }

    // Editar perfil
    let fotoNueva = null;           // null = sin tocar; "" = quitada; data: = otra
    function abrirPerfil() {
        const d = dialogo("dlg-perfil",
            '<form method="dialog" novalidate>' +
            '<h2>Editar perfil</h2><p class="intro">Cuenta: <strong data-campo="cuenta"></strong></p>' +
            '<div class="perfil-foto"><span class="vista" aria-hidden="true"></span>' +
            '<label>Foto de perfil <input data-campo="foto" type="file" accept="image/png,image/jpeg,image/bmp,image/webp"></label>' +
            '<button type="button" class="plano" data-quitar>Quitar foto</button></div>' +
            '<label>Nombre <input data-campo="nombre" type="text" maxlength="24" autocomplete="name" required></label>' +
            '<label>Descripción <small>(hasta 160 caracteres)</small><textarea data-campo="desc" maxlength="160" rows="3" placeholder="Cuéntale a los demás quién eres o a qué juegas"></textarea></label>' +
            '<label>Estado <select data-campo="estado"><option value="">Sin estado</option><option>Disponible</option><option>Jugando</option><option>Ocupado</option><option>Ausente</option></select></label>' +
            '<label>Juego favorito <small>(opcional)</small><input data-campo="juego" type="text" maxlength="40"></label>' +
            '<label>Color de tu insignia <input data-campo="color" type="color" value="#e01e23"></label>' +
            '<div class="fila"><button type="button" class="plano" data-cerrar>Cancelar</button>' +
            '<button type="submit" class="boton" value="guardar"><span>Guardar</span></button></div>' +
            '<p class="aviso" role="alert"></p></form>');
        const f = c => d.querySelector('[data-campo="' + c + '"]');
        if (!d.dataset.listo) {
            d.dataset.listo = "1";
            f("foto").addEventListener("change", async (ev) => {
                const a = ev.target.files[0];
                if (!a) return;
                try { fotoNueva = await reducirFoto(a); pintarFoto(d.querySelector(".vista"), fotoNueva, f("nombre").value); }
                catch (e) { avisarEn(d, e.message, false); }
            });
            d.querySelector("[data-quitar]").addEventListener("click", () => {
                fotoNueva = ""; f("foto").value = "";
                pintarFoto(d.querySelector(".vista"), "", f("nombre").value);
            });
            d.querySelector("form").addEventListener("submit", (ev) => {
                const s = sesion();
                const nombre = f("nombre").value.replace(/ +/g, " ").trim();
                if (!NOMBRE.test(nombre)) { ev.preventDefault(); return avisarEn(d, "Tu nombre: hasta 24 caracteres.", false); }
                s.nombre = nombre;
                s.desc = f("desc").value.replace(/\s+/g, " ").trim().slice(0, 160);
                s.estado = f("estado").value;
                s.juego = f("juego").value.replace(/ +/g, " ").trim().slice(0, 40);
                s.color = f("color").value;
                if (fotoNueva !== null) s.foto = fotoNueva;
                if (!guardar(s)) { ev.preventDefault(); return avisarEn(d, "El navegador no deja guardar el perfil (sin espacio o en modo privado).", false); }
                pintarInsignia();
            });
        }
        const s = sesion();
        fotoNueva = null;
        f("foto").value = "";
        d.querySelector('[data-campo="cuenta"]').textContent = s.cuenta;
        f("nombre").value = s.nombre;
        f("desc").value = s.desc || "";
        f("estado").value = s.estado || "";
        f("juego").value = s.juego || "";
        f("color").value = s.color || "#e01e23";
        pintarFoto(d.querySelector(".vista"), s.foto, s.nombre);
        avisarEn(d, "", true);
        d.showModal();
    }

    // Cambiar contrasena: se comprueba la actual con lo guardado y salen los
    // archivos nuevos, que sustituyen a los viejos en el PC.
    function abrirClave() {
        const d = dialogo("dlg-clave",
            '<form novalidate><h2>Cambiar contraseña</h2>' +
            '<p class="intro">Se cambia para Ludora y para Voxel World. Al guardar se descargan tus archivos de cuenta nuevos: ponlos en lugar de los viejos (cuenta.dat junto a Ludora.exe; en Voxel World, IMPORTAR CUENTA).</p>' +
            '<label>Contraseña actual <input data-campo="actual" type="password" maxlength="64" autocomplete="current-password" required></label>' +
            '<label>Contraseña nueva <small>(6 o más caracteres)</small><input data-campo="nueva" type="password" maxlength="64" autocomplete="new-password" required></label>' +
            '<label>Repite la nueva <input data-campo="nueva2" type="password" maxlength="64" autocomplete="new-password" required></label>' +
            '<div class="fila"><button type="button" class="plano" data-cerrar>Cancelar</button>' +
            '<button type="submit" class="boton"><span>Cambiar</span></button></div>' +
            '<p class="aviso" role="alert"></p></form>');
        const f = c => d.querySelector('[data-campo="' + c + '"]');
        if (!d.dataset.listo) {
            d.dataset.listo = "1";
            d.querySelector("form").addEventListener("submit", async (ev) => {
                ev.preventDefault();
                const s = sesion();
                const actual = f("actual").value, nueva = f("nueva").value;
                if (nueva.length < 6) return avisarEn(d, "La contraseña nueva: 6 o más caracteres.", false);
                if (nueva !== f("nueva2").value) return avisarEn(d, "Las dos contraseñas nuevas no coinciden.", false);
                avisarEn(d, "Comprobando...", true);
                const ok = await comprobar(s, actual);
                if (ok === null) return avisarEn(d, "Tu cuenta es de antes y este navegador no guardó su comprobación. Cierra sesión y vuelve a entrar con tu archivo y tu contraseña.", false);
                if (!ok) return avisarEn(d, "La contraseña actual no es correcta.", false);
                const cred = await credenciales(s.cuenta.replace(/@.*$/, ""), nueva);
                if (!cred.ludora && !cred.vw) return avisarEn(d, "Esa contraseña no sirve: usa de 6 a 64 caracteres.", false);
                s.ludora = cred.ludora; s.vw = cred.vw;
                guardar(s);
                if (s.ludora) descargar("cuenta.dat", archivoLudora(s));
                if (s.vw) setTimeout(() => descargar("cuenta.vwcuenta", archivoVW(s)), 400);
                for (const c of ["actual", "nueva", "nueva2"]) f(c).value = "";
                avisarEn(d, "Contraseña cambiada. Ya se descargaron tus archivos nuevos: reemplaza los anteriores.", true);
            });
        }
        for (const c of ["actual", "nueva", "nueva2"]) f(c).value = "";
        avisarEn(d, "", true);
        d.showModal();
    }

    function abrirSalir() {
        const d = dialogo("dlg-salir",
            '<h2>Cerrar sesión</h2>' +
            '<p class="intro">Se cierra en este navegador. Tus archivos de cuenta no se borran: para volver a entrar necesitas tu archivo (cuenta.dat o cuenta.vwcuenta) y tu contraseña.</p>' +
            '<div class="fila"><button type="button" class="plano" data-cerrar>Cancelar</button>' +
            '<button type="button" class="boton" data-salir><span>Cerrar sesión</span></button></div>');
        if (!d.dataset.listo) {
            d.dataset.listo = "1";
            d.querySelector("[data-salir]").addEventListener("click", () => { d.close(); cerrarSesion(); });
        }
        d.showModal();
    }

    // Iniciar sesion con una cuenta que ya existe: su archivo y su contrasena.
    function abrirInicio() {
        const d = dialogo("dlg-inicio",
            '<form novalidate><h2>Iniciar sesión</h2>' +
            '<p class="intro">Elige tu archivo de cuenta (cuenta.dat de Ludora o cuenta.vwcuenta de Voxel World) y escribe tu contraseña. Se comprueba aquí, en tu navegador: nada se envía.</p>' +
            '<label>Tu archivo de cuenta <input data-campo="archivo" type="file" accept=".dat,.vwcuenta,text/plain" required></label>' +
            '<label>Contraseña <input data-campo="clave" type="password" maxlength="64" autocomplete="current-password" required></label>' +
            '<div class="fila"><button type="button" class="plano" data-cerrar>Cancelar</button>' +
            '<button type="submit" class="boton"><span>Entrar</span></button></div>' +
            '<p class="aviso" role="alert"></p></form>');
        const f = c => d.querySelector('[data-campo="' + c + '"]');
        if (!d.dataset.listo) {
            d.dataset.listo = "1";
            d.querySelector("form").addEventListener("submit", async (ev) => {
                ev.preventDefault();
                if (!window.crypto || !crypto.subtle) return avisarEn(d, "Este navegador no deja comprobar la cuenta (hace falta https).", false);
                const a = f("archivo").files[0], clave = f("clave").value;
                if (!a) return avisarEn(d, "Elige tu archivo de cuenta.", false);
                if (!clave) return avisarEn(d, "Escribe tu contraseña.", false);
                const leido = leerArchivo(await a.text());
                if (!leido) return avisarEn(d, "Ese archivo no es una cuenta de Ludora ni de Voxel World.", false);
                avisarEn(d, "Comprobando...", true);
                const ok = await comprobar(leido, clave);
                if (!ok) return avisarEn(d, "Cuenta o contraseña incorrectas.", false);
                // Con la contrasena correcta se preparan las credenciales que
                // falten, para que la cuenta sirva tambien en el otro juego.
                const local = leido.cuenta.replace(/@.*$/, "");
                const cred = await credenciales(local, clave);
                const s = { nombre: (leido.nombre || local).replace(LIMPIO, "").slice(0, 24) || local,
                            cuenta: leido.cuenta, desc: "", estado: "", juego: "", color: "", foto: "",
                            ludora: leido.ludora || cred.ludora, vw: leido.vw || cred.vw };
                guardar(s);
                f("clave").value = ""; f("archivo").value = "";
                d.close();
                pintarInsignia();
                for (const fn of oyentes) fn(s);
            });
        }
        f("clave").value = ""; f("archivo").value = "";
        avisarEn(d, "", true);
        d.showModal();
    }

    // El boton de la insignia abre el menu de la cuenta.
    document.addEventListener("DOMContentLoaded", () => {
        const b = el("insignia-cambiar");
        if (!b) return;
        pintarInsignia();     // en cualquier pagina con insignia, si hay sesion
        b.textContent = "Editar";
        b.title = "Tu cuenta: perfil, contraseña, archivos y cerrar sesión";
        b.setAttribute("aria-haspopup", "menu");
        b.setAttribute("aria-expanded", "false");
        b.addEventListener("click", (ev) => { ev.stopPropagation(); abrirMenu(!menu || menu.hidden); });
    });

    window.Portal = {
        sesion, crear, cerrarSesion, pintarInsignia, abrirInicio, reducirFoto,
        archivoLudora, archivoVW, descargar,
        alCambiar(fn) { oyentes.push(fn); },
    };
})();
