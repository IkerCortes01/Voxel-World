#ifndef PECARI_MUNDO_H
#define PECARI_MUNDO_H

#include <vector>
#include <unordered_set>
#include <cstdint>
#include <cmath>
#include "PecariSpawn.h"
#include "PecariRepoblacion.h"
#include "PecariEntidad.h"
#include "PecariCuerpo.h"
#include "AnimalMallaCache.h"

// ============================================================================
// EL REGISTRO DE PECARIES DEL MUNDO
// ============================================================================
// RESPONSABILIDAD UNICA: mantener la poblacion viva de pecaries y ponerla al
// dia cada frame. Es la pieza que une los tres archivos anteriores con el
// motor:
//
//   PecariSpawn       dice DONDE hay manadas estructurales
//   PecariRepoblacion dice DONDE nacen grupos de rescate
//   PecariEntidad     dice COMO se mueve y como se dibuja cada individuo
//   PecariMundo       <-- ESTE: los junta y lleva la contabilidad
//
// ----------------------------------------------------------------------------
// POR QUE UNA CLASE Y NO FUNCIONES SUELTAS
// ----------------------------------------------------------------------------
// Los tres archivos anteriores son funciones puras a proposito: se testean sin
// motor. Pero la POBLACION es estado con vida propia, y alguien tiene que
// guardarlo.
//
// Se mantiene la frontera: esta clase guarda estado, pero sigue sin conocer
// World, ni OpenGL, ni chunks. El motor le dice "se cargo este chunk" y ella
// responde. Es la misma inversion de dependencias que PlayerTypes.h aplica a
// la fisica del jugador.
//
// ----------------------------------------------------------------------------
// EVITAR DUPLICADOS: EL PROBLEMA REAL
// ----------------------------------------------------------------------------
// Un chunk puede cargarse MUCHAS veces: el jugador se aleja, el chunk se
// descarga, vuelve y se recarga. Si cada carga poblara otra vez, la manada se
// duplicaria en cada viaje de ida y vuelta.
//
// Por eso se lleva un registro de chunks YA POBLADOS. Es memoria a cambio de
// correccion, y es poca: un par de enteros por chunk visitado.
//
// (El sistema de guardado real deberia persistir esto. Hoy vive en memoria, y
// se dice en las limitaciones.)
// ============================================================================

namespace Fauna {

// ----------------------------------------------------------------------------
// CLAVE DE CHUNK
// ----------------------------------------------------------------------------
struct ClaveChunk {
    int x, z;
    bool operator==(const ClaveChunk& o) const { return x == o.x && z == o.z; }
};

struct HashClaveChunk {
    size_t operator()(const ClaveChunk& c) const {
        // Hash de dos enteros con dispersion decente. Los numeros son primos
        // grandes, el mismo criterio que usa el motor en su hash de columnas.
        uint64_t h = (uint64_t)(uint32_t)c.x * 374761393ull;
        h ^= (uint64_t)(uint32_t)c.z * 668265263ull;
        h ^= h >> 29; h *= 1274126177ull; h ^= h >> 32;
        return (size_t)h;
    }
};

// ----------------------------------------------------------------------------
// LO QUE EL MOTOR TIENE QUE CONTESTAR
// ----------------------------------------------------------------------------
// La interfaz minima de consulta al mundo, calcada del patron IWorldQuery de
// src/player/PlayerTypes.h. Solo cuatro preguntas, y ninguna obliga a la IA a
// conocer chunks, VBOs ni el sistema de guardado.
class IPecariMundo {
public:
    virtual ~IPecariMundo() = default;

    // Altura del suelo en esa columna, en bloques. Para posar al animal.
    virtual float alturaSuelo(int x, int z) const = 0;

    // Bioma de la columna. Decide si la especie vive ahi.
    virtual BiomeType biomaEn(int x, int z) const = 0;

    // Pendiente 0..1. Un ungulado no vive en una pared.
    virtual float pendienteEn(int x, int z) const = 0;

    // Un bloque solido bloquea el paso.
    virtual bool esSolido(int x, int y, int z) const = 0;
};

// ============================================================================
// EL REGISTRO
// ============================================================================
class MundoPecaries {
private:
    PecariSpawn       spawn;
    PecariRepoblacion repoblacion;

    std::vector<PecariAgente> pecaries;
    std::unordered_set<ClaveChunk, HashClaveChunk> chunksPoblados;

    int siguienteId = 0;
    int siguienteIdManada = 0;

    // Reutilizados entre llamadas para no reservar memoria cada frame.
    // La actualizacion corre cada frame sobre todos los agentes: reservar y
    // liberar ahi seria el tipo de coste que 01_ARQUITECTURA pide evitar.
    mutable std::vector<const PecariAgente*> bufVecinos;
    mutable std::vector<int> manadasVistas;

public:
    // Tope de seguridad. 01_ARQUITECTURA lo exige explicitamente como una de
    // las cinco reglas derivadas del fracaso de Ultima Online:
    //   "PONER LIMITES DUROS como red de seguridad: minimos y maximos
    //    poblacionales por region. Un ecosistema extinto no es mas realista
    //    que uno con limites."
    //
    // Aqui el limite es global y protege el frame rate, no la ecologia: con
    // ~2400 individuos/km2 y varios km2 cargados, sin tope el numero crece
    // sin control.
    static constexpr size_t MAX_PECARIES = 4096;

    // Radio en el que se cuentan vivos para la densodependencia.
    static constexpr float RADIO_CONTEO = 128.0f;

    explicit MundoPecaries(int seed)
        : spawn(seed), repoblacion(seed) {}

    const std::vector<PecariAgente>& todos() const { return pecaries; }
    size_t cuantos() const { return pecaries.size(); }

    // ------------------------------------------------------------------------
    // SOLTAR UN PECARI A MANO (huevo de spawn del creativo)
    // ------------------------------------------------------------------------
    // Las otras dos vias de aparicion --la manada estructural de PecariSpawn y
    // el grupo de repoblacion-- deciden ELLAS donde nace cada animal, a partir
    // del bioma y del ruido. Aqui manda el jugador: pone el punto y sale.
    //
    // POR QUE ES UNA FUNCION APARTE Y NO SE REUSA instanciarManada():
    // aquella pide un ManadaSpawn, que es la respuesta de una consulta
    // procedural a unas coordenadas concretas. Fabricar uno falso para poder
    // llamarla seria mentirle al sistema de spawn sobre lo que hay en ese
    // punto del mundo. Esta ruta es honesta: no pretende ser natural.
    //
    // Devuelve el id del animal creado, o -1 si no cabe.
    int soltarUno(float x, float z, EtapaPecari etapa,
                  const IPecariMundo& mundo, int idManadaForzado = -1) {
        if (pecaries.size() >= MAX_PECARIES) return -1;

        const int px = (int)std::floor(x);
        const int pz = (int)std::floor(z);

        PecariAgente p;
        p.id = siguienteId++;
        // Sin manada propia si no se pide una: un animal suelto es un animal
        // suelto, y el codigo de cohesion ya sabe tratar a los solitarios.
        p.idManada = (idManadaForzado >= 0) ? idManadaForzado : siguienteIdManada++;
        p.x = x;
        p.z = z;
        p.y = mundo.alturaSuelo(px, pz) + 1.0f;
        p.semilla = (uint32_t)(p.id * 2654435761u) | 1u;
        p.audacia = AudaciaDeSemilla(p.semilla);
        // El ritmo (0-50) sale de la misma semilla, con otro mezclador: ver
        // RitmoDeSemilla. Es lo que hace que unos vayan delante y otros se
        // rezaguen, siempre los mismos.
        p.ritmo   = RitmoDeSemilla(p.semilla);
        // Etapa ANTES que vida: VidaMaximaMedios() la lee.
        p.etapa = etapa;
        p.vidaMedios = VidaMaximaMedios(p.etapa);
        // La escala la aplica ConstruirCuerpoPecari desde p.etapa. Ponerla
        // tambien aqui la aplicaria dos veces.
        p.escala = 1.0f;
        p.objetivoX = p.x;
        p.objetivoZ = p.z;

        pecaries.push_back(p);
        return p.id;
    }

    // Suelta una manada entera en un punto, con su reparto de edades normal.
    //
    // Usa el MISMO etapaSegunIndice() que las manadas naturales, asi que la
    // estructura de edades es la de siempre: mayoria de adultos, y con
    // cualquier tamano normal salen tambien un bebe y un adolescente.
    //
    // Devuelve cuantos se pusieron de verdad (puede ser menos que 'miembros'
    // si se llega al tope).
    int soltarManada(float x, float z, int miembros, const IPecariMundo& mundo) {
        if (miembros < 1) miembros = 1;

        const int idManada = siguienteIdManada++;
        int puestos = 0;

        for (int i = 0; i < miembros; ++i) {
            if (pecaries.size() >= MAX_PECARIES) break;

            // Reparto en espiral alrededor del punto: se separan lo justo
            // para no nacer unos dentro de otros. La cohesion de manada los
            // junta sola en cuanto empiezan a moverse.
            const float ang = 2.39996f * (float)i;          // angulo aureo
            const float rad = 0.9f * std::sqrt((float)i);   // ~1 bloque entre vecinos
            const float mx = x + std::cos(ang) * rad;
            const float mz = z + std::sin(ang) * rad;

            if (soltarUno(mx, mz, etapaSegunIndice(i, miembros),
                          mundo, idManada) >= 0) {
                ++puestos;
            }
        }
        return puestos;
    }

    // ------------------------------------------------------------------------
    // CUANTOS VIVOS HAY CERCA
    // ------------------------------------------------------------------------
    // Entrada de la respuesta densodependiente de la repoblacion.
    int vivosCerca(float x, float z, float radio) const {
        const float r2 = radio * radio;
        int n = 0;
        for (const PecariAgente& p : pecaries) {
            const float dx = p.x - x;
            const float dz = p.z - z;
            if (dx*dx + dz*dz <= r2) ++n;
        }
        return n;
    }

    // ------------------------------------------------------------------------
    // SE HA CARGADO UN CHUNK
    // ------------------------------------------------------------------------
    // El motor llama a esto una vez por chunk cargado. Aqui se juntan las dos
    // fuentes de poblacion:
    //
    //   1. La ESTRUCTURAL de PecariSpawn: las manadas que "viven ahi" y que
    //      forman parte del mundo desde que se genero.
    //   2. La DINAMICA de PecariRepoblacion: grupos de rescate que llegan
    //      despues, solo si la zona esta vaciada.
    //
    // Devuelve cuantos individuos se anadieron.
    int alCargarChunk(int chunkX, int chunkZ,
                      const IPecariMundo& mundo,
                      float distanciaJugadorBloques,
                      double tiempoJuegoSegundos) {
        constexpr int CHUNK_SIZE = 16;

        // --- Antiduplicado ---
        // Sin esto, ir y volver duplicaria la manada en cada viaje.
        const ClaveChunk clave{chunkX, chunkZ};
        const bool yaVisitado = (chunksPoblados.find(clave) != chunksPoblados.end());

        int anadidos = 0;

        // --- 1. POBLACION ESTRUCTURAL ---
        // Solo la primera vez que se carga el chunk: es la poblacion que
        // "siempre estuvo ahi". Si se repitiera en cada carga, seria una
        // fuente infinita.
        if (!yaVisitado) {
            for (int lx = 0; lx < CHUNK_SIZE; ++lx) {
                for (int lz = 0; lz < CHUNK_SIZE; ++lz) {
                    const int wx = chunkX * CHUNK_SIZE + lx;
                    const int wz = chunkZ * CHUNK_SIZE + lz;

                    const BiomeType bioma = mundo.biomaEn(wx, wz);
                    const float altura = mundo.alturaSuelo(wx, wz);
                    const float pend   = mundo.pendienteEn(wx, wz);

                    const ManadaSpawn m = spawn.ConsultarManada(wx, wz, bioma, altura, pend);
                    if (!m.existe) continue;

                    anadidos += instanciarManada(m, mundo);
                }
            }
            chunksPoblados.insert(clave);
        }

        // --- 2. REPOBLACION DINAMICA ---
        // Esta SI se evalua en cada carga: es su razon de ser. La proteccion
        // contra el farmeo no es el antiduplicado sino la ventana temporal y
        // la cuota por vecindad, que viven dentro de PecariRepoblacion.
        {
            const int centroX = chunkX * CHUNK_SIZE + CHUNK_SIZE/2;
            const int centroZ = chunkZ * CHUNK_SIZE + CHUNK_SIZE/2;

            ContextoRepoblacion ctx;
            ctx.pecariesVivosCerca = vivosCerca((float)centroX, (float)centroZ, RADIO_CONTEO);
            ctx.radioConteoBloques = RADIO_CONTEO;
            ctx.distanciaAlJugadorBloques = distanciaJugadorBloques;
            ctx.tiempoJuegoSegundos = tiempoJuegoSegundos;

            const GrupoRepoblacion g = repoblacion.ConsultarChunk(
                chunkX, chunkZ,
                mundo.biomaEn(centroX, centroZ),
                mundo.alturaSuelo(centroX, centroZ),
                mundo.pendienteEn(centroX, centroZ),
                ctx);

            if (g.aparece) anadidos += instanciarGrupo(g, mundo);
        }

        return anadidos;
    }

    // ------------------------------------------------------------------------
    // ACTUALIZAR TODA LA POBLACION
    // ------------------------------------------------------------------------
    // Un paso de simulacion. dt en segundos.
    //
    // NOTA DE RENDIMIENTO HONESTA: esto actualiza a TODOS los agentes cada
    // frame, sin LOD y sin time-slicing. 01_ARQUITECTURA exige ambas cosas y
    // aqui NO estan. Con pocos cientos de agentes va sobrado; con miles, no.
    // Se declara en las limitaciones en vez de fingir que esta resuelto.
    void actualizar(float dt, const IPecariMundo& mundo) {
        if (pecaries.empty()) return;

        // Centros de manada: se calculan UNA vez por manada, no por individuo.
        // Sin esto seria O(n^2) por frame.
        manadasVistas.clear();
        for (const PecariAgente& p : pecaries) {
            bool visto = false;
            for (int id : manadasVistas) if (id == p.idManada) { visto = true; break; }
            if (!visto) manadasVistas.push_back(p.idManada);
        }

        for (int idManada : manadasVistas) {
            const CentroManada centro = CalcularCentro(pecaries, idManada);

            for (size_t i = 0; i < pecaries.size(); ++i) {
                if (pecaries[i].idManada != idManada) continue;

                BuscarVecinos(pecaries, i, bufVecinos);
                ActualizarPecari(pecaries[i], centro, bufVecinos, dt);

                // ⭐ MOVER COMPROBANDO EL MUNDO.
                //
                // ActualizarPecari solo deja la INTENCION (vx, vz); quien
                // mueve de verdad es esto, que si tiene el mundo delante y
                // puede negarse a atravesar una pared.
                moverConColision(pecaries[i], mundo, dt);

                // Posar sobre el terreno. Sin esto el animal caminaria a
                // altura constante y atravesaria colinas.
                posarEnSuelo(pecaries[i], mundo, dt);

                // Y sondear el suelo bajo CADA PATA, para que en una cuesta no
                // queden todas a la misma altura.
                muestrearSueloPatas(pecaries[i], mundo, dt);
            }
        }

        // Resolver penetraciones al final: contra bloques y entre animales.
        // Va DESPUES de mover a todos, no dentro del bucle, para que el
        // resultado no dependa del orden en que se actualizaron.
        resolverColisiones(mundo);
    }

    // ------------------------------------------------------------------------
    // RETIRAR LOS QUE ESTAN MUY LEJOS
    // ------------------------------------------------------------------------
    // Sin esto la poblacion crece sin limite segun el jugador explora.
    //
    // ADVERTENCIA DE DISENO: esto DESCARTA el estado del animal, no lo guarda.
    // 01_ARQUITECTURA pide LOD_3 ("simulacion estadistica: el estado NO se
    // pierde, se recalcula al volver") y esto no es eso: es un descarte puro.
    //
    // La consecuencia visible: al volver a una zona, los pecaries no son los
    // mismos individuos. Como la generacion estructural es determinista, la
    // MANADA reaparece en el mismo sitio, asi que el jugador no lo nota; pero
    // no es lo que el archivo de arquitectura pide, y conviene no confundirlo.
    void descargarLejos(float jugadorX, float jugadorZ, float radioBloques) {
        const float r2 = radioBloques * radioBloques;
        size_t escritura = 0;
        for (size_t i = 0; i < pecaries.size(); ++i) {
            const float dx = pecaries[i].x - jugadorX;
            const float dz = pecaries[i].z - jugadorZ;
            if (dx*dx + dz*dz <= r2) {
                if (escritura != i) pecaries[escritura] = pecaries[i];
                ++escritura;
            }
        }
        pecaries.resize(escritura);
    }

    // ------------------------------------------------------------------------
    // GEOMETRIA PARA DIBUJAR
    // ------------------------------------------------------------------------
    // Rellena `salida` con las cajas de todos los pecaries visibles.
    // El motor solo tiene que recorrerlas y pintar cubos.
    // ------------------------------------------------------------------------
    // INSTANCIA A DIBUJAR
    // ------------------------------------------------------------------------
    // Lo MINIMO que el renderer necesita saber de un animal. Es el
    // "PecariRenderSnapshot" del punto 40.
    //
    // 60 bytes por animal vivo. Cien pecaries son 6 KB, no cien mallas.
    // La geometria NO viaja aqui: viaja el puntero a la malla compartida.
    struct InstanciaDibujo {
        const MallaAnimal* malla;   // COMPARTIDA, no una copia

        float x, y, z;              // posicion en bloques
        float orientacion;          // radianes

        float giroCabezaY;          // la cabeza gira aparte del cuerpo
        float giroCabezaX;

        float escala;
        float erizado;              // 0..1, cresta de alarma

        uint32_t semillaColor;      // variacion individual SIN duplicar malla
        int   lod;
        float fundido;              // 0..1 para la transicion entre LOD
        float distancia;            // para ordenar de lejos a cerca

        // --- ARTICULACION ---
        // Lo que hace falta para doblar la malla por sus huesos.
        //
        // Los pesos son COMPARTIDOS igual que la malla: van juntos en la misma
        // entrada de cache, asi que si uno es valido el otro tambien.
        const std::vector<PesoVertice>* pesos;

        float fasePaso;             // ciclo de marcha, radianes
        float rapidez;              // bloques/s: modula la amplitud del paso
        int   etapa;                // 0..4: el esqueleto cambia con la edad

        // Si false, se dibuja la malla en reposo sin deformar. Es lo que hace
        // que un animal lejano no cueste nada: a 30 bloques nadie ve doblarse
        // un codo.
        bool  articulado;

        // Suelo bajo cada pata, en BLOQUES y relativo al centro. Lo usa
        // AplicarTerreno para que el animal se adapte a la cuesta en vez de
        // quedarse horizontal con dos patas en el aire.
        float sueloDI, sueloDD, sueloTI, sueloTD;
    };

    // ------------------------------------------------------------------------
    // PREPARAR LA LISTA DE DIBUJO
    // ------------------------------------------------------------------------
    // Aplica culling y seleccion de LOD, y devuelve solo lo que hay que
    // dibujar. Es el paso "PecariSimulation -> PecariRenderSnapshot".
    //
    // La CPU no toca geometria aqui: solo decide QUE malla usar y DONDE
    // ponerla.
    void prepararDibujo(std::vector<InstanciaDibujo>& salida,
                        CacheMallas& cache,
                        float camaraX, float camaraZ,
                        float dirCamX, float dirCamZ,
                        float distanciaMaxima) const {
        salida.clear();

        for (const PecariAgente& p : pecaries) {
            const float dx = p.x - camaraX;
            const float dz = p.z - camaraZ;

            // --- CULLING POR DISTANCIA ---
            if (!Culling::visiblePorDistancia(dx, dz, distanciaMaxima)) continue;

            // --- CULLING POR ANGULO ---
            // Lo que queda detras de la camara no se dibuja.
            if (!Culling::visiblePorAngulo(dx, dz, dirCamX, dirCamZ)) continue;

            const float dist = std::sqrt(dx*dx + dz*dz);

            // --- SELECCION DE LOD ---
            const int lod = LOD::seleccionar(dist, p.lodActual);

            const MallaAnimal* malla =
                cache.obtener(EspecieAnimal::PECARI_COLLAR, (int)p.etapa, lod);
            if (!malla) continue;

            InstanciaDibujo d;
            d.malla        = malla;
            d.x = p.x; d.y = p.y; d.z = p.z;
            d.orientacion  = p.orientacion;
            d.giroCabezaY  = p.giroCabezaY;
            d.giroCabezaX  = p.giroCabezaX;
            d.escala       = p.escala;
            d.erizado      = p.erizado;
            d.semillaColor = p.semilla;
            d.lod          = lod;
            d.fundido      = LOD::factorTransicion(dist, lod);
            d.distancia    = dist;

            // --- ARTICULACION: solo de cerca ---
            //
            // Deformar la malla cuesta una transformacion por vertice y por
            // frame (715 en LOD 0). Se paga solo donde se ve: a partir de
            // LOD 2 el animal ocupa unos pocos pixeles y nadie distingue si
            // dobla el codo.
            //
            // Asi el coste se autolimita: una manada lejana no cuesta mas que
            // antes, y solo los pocos animales cercanos pagan el skinning.
            d.fasePaso    = p.fasePaso;
            d.rapidez     = std::sqrt(p.vx*p.vx + p.vz*p.vz);
            d.etapa       = (int)p.etapa;
            d.articulado  = (lod <= 1);
            d.pesos       = d.articulado
                          ? cache.pesosDe(EspecieAnimal::PECARI_COLLAR,
                                          (int)p.etapa, lod)
                          : nullptr;

            d.sueloDI = p.sueloPataDI;
            d.sueloDD = p.sueloPataDD;
            d.sueloTI = p.sueloPataTI;
            d.sueloTD = p.sueloPataTD;

            salida.push_back(d);
        }
    }

    // Actualiza el LOD guardado en cada agente, para que la histeresis tenga
    // memoria entre frames. Se llama despues de prepararDibujo.
    void confirmarLOD(const std::vector<InstanciaDibujo>& lista) {
        for (const InstanciaDibujo& d : lista) {
            for (PecariAgente& p : pecaries) {
                if (std::fabs(p.x - d.x) < 1e-4f &&
                    std::fabs(p.z - d.z) < 1e-4f) {
                    p.lodActual = d.lod;
                    break;
                }
            }
        }
    }

    void construirGeometria(std::vector<PiezaCuerpo>& salida,
                            float camaraX, float camaraZ,
                            float distanciaMaxima) const {
        salida.clear();
        const float d2max = distanciaMaxima * distanciaMaxima;

        std::vector<PiezaCuerpo> cuerpo;
        for (const PecariAgente& p : pecaries) {
            const float dx = p.x - camaraX;
            const float dz = p.z - camaraZ;
            if (dx*dx + dz*dz > d2max) continue;

            PosePecari pose;
            pose.x = p.x; pose.y = p.y; pose.z = p.z;
            pose.orientacionCuerpo = p.orientacion;
            pose.giroCabezaY = p.giroCabezaY;
            pose.giroCabezaX = p.giroCabezaX;
            pose.fasePaso = p.fasePaso;
            pose.rapidez  = std::sqrt(p.vx*p.vx + p.vz*p.vz);
            pose.erizado  = p.erizado;
            pose.parpadeo = p.parpadeo;
            pose.escala   = p.escala;
            pose.semilla  = p.semilla;
            pose.etapa    = (int)p.etapa;
            pose.tiempoVivo = p.tiempoVivo;

            ConstruirCuerpoPecari(pose, cuerpo);
            salida.insert(salida.end(), cuerpo.begin(), cuerpo.end());
        }
    }

private:
    // Coloca una manada estructural en el mundo.
    int instanciarManada(const ManadaSpawn& m, const IPecariMundo& mundo) {
        if (pecaries.size() >= MAX_PECARIES) return 0;

        const int idManada = siguienteIdManada++;
        int puestos = 0;

        for (int i = 0; i < m.miembros; ++i) {
            if (pecaries.size() >= MAX_PECARIES) break;

            int px = 0, pz = 0;
            spawn.PosicionMiembro(m, i, px, pz);

            PecariAgente p;
            p.id = siguienteId++;
            p.idManada = idManada;
            p.x = (float)px + 0.5f;
            p.z = (float)pz + 0.5f;
            p.y = mundo.alturaSuelo(px, pz) + 1.0f;
            p.semilla = (uint32_t)(p.id * 2654435761u) | 1u;

            // El caracter y la salud salen de la semilla y de la etapa, asi
            // que el mismo animal es siempre igual de atrevido y aguanta lo
            // mismo. No hace falta guardarlos.
            p.audacia    = AudaciaDeSemilla(p.semilla);
            p.ritmo      = RitmoDeSemilla(p.semilla);
            // ⭐ LA ETAPA VA ANTES QUE LA VIDA.
            //
            // Estaba al reves, y el fallo era silencioso: p.etapa tiene valor
            // por defecto (ADULTO), asi que VidaMaximaMedios() leia ese valor
            // y TODOS los pecaries nacian con vida de adulto -- un neonato
            // aguantaba tantos golpes como un adulto hecho.
            //
            // No daba error ni valor absurdo, solo el numero equivocado, que
            // es la clase de bug que sobrevive anos.
            p.etapa = etapaSegunIndice(i, m.miembros);
            p.vidaMedios = VidaMaximaMedios(p.etapa);
            // La escala por etapa la aplica ConstruirCuerpoPecari a partir de
            // p.etapa. Ponerla tambien aqui la aplicaria DOS veces y las
            // crias saldrian diminutas.
            p.escala = 1.0f;
            p.objetivoX = p.x;
            p.objetivoZ = p.z;

            pecaries.push_back(p);
            ++puestos;
        }
        return puestos;
    }

    // Coloca un grupo de repoblacion.
    int instanciarGrupo(const GrupoRepoblacion& g, const IPecariMundo& mundo) {
        if (pecaries.size() >= MAX_PECARIES) return 0;

        const int idManada = siguienteIdManada++;
        int puestos = 0;

        for (int i = 0; i < g.miembros; ++i) {
            if (pecaries.size() >= MAX_PECARIES) break;

            int px = 0, pz = 0;
            repoblacion.PosicionMiembro(g, i, px, pz);

            PecariAgente p;
            p.id = siguienteId++;
            p.idManada = idManada;
            p.x = (float)px + 0.5f;
            p.z = (float)pz + 0.5f;
            p.y = mundo.alturaSuelo(px, pz) + 1.0f;
            p.semilla = (uint32_t)(p.id * 2654435761u) | 1u;

            // El caracter y la salud salen de la semilla y de la etapa, asi
            // que el mismo animal es siempre igual de atrevido y aguanta lo
            // mismo. No hace falta guardarlos.
            p.audacia    = AudaciaDeSemilla(p.semilla);
            p.ritmo      = RitmoDeSemilla(p.semilla);
            // Los grupos de repoblacion son dispersores: adultos y subadultos,
            // no crias. MEDIDO que la dispersion esta sesgada a machos y que
            // el 37-38% de ellos cambian de manada.
            //
            // La etapa va ANTES que la vida, por el mismo motivo que en
            // instanciarManada: VidaMaximaMedios() la lee.
            p.etapa = (i == 0) ? EtapaPecari::ADULTO : EtapaPecari::SUBADULTO;
            p.vidaMedios = VidaMaximaMedios(p.etapa);
            p.escala = 1.0f;   // ver nota en instanciarManada
            p.objetivoX = p.x;
            p.objetivoZ = p.z;

            pecaries.push_back(p);
            ++puestos;
        }
        return puestos;
    }

    // Reparto de edades dentro de una manada.
    //
    // MEDIDO: las manadas son "grupos mixtos de machos y hembras adultos,
    // juveniles y subadultos, de varias clases de edad", con proporcion de
    // sexos 1:1 y filopatria femenina.
    //
    // Las proporciones exactas por clase de edad son AUSENTE en la literatura
    // (declarado en 10_PECARI_BIOLOGIA.lagunas_declaradas), asi que este
    // reparto es ESTIMADO: mayoria de adultos, algunos jovenes.
    static EtapaPecari etapaSegunIndice(int i, int total) {
        if (total <= 2) return EtapaPecari::ADULTO;

        const float frac = (float)i / (float)total;

        // Reparto pensado para que en CUALQUIER manada de tamano normal (5-15)
        // haya al menos un bebe y un adolescente, que es lo que se pidio.
        //
        // Con el reparto anterior (adultos hasta 0.55, neonatos solo desde
        // 0.90) una manada de 6 podia salir sin ninguna cria.
        //
        // La estructura sigue siendo la MEDIDA: "grupos mixtos de machos y
        // hembras adultos, juveniles y subadultos, de varias clases de edad",
        // con mayoria de adultos.
        if (frac < 0.45f) return EtapaPecari::ADULTO;      // ~45% adultos
        if (frac < 0.62f) return EtapaPecari::SENESCENTE;  // ~17% viejos
        if (frac < 0.78f) return EtapaPecari::SUBADULTO;   // ~16% adolescentes
        if (frac < 0.90f) return EtapaPecari::JUVENIL;     // ~12% juveniles
        return EtapaPecari::NEONATO;                       // ~10% bebes
    }

public:
    // ------------------------------------------------------------------------
    // COLISION CON EL JUGADOR
    // ------------------------------------------------------------------------
    // Devuelve true si el jugador esta dentro de algun pecari, y rellena el
    // empuje que hay que aplicarle para sacarlo.
    //
    // El animal NO es un fantasma: tiene cuerpo, y el jugador choca con el.
    // La caja sale de la anatomia MEDIDA (altura a la cruz, longitud
    // cabeza-cuerpo), no de un numero inventado, y se escala por la etapa
    // vital: un bebe estorba menos que un adulto.
    //
    // Se usa una caja ALINEADA para el choque aunque el cuerpo se dibuje
    // rotado. Es lo que hace el motor con el jugador y con todo lo demas:
    // una AABB de colision es mucho mas barata y la diferencia no se nota en
    // un animal casi tan ancho como largo.
    bool empujeSobreJugador(float px, float py, float pz,
                            float radioJugador, float alturaJugador,
                            float& outX, float& outZ) const {
        outX = 0.0f; outZ = 0.0f;
        bool hayChoque = false;

        for (const PecariAgente& a : pecaries) {
            const HitboxPecari h = HitboxDe(a);

            // Solapamiento vertical: si el jugador salta por encima, no choca.
            if (py + alturaJugador < a.y) continue;
            if (py > a.y + h.alto) continue;

            // Se aproxima el cuerpo por un circulo del radio mayor, para que
            // el empuje sea estable con independencia de hacia donde mire el
            // animal. Con una caja rotada el jugador saldria disparado al
            // girar el pecari.
            const float radioAnimal = (h.semiLargo > h.semiAncho)
                                    ? h.semiLargo : h.semiAncho;
            const float rSuma = radioAnimal + radioJugador;

            const float dx = px - a.x;
            const float dz = pz - a.z;
            const float d2 = dx*dx + dz*dz;
            if (d2 >= rSuma * rSuma) continue;

            const float d = std::sqrt(d2);
            if (d < 1e-4f) {
                // Exactamente encima: se empuja en una direccion cualquiera
                // pero determinista, para que no vibre.
                outX += rSuma;
                hayChoque = true;
                continue;
            }

            const float penetracion = rSuma - d;
            outX += (dx / d) * penetracion;
            outZ += (dz / d) * penetracion;
            hayChoque = true;
        }
        return hayChoque;
    }

    // ------------------------------------------------------------------------
    // EL JUGADOR EMPUJA A LOS ANIMALES
    // ------------------------------------------------------------------------
    // El complemento de empujeSobreJugador: ahi el animal aparta al jugador,
    // aqui el jugador aparta al animal. Los dos a la vez es lo que hace que el
    // contacto se sienta como dos cuerpos y no como una pared.
    //
    // EL REPARTO NO ES 50/50, y es deliberado:
    //   El jugador pesa mas o menos como un pecari adulto (18.7 kg MEDIDO...
    //   bueno, bastante mas), asi que en un choque el animal cede MAS que el
    //   jugador. Pero un bebe de 0.5 kg tiene que salir practicamente
    //   despedido, mientras que un adulto apenas se mueve.
    //
    // Por eso el empuje se pondera por la MASA del animal, derivada de su
    // etapa vital a partir de los pesos MEDIDOS (0.5 kg al nacer, 18.7 kg
    // adulto). No es un numero de diseno: sale de la biologia.
    //
    // Devuelve cuantos animales fueron empujados.
    int jugadorEmpuja(float px, float py, float pz,
                      float radioJugador, float alturaJugador,
                      float fuerza = 1.0f) {
        int empujados = 0;

        for (PecariAgente& a : pecaries) {
            const HitboxPecari h = HitboxDe(a);

            // Solapamiento vertical.
            if (py + alturaJugador < a.y) continue;
            if (py > a.y + h.alto) continue;

            const float radioAnimal = (h.semiLargo > h.semiAncho)
                                    ? h.semiLargo : h.semiAncho;
            const float rSuma = radioAnimal + radioJugador;

            const float dx = a.x - px;
            const float dz = a.z - pz;
            const float d2 = dx*dx + dz*dz;
            if (d2 >= rSuma * rSuma) continue;

            const float d = std::sqrt(d2);
            if (d < 1e-4f) continue;   // exactamente encima: lo resuelve el otro lado

            // --- Cuanto cede el animal, segun su masa ---
            //
            // Masas MEDIDAS: 0.5 kg al nacer, 18.7 kg adulto. Las intermedias
            // salen de la escala al cubo, que es como va la masa con la
            // longitud.
            float masaKg = 18.7f;
            switch (a.etapa) {
                case EtapaPecari::NEONATO:    masaKg =  0.5f; break;   // MEDIDO
                case EtapaPecari::JUVENIL:    masaKg =  2.6f; break;   // DERIVADO
                case EtapaPecari::SUBADULTO:  masaKg =  8.9f; break;   // DERIVADO
                case EtapaPecari::ADULTO:     masaKg = 18.7f; break;   // MEDIDO
                case EtapaPecari::SENESCENTE: masaKg = 17.1f; break;   // DERIVADO
            }

            // Un jugador ronda los 70 kg. Cuanto mas ligero el animal, mas
            // cede: un bebe sale casi despedido, un adulto apenas se aparta.
            constexpr float MASA_JUGADOR = 70.0f;
            const float cesion = MASA_JUGADOR / (MASA_JUGADOR + masaKg);

            const float penetracion = rSuma - d;
            const float desplaza = penetracion * cesion * fuerza;

            a.x += (dx / d) * desplaza;
            a.z += (dz / d) * desplaza;

            // El animal tambien se ALTERA al ser empujado: eriza la cresta.
            // Es la respuesta de alarma MEDIDA de la especie, y hace que el
            // empujon tenga una consecuencia visible en vez de ser silencioso.
            a.erizado = 1.0f;

            ++empujados;
        }
        return empujados;
    }

    // ------------------------------------------------------------------------
    // SEPARACION ENTRE ANIMALES Y CONTRA BLOQUES
    // ------------------------------------------------------------------------
    // Resuelve las penetraciones despues de mover. Se llama al final del paso.
    void resolverColisiones(const IPecariMundo& mundo) {
        // --- Contra bloques solidos ---
        //
        // Si el animal ha acabado dentro de terreno, se le devuelve al borde.
        // Sin esto, un pecari puede meterse en una pared al seguir a su manada.
        for (PecariAgente& a : pecaries) {
            const HitboxPecari h = HitboxDe(a);
            const float radio = (h.semiLargo > h.semiAncho) ? h.semiLargo : h.semiAncho;

            // Se comprueban las cuatro direcciones cardinales al nivel del
            // pecho del animal, que es donde chocaria de verdad.
            const int yPecho = (int)std::floor(a.y + h.alto * 0.5f);

            const struct { float dx, dz; } dirs[4] = {
                { 1.0f, 0.0f}, {-1.0f, 0.0f}, {0.0f,  1.0f}, {0.0f, -1.0f}
            };

            for (const auto& d : dirs) {
                const int bx = (int)std::floor(a.x + d.dx * radio);
                const int bz = (int)std::floor(a.z + d.dz * radio);
                if (!mundo.esSolido(bx, yPecho, bz)) continue;

                // Empujar hacia el lado contrario, lo justo para salir.
                const float borde = (d.dx != 0.0f)
                    ? (d.dx > 0 ? (float)bx - radio : (float)(bx + 1) + radio)
                    : 0.0f;
                if (d.dx > 0.0f && a.x + radio > (float)bx)      a.x = borde;
                else if (d.dx < 0.0f && a.x - radio < (float)(bx+1)) a.x = borde;

                const float bordeZ = (d.dz != 0.0f)
                    ? (d.dz > 0 ? (float)bz - radio : (float)(bz + 1) + radio)
                    : 0.0f;
                if (d.dz > 0.0f && a.z + radio > (float)bz)      a.z = bordeZ;
                else if (d.dz < 0.0f && a.z - radio < (float)(bz+1)) a.z = bordeZ;
            }
        }

        // --- Entre animales ---
        //
        // La separacion de boids ya los mantiene apartados en condiciones
        // normales, pero al apretarse contra un obstaculo pueden solaparse.
        // Esto lo resuelve geometricamente, sin fuerzas.
        for (size_t i = 0; i < pecaries.size(); ++i) {
            for (size_t j = i + 1; j < pecaries.size(); ++j) {
                PecariAgente& a = pecaries[i];
                PecariAgente& b = pecaries[j];

                const HitboxPecari ha = HitboxDe(a);
                const HitboxPecari hb = HitboxDe(b);
                const float ra = (ha.semiLargo > ha.semiAncho) ? ha.semiLargo : ha.semiAncho;
                const float rb = (hb.semiLargo > hb.semiAncho) ? hb.semiLargo : hb.semiAncho;
                const float rSuma = (ra + rb) * 0.80f;   // se admite algo de roce

                const float dx = b.x - a.x;
                const float dz = b.z - a.z;
                const float d2 = dx*dx + dz*dz;
                if (d2 >= rSuma * rSuma || d2 < 1e-6f) continue;

                const float d = std::sqrt(d2);
                const float mitad = (rSuma - d) * 0.5f;
                const float ux = dx / d, uz = dz / d;

                a.x -= ux * mitad;  a.z -= uz * mitad;
                b.x += ux * mitad;  b.z += uz * mitad;
            }
        }
    }

    // ========================================================================
    // ⭐ LE HAN PEGADO
    // ========================================================================
    // Devuelve true si el animal ha MUERTO con este golpe.
    //
    // ------------------------------------------------------------------------
    // LA DECISION: HUIR O PLANTAR CARA
    // ------------------------------------------------------------------------
    // Esto no es una moneda al aire. La condicion sale de 11_PECARI_IA.json,
    // que para el comportamiento DEFENDER dice literalmente:
    //
    //     "Solo si hay crias cerca Y la huida esta bloqueada"
    //     "El mobbing coordinado NO esta documentado en esta especie (si en
    //      T. pecari). Implementar con cautela y marcar como extrapolacion."
    //
    // Y el dato MEDIDO va en la misma direccion: ante una amenaza, lo que se
    // observo en Barro Colorado es que la manada AUMENTA LA COHESION y la
    // vigilancia. Se apiñan. No cargan en grupo.
    //
    // Por eso la respuesta por defecto es HUIR, y encarar es la excepcion:
    //
    //   1. HAY UNA CRIA CERCA           -> defiende. Es la condicion del diseño.
    //   2. ESTA ACORRALADO              -> defiende. Un animal sin salida pelea;
    //                                     es la otra mitad de la condicion.
    //   3. ES MUY AUDAZ (top ~6%)       -> encara. La audacia es un rasgo
    //                                     MEDIDO en esta especie, con
    //                                     consecuencias fisiologicas
    //                                     demostradas. Que el mas atrevido
    //                                     plante cara es lo que ese rasgo
    //                                     significa.
    //   4. EN CUALQUIER OTRO CASO       -> huye.
    //
    // Las crias NUNCA defienden: un neonato de 0.5 kg no encara a nadie. Huyen
    // siempre.
    //
    // ------------------------------------------------------------------------
    // LO QUE ES EXTRAPOLACION, DICHO CLARO
    // ------------------------------------------------------------------------
    // Que los companeros ACUDAN a defender es la parte no documentada. Se
    // implementa porque se pidio, y se acota a la condicion del diseño: solo
    // acuden cuando el agredido esta defendiendo crias o acorralado. Un golpe
    // a un adulto suelto en campo abierto NO levanta a la manada: la dispersa.
    bool recibirDano(int idPecari, int danoMedios,
                     float desdeX, float desdeZ,
                     const IPecariMundo& mundo) {
        PecariAgente* victima = nullptr;
        for (PecariAgente& p : pecaries)
            if (p.id == idPecari) { victima = &p; break; }
        if (victima == nullptr) return false;

        // Vida sin estrenar: se le pone la que le toca por su etapa.
        if (victima->vidaMedios <= 0)
            victima->vidaMedios = VidaMaximaMedios(victima->etapa);

        victima->vidaMedios -= danoMedios;

        if (victima->vidaMedios <= 0) {
            const int idManadaMuerto = victima->idManada;
            const float mx = victima->x, mz = victima->z;

            // Retirar al muerto.
            for (size_t i = 0; i < pecaries.size(); ++i) {
                if (pecaries[i].id != idPecari) continue;
                pecaries[i] = pecaries.back();
                pecaries.pop_back();
                break;
            }

            // Ver morir a uno de los tuyos alarma al grupo. Aqui SIEMPRE se
            // huye: no hay a quien defender, y el agresor ha demostrado que
            // mata.
            alarmarManada(idManadaMuerto, mx, mz, desdeX, desdeZ,
                          /*defender=*/false);
            return true;
        }

        // --- Sigue vivo: decide que hacer ---
        victima->hayAmenaza = true;
        victima->amenazaX = desdeX;
        victima->amenazaZ = desdeZ;
        victima->erizado  = 1.0f;

        const bool esCria = (victima->etapa == EtapaPecari::NEONATO ||
                             victima->etapa == EtapaPecari::JUVENIL);

        const bool defiende =
            !esCria && (hayCriaCerca(*victima) ||
                        estaAcorralado(*victima, mundo) ||
                        victima->audacia >= PecariAmenaza::AUDACIA_ENCARA);

        if (defiende) {
            victima->conducta = ConductaPecari::DEFENSA;
            victima->tiempoConducta = PecariAmenaza::DURACION_DEFENSA;
        } else {
            victima->conducta = ConductaPecari::HUIDA;
            victima->tiempoConducta = PecariAmenaza::DURACION_HUIDA;
        }

        // Y avisa a los suyos.
        alarmarManada(victima->idManada, victima->x, victima->z,
                      desdeX, desdeZ, defiende);
        return false;
    }

    // ¿Hay una cria a tiro? Es la condicion que el diseño exige para defender.
    bool hayCriaCerca(const PecariAgente& p) const {
        const float r2 = PecariAmenaza::RADIO_CRIA * PecariAmenaza::RADIO_CRIA;
        for (const PecariAgente& o : pecaries) {
            if (o.id == p.id) continue;
            if (o.idManada != p.idManada) continue;
            if (o.etapa != EtapaPecari::NEONATO &&
                o.etapa != EtapaPecari::JUVENIL) continue;
            const float dx = o.x - p.x, dz = o.z - p.z;
            if (dx*dx + dz*dz <= r2) return true;
        }
        return false;
    }

    // ¿Esta acorralado? La otra mitad de la condicion del diseño: "la huida
    // esta bloqueada".
    //
    // Se mira si tiene salida por las cuatro direcciones a un par de cuerpos
    // de distancia. Si esta metido en un callejon o contra una pared, pelea:
    // es lo que hace cualquier animal sin escapatoria.
    static bool estaAcorralado(const PecariAgente& p, const IPecariMundo& mundo) {
        const HitboxPecari h = HitboxDe(p);
        const int y = (int)std::floor(p.y + h.alto * 0.5f);
        const float alcance = 2.5f;

        int salidas = 0;
        const struct { float dx, dz; } dirs[4] = {
            { 1.0f, 0.0f}, {-1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, -1.0f}
        };
        for (const auto& d : dirs) {
            const int bx = (int)std::floor(p.x + d.dx * alcance);
            const int bz = (int)std::floor(p.z + d.dz * alcance);
            if (!mundo.esSolido(bx, y, bz)) ++salidas;
        }
        // Con una sola salida o ninguna se considera acorralado.
        return salidas <= 1;
    }

    // ------------------------------------------------------------------------
    // EL AVISO A LA MANADA
    // ------------------------------------------------------------------------
    // No hay vocalizaciones documentadas en esta especie -- 11_PECARI_IA.json
    // lo dice y pide que, si se implementan, se declaren como eleccion de
    // diseño y no como dato. Asi que esto no es "un grito": es que los
    // companeros VEN lo que le pasa a uno de los suyos, que es lo unico que
    // la evidencia respalda (la amenaza aumenta la vigilancia del grupo).
    //
    // De ahi el radio corto: se enteran los que lo tienen cerca.
    void alarmarManada(int idManada, float origenX, float origenZ,
                       float amenazaX, float amenazaZ, bool defender) {
        if (idManada < 0) return;   // un solitario no tiene a quien avisar
        const float r2 = PecariAmenaza::RADIO_ALARMA * PecariAmenaza::RADIO_ALARMA;

        for (PecariAgente& o : pecaries) {
            if (o.idManada != idManada) continue;

            const float dx = o.x - origenX, dz = o.z - origenZ;
            if (dx*dx + dz*dz > r2) continue;

            // Al que ya esta defendiendo no se le rebaja a alerta.
            if (o.conducta == ConductaPecari::DEFENSA) continue;

            o.hayAmenaza = true;
            o.amenazaX = amenazaX;
            o.amenazaZ = amenazaZ;
            o.erizado  = 1.0f;

            const bool esCria = (o.etapa == EtapaPecari::NEONATO ||
                                 o.etapa == EtapaPecari::JUVENIL);

            if (defender && !esCria) {
                // ⚠️ ESTA ES LA PARTE EXTRAPOLADA (ver la nota de recibirDano).
                // Los adultos acuden. Las crias no: se van, que es lo que
                // tiene sentido y ademas es el motivo de que los adultos se
                // queden.
                o.conducta = ConductaPecari::DEFENSA;
                o.tiempoConducta = PecariAmenaza::DURACION_DEFENSA;
            } else if (esCria) {
                // Las crias siempre huyen.
                o.conducta = ConductaPecari::HUIDA;
                o.tiempoConducta = PecariAmenaza::DURACION_HUIDA;
            } else if (o.conducta == ConductaPecari::CALMA) {
                // El resto se pone en guardia. Solo los mas timidos salen
                // corriendo sin haber visto el peligro ellos mismos.
                if (o.audacia < 0.35f) {
                    o.conducta = ConductaPecari::HUIDA;
                    o.tiempoConducta = PecariAmenaza::DURACION_HUIDA;
                } else {
                    o.conducta = ConductaPecari::ALERTA;
                    o.tiempoConducta = PecariAmenaza::DURACION_ALERTA;
                }
            }
        }
    }

    // ------------------------------------------------------------------------
    // ¿A QUE ANIMAL APUNTA EL JUGADOR?
    // ------------------------------------------------------------------------
    // Devuelve el id del pecari que el rayo atraviesa antes, o -1.
    //
    // El motor solo sabia lanzar rayos contra BLOQUES (ver raycastBlock), asi
    // que sin esto no habia forma de golpear a un animal.
    //
    // Se resuelve con avance por pasos y no analiticamente: son pocos
    // animales, el alcance es corto y asi el codigo cabe de un vistazo. Cada
    // paso comprueba la caja del animal, no una esfera, para que apuntar al
    // morro o al lomo cuente igual.
    int pecariApuntado(float ojoX, float ojoY, float ojoZ,
                       float dirX, float dirY, float dirZ,
                       float alcance) const {
        constexpr float PASO = 0.15f;
        const int nPasos = (int)(alcance / PASO);

        for (int i = 1; i <= nPasos; ++i) {
            const float t = i * PASO;
            const float px = ojoX + dirX * t;
            const float py = ojoY + dirY * t;
            const float pz = ojoZ + dirZ * t;

            for (const PecariAgente& a : pecaries) {
                const HitboxPecari h = HitboxDe(a);
                const float r = (h.semiLargo > h.semiAncho) ? h.semiLargo
                                                            : h.semiAncho;
                if (py < a.y || py > a.y + h.alto) continue;
                const float dx = px - a.x, dz = pz - a.z;
                if (dx*dx + dz*dz <= r*r) return a.id;
            }
        }
        return -1;
    }

    // ------------------------------------------------------------------------
    // PUERTA PARA LOS TESTS
    // ------------------------------------------------------------------------
    // moverConColision es privada porque es un detalle del bucle de
    // actualizacion, pero es LA funcion que impide atravesar bloques y tiene
    // que poder probarse sola: montar una manada entera solo para comprobar
    // que un animal no cruza una pared enterraria el fallo entre ruido.
    //
    // Es un reenvio de una linea, sin logica propia, asi que no puede
    // desviarse de lo que hace el juego.
    static void moverConColisionTest(PecariAgente& p, const IPecariMundo& mundo,
                                     float dt) {
        moverConColision(p, mundo, dt);
    }

    // Los de abajo montan escenarios concretos para los tests de conducta.
    //
    // Hacen falta porque la composicion de la manada la decide un hash: sin
    // poder fijar "este es adulto, este es cria y estan juntos", no se puede
    // comprobar que un adulto defiende a una cria -- habria que rezar para que
    // la manada generada tuviera esa forma.
    const PecariAgente* buscarParaTest(int id) const {
        for (const PecariAgente& p : pecaries)
            if (p.id == id) return &p;
        return nullptr;
    }

    void forzarParaTest(int id, EtapaPecari etapa, float audacia) {
        for (PecariAgente& p : pecaries) {
            if (p.id != id) continue;
            p.etapa      = etapa;
            p.escala     = EscalaDeEtapa(etapa);
            p.audacia    = audacia;
            p.vidaMedios = VidaMaximaMedios(etapa);
            return;
        }
    }

    // Aparta a uno de todos los demas, para probar el caso "solo".
    void aislarParaTest(int id) {
        for (PecariAgente& p : pecaries) {
            if (p.id != id) continue;
            p.x = 10000.0f; p.z = 10000.0f;
            return;
        }
    }

    // Pone a `id` justo al lado de `junto`.
    void juntarParaTest(int id, int junto) {
        float jx = 0.0f, jz = 0.0f;
        bool hallado = false;
        for (const PecariAgente& p : pecaries)
            if (p.id == junto) { jx = p.x; jz = p.z; hallado = true; break; }
        if (!hallado) return;

        for (PecariAgente& p : pecaries) {
            if (p.id != id) continue;
            p.x = jx + 1.0f; p.z = jz;
            return;
        }
    }

private:
    // Posa al animal sobre el terreno.
    //
    // Deliberadamente simple: se consulta la altura del suelo bajo el animal
    // y se le pone encima, con una interpolacion para que no de tirones al
    // cambiar de bloque. NO es fisica de caida: es seguimiento de terreno.
    //
    // Un ungulado que camina no cae en caida libre; lo que necesita es no
    // atravesar la colina. La fisica completa seria necesaria si hubiera
    // acantilados o saltos, y eso es trabajo posterior.
    // ========================================================================
    // ⭐ MOVER SIN ATRAVESAR NADA
    // ========================================================================
    // EL BUG QUE ESTO CIERRA: el pecari cruzaba paredes, casas y arboles como
    // si no existieran. La causa eran dos cosas a la vez:
    //
    //   1. La posicion se integraba a pelo (`p.x += p.vx*dt`) sin preguntar si
    //      el destino estaba libre.
    //   2. Y la unica correccion que habia era POSTERIOR: dejaba entrar al
    //      animal en el bloque y luego lo empujaba fuera, mirando solo cuatro
    //      puntos a la altura del pecho. Un bloque a la altura de las patas o
    //      de la cabeza no lo veia nadie.
    //
    // COMO SE HACE AHORA: lo mismo que hace el jugador (ver
    // src/player/CollisionSystem.h) -- probar el movimiento EJE POR EJE y
    // quedarse solo con el que cabe.
    //
    // Probar los ejes por separado es lo que da el DESLIZAMIENTO gratis: si un
    // animal camina en diagonal contra una pared recta, el eje que choca se
    // descarta y el otro sigue, asi que resbala a lo largo del muro en vez de
    // quedarse clavado. Moverlo en diagonal de una vez lo dejaria atascado en
    // cada esquina.
    static void moverConColision(PecariAgente& p, const IPecariMundo& mundo,
                                 float dt) {
        const float dx = p.vx * dt;
        const float dz = p.vz * dt;
        if (dx == 0.0f && dz == 0.0f) return;

        // --- EJE X ---
        if (dx != 0.0f) {
            if (cabeEn(p, mundo, p.x + dx, p.z)) {
                p.x += dx;
            } else {
                // Choca: se para en ese eje. Anular la velocidad ademas de la
                // posicion evita que el animal siga empujando contra el muro
                // frame tras frame y que el ciclo de patas corra en el sitio.
                p.vx = 0.0f;
            }
        }

        // --- EJE Z ---
        if (dz != 0.0f) {
            if (cabeEn(p, mundo, p.x, p.z + dz)) {
                p.z += dz;
            } else {
                p.vz = 0.0f;
            }
        }
    }

    // ¿Cabe el animal con su centro en (nx, nz)?
    //
    // Recorre TODOS los voxeles que toca su caja, no cuatro puntos sueltos.
    // Esa es la diferencia entre "no atraviesa paredes gruesas" y "no
    // atraviesa nada": con cuatro sondas, un bloque en diagonal o a otra
    // altura se cuela por el hueco entre ellas.
    static bool cabeEn(const PecariAgente& p, const IPecariMundo& mundo,
                       float nx, float nz) {
        const HitboxPecari h = HitboxDe(p);

        // El cuerpo se aproxima por su lado MAYOR, igual que hace el resto del
        // sistema (el empuje y la separacion entre animales usan el mismo
        // radio). Asi la colision con el mundo y la de entre animales hablan
        // de la misma caja.
        const float r = (h.semiLargo > h.semiAncho) ? h.semiLargo : h.semiAncho;

        // ⭐ EL MARGEN DE ESCALON.
        //
        // No se comprueba desde los pies sino un poco mas arriba: un ungulado
        // sube un bordillo sin pensarlo. Sin esto, cualquier loncha de terreno
        // de 1/8 seria un muro infranqueable y las manadas se quedarian
        // encerradas en el primer desnivel.
        //
        // 0.35 bloques son 21 cm: por debajo de la altura a la cruz medida
        // (0.44 m) y bastante por encima de una capa fina de tierra.
        constexpr float ESCALON = 0.35f;

        const int y0 = (int)std::floor(p.y + ESCALON);
        const int y1 = (int)std::floor(p.y + h.alto);

        const int xa = (int)std::floor(nx - r);
        const int xb = (int)std::floor(nx + r);
        const int za = (int)std::floor(nz - r);
        const int zb = (int)std::floor(nz + r);

        for (int by = y0; by <= y1; ++by)
            for (int bx = xa; bx <= xb; ++bx)
                for (int bz = za; bz <= zb; ++bz)
                    if (mundo.esSolido(bx, by, bz)) return false;

        return true;
    }

    // ========================================================================
    // ⭐ EL PECARI CAE DE VERDAD
    // ========================================================================
    // BUG QUE ESTO CORRIGE: el animal no tenia gravedad. `vy` estaba declarado
    // en PecariAgente pero NO SE USABA: la altura se resolvia interpolando
    // suavemente hacia el suelo con 1 - exp(-12*dt).
    //
    // Eso funciona para un escalon de un bloque --que es para lo que se
    // escribio-- pero es FALSO en cuanto hay altura de verdad: tirar un pecari
    // por un acantilado de 30 bloques lo hacia DESCENDER FLOTANDO, cada vez
    // mas despacio segun se acercaba al fondo, como una pluma. Nunca aceleraba.
    //
    // Ahora hay dos regimenes distintos, y esa es la clave:
    //
    //   DESNIVEL PEQUEÑO (<= PASO_MAXIMO)  -> se sube/baja suave, como antes.
    //       Es andar por terreno irregular: el animal salva un escalon sin
    //       despegarse del suelo. Aqui el suavizado exponencial es lo correcto
    //       y se conserva tal cual.
    //
    //   CAIDA DE VERDAD (mas que eso)      -> gravedad real, con aceleracion.
    //       El animal se despega, `vy` crece, y cae mas rapido cuanto mas
    //       lleva cayendo. Desde cualquier altura.
    //
    // La frontera entre los dos es lo unico que hay que elegir bien: si fuera
    // muy alta, saltarse un muro de 2 m se veria como flotar; si fuera muy
    // baja, cada bache del terreno lanzaria al animal al aire.
    // ========================================================================

    // Gravedad, en bloques/s^2. El motor mide en bloques de 0.60 m, asi que
    // 9.80665 m/s^2 son 16.34 bloques/s^2. Es la MISMA constante que usa la
    // fisica de bloques que caen (ver FisicaCaida.h), para que un pecari y una
    // piedra caigan igual -- que es lo que hacen en la realidad.
    static constexpr float GRAVEDAD = 9.80665f / 0.60f;

    // Velocidad terminal, en bloques/s. Sin tope, una caida larga acumularia
    // velocidad sin limite y el animal atravesaria el suelo en un solo frame
    // (a 30 fps, 50 bloques/s son 1.7 bloques por frame).
    //
    // 55 m/s es la velocidad terminal de un cuerpo humano en caida libre; para
    // un animal de 18.7 kg y menos superficie sale del mismo orden. En bloques:
    // ~90. Se toma algo menos porque a esa velocidad ya no se distingue.
    static constexpr float VEL_TERMINAL = 75.0f;

    // Desnivel que el animal salva sin despegarse, en bloques. Por encima de
    // esto se considera una caida y entra la gravedad.
    //
    // 0.6 bloques = 36 cm reales. Un pecari mide 50 cm a la cruz, asi que es
    // aproximadamente lo que sube sin saltar -- un bordillo, una raiz. Un
    // bloque entero (60 cm) ya es un escalon que hay que subir, no un bache.
    static constexpr float PASO_MAXIMO = 0.6f;

    static void posarEnSuelo(PecariAgente& p, const IPecariMundo& mundo,
                             float dt) {
        const int bx = (int)std::floor(p.x);
        const int bz = (int)std::floor(p.z);
        const float suelo = mundo.alturaSuelo(bx, bz) + 1.0f;

        const float diferencia = suelo - p.y;

        // --------------------------------------------------------------------
        // CAIDA: el suelo esta MUY por debajo
        // --------------------------------------------------------------------
        if (diferencia < -PASO_MAXIMO || !p.enSuelo) {
            p.enSuelo = false;

            // Acelera. Es lo que hace que una caida de 30 bloques se vea como
            // una caida y no como un descenso en paracaidas.
            p.vy -= GRAVEDAD * dt;
            if (p.vy < -VEL_TERMINAL) p.vy = -VEL_TERMINAL;

            p.y += p.vy * dt;

            // ¿Ha tocado suelo en este paso?
            //
            // Se comprueba DESPUES de mover, y contra el suelo de la columna
            // donde ha acabado: cayendo en diagonal el animal puede cambiar de
            // celda a mitad de la caida.
            const int nx = (int)std::floor(p.x);
            const int nz = (int)std::floor(p.z);
            const float sueloAhora = mundo.alturaSuelo(nx, nz) + 1.0f;

            if (p.y <= sueloAhora) {
                p.y = sueloAhora;
                p.vy = 0.0f;
                p.enSuelo = true;
            }
            return;
        }

        // --------------------------------------------------------------------
        // ANDANDO: terreno irregular, sin despegarse
        // --------------------------------------------------------------------
        // ⭐ EL FACTOR VA CON dt, Y ANTES NO.
        //
        // Esto era `p.y += diferencia * 0.25f`, o sea un 25% POR FRAME. A 30
        // fps un escalon de un bloque se subia en ~0.32 s; a 144 fps, en
        // ~0.066 s. La altura del animal dependia del framerate, y en un PC
        // rapido los pecaries pegaban un salto vertical al cruzar cualquier
        // desnivel.
        //
        // 1 - exp(-k*dt) es el mismo suavizado pero medido en SEGUNDOS: con
        // k = 12 el animal cubre el 70% del desnivel en 0.1 s, vaya el juego
        // a los fps que vaya.
        constexpr float K_SUELO = 12.0f;
        const float factor = 1.0f - std::exp(-K_SUELO * dt);

        p.vy = 0.0f;
        if (std::fabs(diferencia) < 0.02f) {
            p.y = suelo;
        } else {
            p.y += diferencia * factor;
        }
        p.enSuelo = true;
    }

    // ------------------------------------------------------------------------
    // SONDEAR EL SUELO BAJO CADA PATA
    // ------------------------------------------------------------------------
    // Cuatro consultas de altura, una por pie, guardadas RELATIVAS al suelo
    // bajo el centro. El esqueleto las usa para inclinar el tronco y ajustar
    // cada pata (ver AplicarTerreno en PecariEsqueleto.h).
    //
    // POR QUE SE SUAVIZAN. La altura del terreno es una funcion ESCALONADA: al
    // cruzar la frontera de un bloque salta de golpe. Sin suavizar, la pata
    // daria un tiron seco cada vez que el animal avanza 60 cm -- peor que no
    // adaptarse. Con el mismo 1-exp(-k*dt) que usa posarEnSuelo, el pie sube y
    // baja de forma continua, y ademas queda independiente del framerate.
    //
    // La k es MAS BAJA que la del cuerpo (8 contra 12) a proposito: la pata
    // debe ir ligeramente por detras del cuerpo, no adelantarse. Es lo que da
    // la sensacion de que el pie BUSCA el suelo en vez de teletransportarse.
    static void muestrearSueloPatas(PecariAgente& p, const IPecariMundo& mundo,
                                    float dt) {
        // En el aire no hay terreno al que adaptarse: las patas van a su
        // postura de reposo. Sin esto, un animal cayendo por un acantilado
        // seguiria estirando las patas hacia un suelo que ya no pisa.
        if (!p.enSuelo) {
            constexpr float K_AIRE = 6.0f;
            const float f = 1.0f - std::exp(-K_AIRE * dt);
            p.sueloPataDI -= p.sueloPataDI * f;
            p.sueloPataDD -= p.sueloPataDD * f;
            p.sueloPataTI -= p.sueloPataTI * f;
            p.sueloPataTD -= p.sueloPataTD * f;
            return;
        }

        // Donde cae cada pie, en el espacio del mundo. Hay que ROTAR los
        // desplazamientos por la orientacion del animal: un pecari mirando al
        // este tiene las patas delanteras al este, no al norte.
        //
        // Las separaciones salen de la anatomia (ver PecariAnatomia.h):
        // ~0.10 m a los lados del eje y ~0.22 m adelante/atras. En bloques de
        // 0.60 m son 0.17 y 0.37.
        constexpr float SEP_X = 0.17f;
        constexpr float SEP_Z = 0.37f;

        const float c = std::cos(p.orientacion);
        const float s = std::sin(p.orientacion);

        const int bx = (int)std::floor(p.x);
        const int bz = (int)std::floor(p.z);
        const float sueloCentro = mundo.alturaSuelo(bx, bz) + 1.0f;

        auto sondear = [&](float dxLocal, float dzLocal) -> float {
            // Rotacion estandar en Y: el mismo convenio que usa el dibujo.
            const float wx = p.x + dxLocal * c + dzLocal * s;
            const float wz = p.z - dxLocal * s + dzLocal * c;
            const float h = mundo.alturaSuelo((int)std::floor(wx),
                                              (int)std::floor(wz)) + 1.0f;
            return h - sueloCentro;
        };

        const float objDI = sondear(-SEP_X,  SEP_Z);
        const float objDD = sondear( SEP_X,  SEP_Z);
        const float objTI = sondear(-SEP_X, -SEP_Z);
        const float objTD = sondear( SEP_X, -SEP_Z);

        constexpr float K_PATA = 8.0f;
        const float f = 1.0f - std::exp(-K_PATA * dt);

        p.sueloPataDI += (objDI - p.sueloPataDI) * f;
        p.sueloPataDD += (objDD - p.sueloPataDD) * f;
        p.sueloPataTI += (objTI - p.sueloPataTI) * f;
        p.sueloPataTD += (objTD - p.sueloPataTD) * f;
    }
};

} // namespace Fauna

#endif // PECARI_MUNDO_H
