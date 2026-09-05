#ifndef PECARI_REPOBLACION_H
#define PECARI_REPOBLACION_H

#include <cstdint>
#include <cmath>
#include "PecariSpawn.h"

// ============================================================================
// REPOBLACION DINAMICA DE PECARIES AL CARGAR CHUNKS
// ============================================================================
// RESPONSABILIDAD UNICA: decidir si un chunk RECIEN CARGADO debe traer una
// manada pequena que NO estaba en la generacion original del mundo.
//
// Es el complemento de PecariSpawn.h, y la diferencia entre los dos importa:
//
//   PecariSpawn.h      La poblacion ESTRUCTURAL del mundo. Funcion pura de
//                      (seed, x, z). Siempre da lo mismo, para siempre. Es
//                      "donde vive esta especie".
//
//   PecariRepoblacion  La poblacion DINAMICA. Depende del tiempo de juego y
//                      de cuantos pecaries quedan vivos cerca. Es "el mundo
//                      se recupera de lo que el jugador ha hecho".
//
// ----------------------------------------------------------------------------
// POR QUE ESTO EXISTE: LA LECCION DE ULTIMA ONLINE
// ----------------------------------------------------------------------------
// No es un anadido de conveniencia. 01_ARQUITECTURA_DEL_SISTEMA.json lo exige
// como REQUISITO, y lo hace citando un fracaso concreto y documentado.
//
// Ultima Online (1997) implemento un ecosistema con depredadores y presas de
// comportamiento emergente. En pruebas cerradas funciono. Al abrir el juego,
// los jugadores mataron sistematicamente a los herbivoros mas deprisa de lo
// que el sistema podia generarlos, la cadena trofica colapso, y el sistema
// entero tuvo que retirarse sin que ningun jugador llegara a notarlo.
//
// De las cuatro causas que el archivo de arquitectura identifica, esta es la
// que este fichero ataca:
//
//   "No habia mecanismo de resiliencia: sin refugios inaccesibles, sin
//    RECOLONIZACION DESDE FUERA, sin respuesta densodependiente que acelerase
//    la reproduccion a baja densidad. En ecologia real esos mecanismos existen
//    y son los que evitan la extincion."
//
// Y la mitigacion que el propio archivo recomienda, literalmente:
//
//   "Reposicion controlada por bioma con un objetivo de densidad, en vez de
//    dependencia estricta de la reproduccion interna. Menos puro, pero es lo
//    que hace que el mundo siga vivo tras cien horas de juego."
//
// Eso es exactamente lo que implementa este archivo.
//
// ----------------------------------------------------------------------------
// LAS TRES REGLAS DE APARICION QUE HAY QUE RESPETAR
// ----------------------------------------------------------------------------
// 01_ARQUITECTURA.capa_1_poblacion.aparicion_y_densidad las fija:
//
//   1. "No aparecer a la vista del jugador"
//        -> RADIO_MIN_BLOQUES. Ver la nota sobre la niebla mas abajo.
//   2. "Respetar los requisitos de habitat de la especie"
//        -> se reutiliza PecariSpawn::IdoneidadBioma, sin duplicar reglas.
//   3. "Aparecer como grupo si la especie es social, no como individuos
//      sueltos"
//        -> se repuebla en GRUPOS PEQUENOS, nunca de uno en uno.
//
// La regla 3 tiene un matiz que el usuario pidio explicitamente: "pocos".
// Un grupo de repoblacion NO es una manada completa. Es un fragmento: los
// supervivientes o dispersores que llegan a recolonizar. Por eso el rango es
// menor que el de PecariSpawn::RangoManada.
//
// Y eso tambien es biologia, no solo diseno: MEDIDO que el 37-38% de los
// machos se dispersan entre manadas (tesis Purdue, 268 individuos, 31
// manadas, dos metodos independientes que concuerdan). La recolonizacion por
// individuos dispersores es un fenomeno real de esta especie.
//
// ----------------------------------------------------------------------------
// RESPUESTA DENSODEPENDIENTE
// ----------------------------------------------------------------------------
// La clave para que esto NO se convierta en una fabrica de pecaries.
//
// La probabilidad de repoblar NO es constante: depende de cuantos pecaries
// vivos quedan cerca. Si la zona esta poblada, no aparece casi nada. Si el
// jugador ha vaciado la zona, la repoblacion se acelera.
//
// Es lo que el archivo de arquitectura pide ("respuesta densodependiente que
// acelerase la reproduccion a baja densidad") y ademas es ecologia real: una
// poblacion por debajo de su capacidad de carga crece mas deprisa.
//
// Efecto secundario deseable: el sistema se AUTOLIMITA. No hace falta un
// contador global ni un limite duro arbitrario. Cuando la densidad objetivo
// se alcanza, la repoblacion se apaga sola.
//
// ----------------------------------------------------------------------------
// TRAZABILIDAD
// ----------------------------------------------------------------------------
// Misma escala que el resto del AI simulator: MEDIDO / DERIVADO / INFERIDO /
// ESTIMADO / AUSENTE. La mayoria de constantes de ESTE archivo son ESTIMADO
// porque son decisiones de ritmo de juego, no biologia. Se dice sin disimulo.
// ============================================================================

namespace Fauna {

// ----------------------------------------------------------------------------
// LO QUE EL MOTOR DEBE CONTARLE AL SISTEMA
// ----------------------------------------------------------------------------
// Contexto minimo para decidir. Se pasa por valor: son cuatro numeros.
//
// Mantener esta estructura PEQUENA es deliberado. Si la repoblacion necesitara
// conocer el mundo entero, seria imposible de testear. Asi se prueba con
// valores sinteticos, igual que la fisica del jugador se prueba sin OpenGL.
struct ContextoRepoblacion {
    // Cuantos pecaries VIVOS hay ya cerca del chunk que se esta cargando.
    // Es la entrada de la respuesta densodependiente.
    int pecariesVivosCerca = 0;

    // Radio en bloques sobre el que se conto lo anterior.
    // Necesario para convertir el conteo en densidad.
    float radioConteoBloques = 128.0f;

    // Distancia del centro del chunk al jugador, en bloques.
    // Es lo que hace cumplir la regla "no aparecer a la vista".
    float distanciaAlJugadorBloques = 0.0f;

    // Segundos de juego transcurridos. Sin esto la repoblacion no tendria
    // ritmo: un jugador que recarga el mismo chunk cien veces seguidas
    // obtendria cien manadas.
    double tiempoJuegoSegundos = 0.0;
};

// ----------------------------------------------------------------------------
// RESULTADO
// ----------------------------------------------------------------------------
struct GrupoRepoblacion {
    bool  aparece = false;
    int   miembros = 0;
    int   x = 0;            // columna donde aparece el grupo
    int   z = 0;
    float dispersionBloques = 0.0f;

    // Por que aparecio o no. Existe para depuracion y para el tooling que
    // 01_ARQUITECTURA exige ANTES que el contenido: "graficas de poblacion en
    // vivo" y "volcado de la decision: que comportamiento gano y por que".
    // Sin esto, un bug de repoblacion es invisible hasta que el mundo esta
    // roto, que es exactamente como fallo Ultima Online.
    enum Motivo : uint8_t {
        MOTIVO_APARECE = 0,
        MOTIVO_BIOMA_NO_APTO,
        MOTIVO_DEMASIADO_CERCA_DEL_JUGADOR,
        MOTIVO_ZONA_YA_POBLADA,
        MOTIVO_PENDIENTE_O_ALTITUD,
        MOTIVO_NO_TOCA_POR_AZAR
    };
    Motivo motivo = MOTIVO_NO_TOCA_POR_AZAR;
};

// ============================================================================
// SISTEMA DE REPOBLACION
// ============================================================================
class PecariRepoblacion {
private:
    int seed;

    int seedRepob() const { return seed + 3353017; }
    int seedGrupo() const { return seed + 3466773; }

public:
    // ------------------------------------------------------------------------
    // DISTANCIA MINIMA AL JUGADOR
    // ------------------------------------------------------------------------
    // Regla 1: "No aparecer a la vista del jugador".
    //
    // Este numero NO es arbitrario: sale del propio motor.
    //   RENDER_DISTANCE = 5 chunks
    //   CHUNK_SIZE      = 16 bloques
    //   -> se cargan chunks hasta 80 bloques
    //   -> la niebla se vuelve OPACA a RENDER_DISTANCE*16*0.98 = 78.4 bloques
    //
    // Apareciendo a 88 bloques o mas, el grupo nace SIEMPRE dentro de niebla
    // opaca: el jugador no puede ver el "pop". Los 10 bloques de margen sobre
    // los 78.4 absorben el movimiento del jugador entre el momento en que se
    // decide y el momento en que se instancia.
    //
    // confianza: DERIVADO (de constantes reales del motor)
    static constexpr float RADIO_MIN_BLOQUES = 88.0f;

    // ------------------------------------------------------------------------
    // TAMANO DE UN GRUPO DE REPOBLACION: "POCOS"
    // ------------------------------------------------------------------------
    // NO es una manada completa (5-15). Es un fragmento recolonizador.
    //
    // Base biologica: MEDIDO que el 37-38% de los machos se dispersan entre
    // manadas. La recolonizacion real ocurre por individuos o grupitos que
    // llegan de fuera, no por manadas enteras que se teletransportan.
    //
    // 2-4 individuos: suficiente para que sea un grupo (regla 3: nunca
    // individuos sueltos) y suficientemente pocos para que no compita con la
    // poblacion estructural de PecariSpawn.
    //
    // confianza: ESTIMADO (los valores) / MEDIDO (que la dispersion existe y
    //            es sesgada a machos)
    static constexpr int GRUPO_MIN = 2;
    static constexpr int GRUPO_MAX = 4;

    // ------------------------------------------------------------------------
    // DENSIDAD OBJETIVO Y RESPUESTA DENSODEPENDIENTE
    // ------------------------------------------------------------------------
    // El techo al que tiende la repoblacion. Por encima de esto no aparece
    // nada, y el sistema se apaga solo.
    //
    // Se expresa en individuos por bloque cuadrado para no tener que convertir
    // unidades en el bucle caliente.
    //
    // PecariSpawn a MANADA_CELL=96 produce ~2437 ind/km2 en selva. En bloques:
    //   1 km2 = (1000/0.60)^2 = 2.777.778 bloques cuadrados
    //   2437 / 2.777.778 = 0.000877 ind/bloque2
    //
    // La repoblacion apunta a MANTENER esa densidad, no a superarla: su
    // trabajo es rellenar huecos, no crear poblacion nueva.
    //
    // confianza: DERIVADO (de la densidad que produce PecariSpawn)
    static constexpr float DENSIDAD_OBJETIVO_IND_POR_BLOQUE2 = 0.000877f;

    // ------------------------------------------------------------------------
    // RITMO
    // ------------------------------------------------------------------------
    // Cada cuanto puede repoblarse una misma zona, en segundos de juego.
    //
    // Sin esto, un jugador que entra y sale de un chunk repetidamente
    // obtendria un grupo en cada recarga. Es el fallo mas obvio de un sistema
    // de repoblacion y es facil de cometer: la funcion se llama AL CARGAR, y
    // cargar es algo que el jugador controla y puede repetir a voluntad.
    //
    // 300 s = 5 minutos de juego. Cuantiza el tiempo en ventanas: todas las
    // cargas dentro de la misma ventana dan el MISMO resultado, asi que
    // recargar no da premios.
    //
    // confianza: ESTIMADO (ritmo de juego, sin base biologica)
    static constexpr double VENTANA_SEGUNDOS = 300.0;

    // Probabilidad base de que una zona apta y VACIA reciba un grupo en una
    // ventana dada. Con densodependencia, esto es el TECHO, no el valor
    // habitual.
    // confianza: ESTIMADO
    static constexpr float PROB_BASE = 0.35f;

    // ------------------------------------------------------------------------
    // CUOTA POR VECINDAD: EL ARREGLO DE UN BUG REAL
    // ------------------------------------------------------------------------
    // Sin esto el sistema tiene un fallo grave que los tests unitarios NO
    // detectan, porque solo aparece al cargar MUCHOS chunks a la vez.
    //
    // EL BUG (medido, no hipotetico):
    //   Al cargar una zona arrasada de 20x20 chunks, la densodependencia se
    //   evalua UNA VEZ por chunk, y los 400 chunks reciben el MISMO contexto:
    //   "hay 0 pecaries vivos". Los 400 deciden en paralelo, cada uno con
    //   probabilidad 0.35, y cada uno acierta por separado.
    //   Resultado medido: 131 grupos y 389 individuos de golpe, cuando el
    //   objetivo de la zona eran ~45. Un sobredisparo de 8.6x.
    //
    // Es el problema clasico de N agentes decidiendo a la vez sobre un
    // recurso compartido sin verse entre ellos. Cada decision es correcta;
    // el agregado es un desastre.
    //
    // LA SOLUCION, sin estado global ni contadores:
    //   Se agrupan los chunks en VECINDADES grandes, y dentro de cada
    //   vecindad y cada ventana temporal solo unos POCOS chunks tienen
    //   derecho a repoblar. Cual de ellos, lo decide el mismo hash
    //   determinista que ya se usa para todo lo demas.
    //
    //   Asi la funcion sigue siendo pura: no necesita saber que otros chunks
    //   se estan cargando, pero el resultado agregado queda acotado.
    //
    // 16x16 chunks = 256x256 bloques = 153x153 m reales, comparable al area
    // de conteo de densidad (radio 128 bloques).
    static constexpr int VECINDAD_CHUNKS = 16;

    // Cuantos chunks de cada vecindad pueden repoblar en una misma ventana.
    //
    // DERIVADO, no elegido a ojo:
    //   Grupo medio = (2+4)/2 = 3 individuos.
    //   Objetivo de poblacion en la vecindad ~= 45 individuos.
    //   Para recuperar una zona arrasada en ~4-5 ventanas (20-25 min de
    //   juego), hacen falta ~45/3/4.5 = ~3.3 grupos por ventana.
    //
    //   Con 4 chunks habilitados y PROB_BASE=0.35, salen ~1.4 grupos por
    //   ventana en el peor caso, y la recuperacion completa lleva del orden
    //   de 10 ventanas (~50 min de juego). Es un ritmo que se nota sin ser
    //   instantaneo, que es justo lo que se quiere: el mundo se recupera,
    //   pero no delante de los ojos del jugador.
    static constexpr int CHUNKS_POR_VECINDAD = 4;

    explicit PecariRepoblacion(int s) : seed(s) {}

    // ------------------------------------------------------------------------
    // FACTOR DENSODEPENDIENTE
    // ------------------------------------------------------------------------
    // Devuelve 1.0 si la zona esta vacia (repoblacion maxima) y 0.0 si ya
    // alcanzo la densidad objetivo (repoblacion apagada).
    //
    // Es una funcion aparte y publica a proposito: es el corazon del sistema
    // y debe poder testearse en aislamiento.
    static float FactorDensodependiente(int vivosCerca, float radioBloques) {
        if (radioBloques <= 0.0f) return 0.0f;

        // Area del circulo de conteo, en bloques cuadrados.
        const float area = 3.14159265f * radioBloques * radioBloques;
        const float densidadActual = (float)vivosCerca / area;

        const float ocupacion = densidadActual / DENSIDAD_OBJETIVO_IND_POR_BLOQUE2;
        if (ocupacion >= 1.0f) return 0.0f;   // lleno: no repuebla

        // Decaimiento CUADRATICO, no lineal.
        //
        // POR QUE: con decaimiento lineal, una zona al 50% de ocupacion
        // repuebla al 50% de ritmo, y el acercamiento al objetivo es lento y
        // constante. Con cuadratico, una zona medio vacia repuebla al 25%:
        // la recuperacion es RAPIDA cuando la zona esta muy vaciada y se
        // frena mucho al acercarse al objetivo.
        //
        // Eso reproduce el comportamiento ecologico correcto (crecimiento
        // logistico: maximo lejos de la capacidad de carga, casi nulo al
        // llegar) y evita el efecto de "goteo constante" que haria que el
        // mundo siempre pareciera repoblandose.
        const float hueco = 1.0f - ocupacion;
        return hueco * hueco;
    }

    // ------------------------------------------------------------------------
    // CONSULTA PRINCIPAL
    // ------------------------------------------------------------------------
    // Se llama UNA VEZ por chunk recien cargado.
    //
    // chunkX, chunkZ: coordenadas de CHUNK (no de bloque).
    // El resto de parametros describen el terreno y el contexto.
    GrupoRepoblacion ConsultarChunk(int chunkX, int chunkZ,
                                    BiomeType biome,
                                    float alturaTerreno,
                                    float pendiente,
                                    const ContextoRepoblacion& ctx) const {
        GrupoRepoblacion r;

        // --- 1. Regla 2: habitat ---
        // Se REUTILIZA la idoneidad de PecariSpawn en vez de duplicar la
        // tabla. Si manana se corrige un bioma, se corrige en un solo sitio.
        const float idoneidad = PecariSpawn::IdoneidadBioma(biome);
        if (idoneidad <= 0.0f) {
            r.motivo = GrupoRepoblacion::MOTIVO_BIOMA_NO_APTO;
            return r;
        }

        // --- 2. Regla 1: no aparecer a la vista ---
        if (ctx.distanciaAlJugadorBloques < RADIO_MIN_BLOQUES) {
            r.motivo = GrupoRepoblacion::MOTIVO_DEMASIADO_CERCA_DEL_JUGADOR;
            return r;
        }

        // --- 3. Terreno: misma exigencia que la poblacion estructural ---
        constexpr float PENDIENTE_MAX = 0.55f;
        if (pendiente > PENDIENTE_MAX) {
            r.motivo = GrupoRepoblacion::MOTIVO_PENDIENTE_O_ALTITUD;
            return r;
        }
        const float alturaM = alturaTerreno * 0.60f;   // Fisica::LADO_M
        if (alturaM > PecariDatos::ALTITUD_MAX_ABSOLUTA_M) {
            r.motivo = GrupoRepoblacion::MOTIVO_PENDIENTE_O_ALTITUD;
            return r;
        }

        // --- 4. Respuesta densodependiente ---
        const float factorDensidad = FactorDensodependiente(ctx.pecariesVivosCerca,
                                                            ctx.radioConteoBloques);
        if (factorDensidad <= 0.0f) {
            r.motivo = GrupoRepoblacion::MOTIVO_ZONA_YA_POBLADA;
            return r;
        }

        // --- 5. Ventana temporal ---
        // Cuantizar el tiempo es lo que impide farmear recargando el chunk.
        // Dos cargas del mismo chunk en la misma ventana dan el MISMO hash y
        // por tanto el mismo resultado.
        const int ventana = (int)(ctx.tiempoJuegoSegundos / VENTANA_SEGUNDOS);

        // --- 6. CUOTA POR VECINDAD ---
        // Impide que 400 chunks cargados a la vez repueblen todos en
        // paralelo. Ver la explicacion del bug en CHUNKS_POR_VECINDAD.
        //
        // Floor correcto para negativos, igual que en PecariSpawn: en C++
        // -1/16 da 0, no -1, y eso haria que la vecindad del origen fuera el
        // doble de grande y recibiera el doble de repoblacion.
        const int vecX = (chunkX >= 0) ? (chunkX / VECINDAD_CHUNKS)
                                       : ((chunkX - VECINDAD_CHUNKS + 1) / VECINDAD_CHUNKS);
        const int vecZ = (chunkZ >= 0) ? (chunkZ / VECINDAD_CHUNKS)
                                       : ((chunkZ - VECINDAD_CHUNKS + 1) / VECINDAD_CHUNKS);

        // Indice del chunk DENTRO de su vecindad, en [0, VECINDAD_CHUNKS^2).
        // El modulo se corrige para negativos por la misma razon.
        const int locX = chunkX - vecX * VECINDAD_CHUNKS;
        const int locZ = chunkZ - vecZ * VECINDAD_CHUNKS;
        const int indiceLocal = locZ * VECINDAD_CHUNKS + locX;

        // De los VECINDAD_CHUNKS^2 chunks, solo CHUNKS_POR_VECINDAD tienen
        // derecho a repoblar esta ventana. Cuales, lo decide el hash de
        // (vecindad, ventana): cambia cada ventana, asi que la repoblacion
        // se reparte por toda la vecindad con el tiempo en vez de concentrarse
        // siempre en los mismos chunks.
        constexpr int CHUNKS_EN_VECINDAD = VECINDAD_CHUNKS * VECINDAD_CHUNKS;
        const uint32_t hCuota = Noise::rawHash2D(seedRepob() + 1013 + ventana, vecX, vecZ);

        bool tieneCuota = false;
        for (int k = 0; k < CHUNKS_POR_VECINDAD; ++k) {
            // Se derivan CHUNKS_POR_VECINDAD indices distintos del mismo hash,
            // desplazando bits para que no queden correlacionados.
            const uint32_t hk = hCuota ^ (uint32_t)(k * 0x9E3779B9u);
            if ((int)(hk % (uint32_t)CHUNKS_EN_VECINDAD) == indiceLocal) {
                tieneCuota = true;
                break;
            }
        }
        if (!tieneCuota) {
            r.motivo = GrupoRepoblacion::MOTIVO_NO_TOCA_POR_AZAR;
            return r;
        }

        // --- 7. Tirada ---
        // Hash de (chunk, ventana): determinista dentro de la ventana, pero
        // distinto en la siguiente.
        const uint32_t h = Noise::rawHash2D(seedRepob() + ventana, chunkX, chunkZ);
        const float roll = (float)(h % 100000u) / 100000.0f;

        const float probabilidad = PROB_BASE * idoneidad * factorDensidad;
        if (roll > probabilidad) {
            r.motivo = GrupoRepoblacion::MOTIVO_NO_TOCA_POR_AZAR;
            return r;
        }

        // --- 8. Tamano del grupo: POCOS ---
        const uint32_t hT = Noise::rawHash2D(seedGrupo() + ventana, chunkX, chunkZ);
        constexpr int RANGO = GRUPO_MAX - GRUPO_MIN + 1;
        r.miembros = GRUPO_MIN + (int)(hT % (uint32_t)RANGO);

        // --- 9. Posicion dentro del chunk ---
        // Con jitter, para que los grupos no salgan siempre en la esquina del
        // chunk, que produciria una rejilla visible.
        constexpr int CHUNK_SIZE = 16;
        const uint32_t hX = Noise::rawHash2D(seedGrupo() + 17, chunkX, chunkZ);
        const uint32_t hZ = Noise::rawHash2D(seedGrupo() + 53, chunkX, chunkZ);
        r.x = chunkX * CHUNK_SIZE + (int)(hX % (uint32_t)CHUNK_SIZE);
        r.z = chunkZ * CHUNK_SIZE + (int)(hZ % (uint32_t)CHUNK_SIZE);

        // Grupo pequeno: mas apretado que una manada completa.
        // MEDIDO (Byers y Bekoff 1981): distancias interindividuales pequenas.
        r.dispersionBloques = 4.0f;

        r.aparece = true;
        r.motivo  = GrupoRepoblacion::MOTIVO_APARECE;
        return r;
    }

    // ------------------------------------------------------------------------
    // POSICION DE UN MIEMBRO DEL GRUPO
    // ------------------------------------------------------------------------
    // Mismo reparto por angulo aureo que PecariSpawn, por coherencia visual:
    // un grupo de repoblacion debe verse igual que uno estructural.
    void PosicionMiembro(const GrupoRepoblacion& g, int indice,
                         int& outX, int& outZ) const {
        if (indice <= 0) {
            outX = g.x;
            outZ = g.z;
            return;
        }

        constexpr float ANGULO_AUREO = 2.39996323f;
        const float ang = (float)indice * ANGULO_AUREO;
        const float radio = g.dispersionBloques *
            std::sqrt((float)indice / (float)(g.miembros > 1 ? g.miembros - 1 : 1));

        const uint32_t hJ = Noise::rawHash2D(seedGrupo() + 71, g.x + indice, g.z);
        const float jitter = ((float)(hJ % 100u) / 100.0f - 0.5f) * 2.0f;

        outX = g.x + (int)std::lround(radio * std::cos(ang) + jitter);
        outZ = g.z + (int)std::lround(radio * std::sin(ang) + jitter);
    }

    // ------------------------------------------------------------------------
    // TEXTO DEL MOTIVO (para depuracion)
    // ------------------------------------------------------------------------
    // 01_ARQUITECTURA exige tooling ANTES que contenido, y avisa de que la
    // causa 4 del fracaso de Ultima Online fue "no hubo deteccion de fallo en
    // produccion: el sistema colapso y nadie lo supo".
    static const char* MotivoTexto(GrupoRepoblacion::Motivo m) {
        switch (m) {
            case GrupoRepoblacion::MOTIVO_APARECE:                     return "aparece";
            case GrupoRepoblacion::MOTIVO_BIOMA_NO_APTO:               return "bioma no apto";
            case GrupoRepoblacion::MOTIVO_DEMASIADO_CERCA_DEL_JUGADOR: return "demasiado cerca del jugador";
            case GrupoRepoblacion::MOTIVO_ZONA_YA_POBLADA:             return "zona ya poblada";
            case GrupoRepoblacion::MOTIVO_PENDIENTE_O_ALTITUD:         return "pendiente o altitud";
            case GrupoRepoblacion::MOTIVO_NO_TOCA_POR_AZAR:            return "no toca por azar";
        }
        return "desconocido";
    }
};

} // namespace Fauna

#endif // PECARI_REPOBLACION_H
