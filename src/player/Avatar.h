#pragma once
// ============================================================================
// EL AVATAR: CADA PARTE DEL CUERPO, EL PELO, LAS UNAS Y LOS GENES
// ============================================================================
// PEDIDO: "en Avatar ... una seccion grande donde iran minisecciones: Cabeza
// (tamanos y todo lo que contiene), cuello, hombros, y asi sucesivamente en
// familias de cada parte del cuerpo, de la cabeza a los pies. Pelo y su
// crecimiento, crecimiento de unas, altura, fuerza, edad, genero, e incluso
// una seccion para modificar tus genes y ADN de forma interactiva, realista y
// biologica y geneticamente correcta".
//
// DOS CAPAS, como en una persona:
//
//   GENOMA     lo heredado. Diploide: dos alelos por gen, uno de cada padre.
//              Los genes del X solo tienen una copia en XY. De aqui salen los
//              PIGMENTOS (piel, ojos, pelo), el rizo y el grosor del pelo, la
//              musculatura de base, el sexo y el potencial de altura.
//   AJUSTES    la forma de cada parte: tamanos, grosores, largos. Son
//              deslizadores agrupados por familias, de la cabeza a los pies.
//
// Lo que ve el juego es el FENOTIPO: lo que sale de juntar las dos con la
// edad. Con la edad aparecen canas, y la calvicie si los genes y el sexo la
// permiten; el pelo y las unas CRECEN con los dias del mundo.
//
// LA GENETICA ES LA DE VERDAD, SIMPLIFICADA. Cada gen es uno real, con su
// variante real y su modo de herencia:
//
//   SRY            cromosoma Y. Su presencia decide el sexo (XX / XY).
//   MC1R R151C     cr. 16. Recesivo: dos copias = pelo rojo y pecas; una, algo
//                  de tono rojizo y pecas.
//   HERC2 rs12913832  cr. 15. Regula a OCA2: GG ojos azules, AG intermedios
//                  (verdes, avellana), AA marrones.
//   SLC24A5 A111T  cr. 15. Aditivo: cada alelo Thr111 aclara la piel.
//   SLC45A2 L374F  cr. 5. Aditivo: aclara piel y pelo.
//   IRF4 rs12203592  cr. 6. Aditivo: pelo mas claro y pecas.
//   TYR (OCA1)     cr. 11. Recesivo: dos copias sin funcion = albinismo.
//   EDAR V370A     cr. 2. Aditivo: pelo mas grueso y lacio.
//   TCHH           cr. 1. Aditivo: la variante del rizo.
//   ACTN3 R577X    cr. 11. XX = sin alfa-actinina-3 en la fibra rapida: menos
//                  potencia explosiva. RR, la maxima.
//   MSTN           cr. 2. Miostatina: perderla da mas musculo, mucho mas con
//                  dos copias (hay casos humanos documentados).
//   HMGA2, GDF5, LCORL, ZBTB38  altura poligenica: cada alelo suma unos
//                  milimetros; la altura real depende de miles de loci.
//   AR             cromosoma X. Variante de riesgo de calvicie androgenica:
//                  en XY basta una copia; en XX casi no se expresa.
//   LCT -13910     cr. 2. Dominante: tolerancia a la lactosa en el adulto.
//                  No se ve, pero es un rasgo.
//   ABO            cr. 9. Tres alelos: A y B codominantes, O recesivo.
//
// Este archivo no sabe nada de OpenGL: lo prueban los tests y lo dibujan
// MallaJugador.h, CabezaJugador.h y la pantalla de Avatar en main.cpp.
// ============================================================================

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace Avatar {

// ----------------------------------------------------------------------------
// LOS AJUSTES: deslizadores agrupados por familias, de la cabeza a los pies
// ----------------------------------------------------------------------------
enum Param : int {
    // CABEZA
    CAB_TAMANO, CAB_ANCHO, CAB_LARGO, CAB_FRENTE, CAB_POMULOS, CAB_MANDIBULA, CAB_MENTON,
    // OJOS Y CEJAS
    OJO_TAMANO, OJO_SEPARACION, OJO_ALTURA, CEJA_GROSOR, CEJA_ALTURA,
    // NARIZ
    NARIZ_LARGO, NARIZ_ANCHO, NARIZ_PUENTE,
    // BOCA
    BOCA_ANCHO, LABIOS_GROSOR,
    // OREJAS
    OREJA_TAMANO, OREJA_DESPEGADA,
    // PELO
    PELO_LARGO_CM, PELO_CRECE, PELO_DENSIDAD, PELO_RIZO_EXTRA, BARBA_LARGO_MM,
    // CUELLO
    CUELLO_LARGO, CUELLO_GROSOR,
    // HOMBROS
    HOMBRO_ANCHO, HOMBRO_DELTOIDES,
    // BRAZOS
    BRAZO_GROSOR, BRAZO_BICEPS, BRAZO_ANTEBRAZO,
    // MANOS
    MANO_TAMANO, MANO_DEDOS_GROSOR,
    // UNAS
    UNA_LARGO_MM, UNA_CRECE,
    // TORSO
    TORSO_PECHO, TORSO_BUSTO, TORSO_CINTURA, TORSO_ABDOMEN,
    // CADERA
    CADERA_ANCHO, CADERA_GLUTEOS,
    // PIERNAS
    PIERNA_MUSLO, PIERNA_GEMELO,
    // PIES
    PIE_TAMANO, PIE_ANCHO, PIE_ARCO,
    // CUERPO
    CUERPO_ALTURA_EXTRA, CUERPO_FUERZA, CUERPO_EDAD, CUERPO_GENERO, CUERPO_GRASA,
    // ROPA
    ROPA_CAMISA_TONO, ROPA_CAMISA_CLARO, ROPA_SHORT_TONO, ROPA_SHORT_CLARO,
    NUM_PARAMS
};

struct InfoParam {
    const char* nombre;
    float minimo, maximo, defecto;
    const char* unidad;   // "" = factor sin unidad
};

inline const InfoParam& info(int p) {
    static const InfoParam T[NUM_PARAMS] = {
        { "TAMANO DE CABEZA",   0.85f, 1.15f, 1.0f, "" },
        { "ANCHO DE CARA",      0.85f, 1.15f, 1.0f, "" },
        { "LARGO DE CARA",      0.85f, 1.15f, 1.0f, "" },
        { "FRENTE",             0.80f, 1.20f, 1.0f, "" },
        { "POMULOS",            0.70f, 1.30f, 1.0f, "" },
        { "MANDIBULA",          0.80f, 1.25f, 1.0f, "" },
        { "MENTON",             0.70f, 1.30f, 1.0f, "" },
        { "TAMANO DE OJOS",     0.80f, 1.20f, 1.0f, "" },
        { "SEPARACION OJOS",    0.85f, 1.15f, 1.0f, "" },
        { "ALTURA DE OJOS",     0.90f, 1.10f, 1.0f, "" },
        { "GROSOR DE CEJAS",    0.40f, 1.80f, 1.0f, "" },
        { "ALTURA DE CEJAS",    0.85f, 1.15f, 1.0f, "" },
        { "LARGO DE NARIZ",     0.80f, 1.25f, 1.0f, "" },
        { "ANCHO DE NARIZ",     0.75f, 1.30f, 1.0f, "" },
        { "PUENTE NASAL",       0.70f, 1.30f, 1.0f, "" },
        { "ANCHO DE BOCA",      0.80f, 1.20f, 1.0f, "" },
        { "GROSOR DE LABIOS",   0.60f, 1.50f, 1.0f, "" },
        { "TAMANO DE OREJAS",   0.80f, 1.25f, 1.0f, "" },
        { "OREJAS DESPEGADAS",  0.00f, 1.00f, 0.2f, "" },
        { "LARGO DE PELO",      0.0f, 60.0f, 4.0f, "CM" },
        { "CRECIMIENTO PELO",   0.0f, 3.0f, 1.2f, "CM/MES" },
        { "DENSIDAD DE PELO",   0.2f, 1.2f, 1.0f, "" },
        { "RIZO EXTRA",        -0.5f, 0.5f, 0.0f, "" },
        { "LARGO DE BARBA",     0.0f, 80.0f, 0.0f, "MM" },
        { "LARGO DE CUELLO",    0.80f, 1.25f, 1.0f, "" },
        { "GROSOR DE CUELLO",   0.80f, 1.35f, 1.0f, "" },
        { "ANCHO DE HOMBROS",   0.85f, 1.20f, 1.0f, "" },
        { "DELTOIDES",          0.70f, 1.50f, 1.0f, "" },
        { "GROSOR DE BRAZO",    0.80f, 1.30f, 1.0f, "" },
        { "BICEPS",             0.60f, 1.80f, 1.0f, "" },
        { "ANTEBRAZO",          0.80f, 1.35f, 1.0f, "" },
        { "TAMANO DE MANOS",    0.85f, 1.15f, 1.0f, "" },
        { "GROSOR DE DEDOS",    0.80f, 1.25f, 1.0f, "" },
        { "LARGO DE UNAS",      0.0f, 25.0f, 1.0f, "MM" },
        { "CRECIMIENTO UNAS",   0.0f, 8.0f, 3.5f, "MM/MES" },
        { "ANCHO DE PECHO",     0.85f, 1.20f, 1.0f, "" },
        { "BUSTO",              0.0f, 1.0f, 0.0f, "" },
        { "CINTURA",            0.80f, 1.25f, 1.0f, "" },
        { "ABDOMEN",            0.0f, 1.0f, 0.1f, "" },
        { "ANCHO DE CADERA",    0.85f, 1.25f, 1.0f, "" },
        { "GLUTEOS",            0.60f, 1.60f, 1.0f, "" },
        { "MUSLO",              0.80f, 1.35f, 1.0f, "" },
        { "GEMELOS",            0.70f, 1.50f, 1.0f, "" },
        { "TAMANO DE PIES",     0.85f, 1.15f, 1.0f, "" },
        { "ANCHO DE PIES",      0.85f, 1.20f, 1.0f, "" },
        { "ARCO DEL PIE",       0.0f, 2.0f, 1.0f, "" },
        { "ALTURA (AJUSTE)",   -15.0f, 15.0f, 0.0f, "CM" },
        { "FUERZA",             0.0f, 1.0f, 0.5f, "" },
        { "EDAD",              14.0f, 90.0f, 25.0f, "ANOS" },
        { "GENERO (ASPECTO)",   0.0f, 1.0f, 1.0f, "" },
        { "GRASA CORPORAL",     0.0f, 1.0f, 0.25f, "" },
        { "TONO DE CAMISA",     0.0f, 1.0f, 0.58f, "" },
        { "CLARIDAD CAMISA",    0.1f, 1.0f, 0.55f, "" },
        { "TONO DE SHORT",      0.0f, 1.0f, 0.64f, "" },
        { "CLARIDAD SHORT",     0.05f, 1.0f, 0.25f, "" },
    };
    return T[p < 0 ? 0 : (p >= NUM_PARAMS ? NUM_PARAMS - 1 : p)];
}

struct Familia { const char* nombre; int desde, hasta; };   // [desde, hasta)
constexpr int NUM_FAMILIAS = 16;
inline const Familia& familia(int f) {
    static const Familia F[NUM_FAMILIAS] = {
        { "CABEZA",         CAB_TAMANO,     OJO_TAMANO },
        { "OJOS Y CEJAS",   OJO_TAMANO,     NARIZ_LARGO },
        { "NARIZ",          NARIZ_LARGO,    BOCA_ANCHO },
        { "BOCA",           BOCA_ANCHO,     OREJA_TAMANO },
        { "OREJAS",         OREJA_TAMANO,   PELO_LARGO_CM },
        { "PELO",           PELO_LARGO_CM,  CUELLO_LARGO },
        { "CUELLO",         CUELLO_LARGO,   HOMBRO_ANCHO },
        { "HOMBROS",        HOMBRO_ANCHO,   BRAZO_GROSOR },
        { "BRAZOS",         BRAZO_GROSOR,   MANO_TAMANO },
        { "MANOS",          MANO_TAMANO,    UNA_LARGO_MM },
        { "UNAS",           UNA_LARGO_MM,   TORSO_PECHO },
        { "TORSO",          TORSO_PECHO,    CADERA_ANCHO },
        { "CADERA",         CADERA_ANCHO,   PIERNA_MUSLO },
        { "PIERNAS",        PIERNA_MUSLO,   PIE_TAMANO },
        { "PIES",           PIE_TAMANO,     CUERPO_ALTURA_EXTRA },
        { "CUERPO Y ROPA",  CUERPO_ALTURA_EXTRA, NUM_PARAMS },
    };
    return F[f < 0 ? 0 : (f >= NUM_FAMILIAS ? NUM_FAMILIAS - 1 : f)];
}

// ----------------------------------------------------------------------------
// EL GENOMA
// ----------------------------------------------------------------------------
enum Gen : int {
    G_SRY, G_MC1R, G_HERC2, G_SLC24A5, G_SLC45A2, G_IRF4, G_TYR, G_EDAR, G_TCHH,
    G_ACTN3, G_MSTN, G_HMGA2, G_GDF5, G_LCORL, G_ZBTB38, G_AR, G_LCT, G_ABO,
    NUM_GENES
};

enum Herencia : int { AUTOSOMICO, LIGADO_X, LIGADO_Y };

struct InfoGen {
    const char* nombre;
    const char* cromosoma;
    const char* variante;      // la variante real que se modela
    const char* alelo[3];      // nombres de los alelos (ABO usa los 3)
    int nAlelos;
    Herencia herencia;
    const char* cambio;        // el cambio en el ADN (o "" si no aplica)
    const char* efecto;        // lo que hace, en una linea
};

inline const InfoGen& infoGen(int g) {
    static const InfoGen T[NUM_GENES] = {
        { "SRY", "Y", "PRESENCIA DEL GEN", { "X", "Y", "" }, 2, LIGADO_Y,
          "",
          "CON Y (SRY) SE DESARROLLA COMO VARON" },
        { "MC1R", "16q24.3", "R151C (rs1805007)", { "+", "R151C", "" }, 2, AUTOSOMICO,
          "c.451C>T  CGC>TGC  ARG>CYS",
          "RECESIVO: DOS COPIAS = PELO ROJO Y PECAS" },
        { "HERC2", "15q13.1", "rs12913832", { "A", "G", "" }, 2, AUTOSOMICO,
          "INTRON 86: A>G (APAGA A OCA2 EN EL IRIS)",
          "GG OJOS AZULES, AG VERDES/AVELLANA, AA MARRONES" },
        { "SLC24A5", "15q21.1", "A111T (rs1426654)", { "ALA111", "THR111", "" }, 2, AUTOSOMICO,
          "c.331G>A  GCA>ACA  ALA>THR",
          "ADITIVO: CADA THR111 ACLARA LA PIEL" },
        { "SLC45A2", "5p13.2", "L374F (rs16891982)", { "LEU374", "PHE374", "" }, 2, AUTOSOMICO,
          "MISSENSE: LEU>PHE EN LA PROTEINA MATP",
          "ADITIVO: ACLARA PIEL Y PELO" },
        { "IRF4", "6p25.3", "rs12203592", { "C", "T", "" }, 2, AUTOSOMICO,
          "INTRON 4: C>T (MENOS TIROSINASA)",
          "ADITIVO: PELO MAS CLARO Y PECAS" },
        { "TYR", "11q14.3", "OCA1 (SIN FUNCION)", { "+", "OCA1", "" }, 2, AUTOSOMICO,
          "PERDIDA DE FUNCION DE LA TIROSINASA",
          "RECESIVO: DOS COPIAS = ALBINISMO (SIN MELANINA)" },
        { "EDAR", "2q13", "V370A (rs3827760)", { "VAL370", "ALA370", "" }, 2, AUTOSOMICO,
          "c.1109T>C  GTG>GCG  VAL>ALA",
          "ADITIVO: PELO MAS GRUESO Y LACIO" },
        { "TCHH", "1q21.3", "rs11803731", { "LISO", "RIZO", "" }, 2, AUTOSOMICO,
          "MISSENSE EN LA TRICOHIALINA",
          "ADITIVO: EL PELO SE RIZA" },
        { "ACTN3", "11q13.2", "R577X (rs1815739)", { "R577", "X577", "" }, 2, AUTOSOMICO,
          "c.1729C>T  CGA>TGA  ARG>STOP",
          "XX: SIN ALFA-ACTININA-3, MENOS POTENCIA" },
        { "MSTN", "2q32.2", "PERDIDA DE FUNCION", { "+", "NULO", "" }, 2, AUTOSOMICO,
          "MUTACION QUE CORTA EL EMPALME (MIOSTATINA NULA)",
          "UNA COPIA: MAS MUSCULO. DOS: HIPERTROFIA" },
        { "HMGA2", "12q14.3", "rs1042725", { "T", "C", "" }, 2, AUTOSOMICO,
          "3' UTR: T>C",
          "CADA C SUMA ~4 MM DE ALTURA" },
        { "GDF5", "20q11.22", "rs143384", { "A", "G", "" }, 2, AUTOSOMICO,
          "5' UTR: A>G",
          "CADA G SUMA ~3 MM DE ALTURA" },
        { "LCORL", "4p15.31", "rs16896068", { "A", "G", "" }, 2, AUTOSOMICO,
          "LOCUS DE ALTURA (GWAS)",
          "CADA G SUMA ~3 MM DE ALTURA" },
        { "ZBTB38", "3q23", "rs6440003", { "G", "A", "" }, 2, AUTOSOMICO,
          "LOCUS DE ALTURA (GWAS)",
          "CADA A SUMA ~3 MM DE ALTURA" },
        { "AR", "Xq12", "rs6152 (RIESGO DE ALOPECIA)", { "+", "RIESGO", "" }, 2, LIGADO_X,
          "VARIANTE SINONIMA: MARCA EL HAPLOTIPO DE RIESGO",
          "LIGADO AL X: CALVICIE CON LA EDAD EN VARONES" },
        { "LCT", "2q21.3", "-13910 C>T (rs4988235)", { "C", "T", "" }, 2, AUTOSOMICO,
          "POTENCIADOR EN MCM6: C>T",
          "DOMINANTE: DIGIERE LA LECHE DE ADULTO" },
        { "ABO", "9q34.2", "GRUPO SANGUINEO", { "A", "B", "O" }, 3, AUTOSOMICO,
          "O: DELECION c.261delG (MARCO ROTO)",
          "A Y B CODOMINANTES, O RECESIVO" },
    };
    return T[g < 0 ? 0 : (g >= NUM_GENES ? NUM_GENES - 1 : g)];
}

struct Genoma {
    // a[g][0] del padre, a[g][1] de la madre. Indices de alelo 0.. nAlelos-1.
    // SRY: a[0] siempre X (de la madre), a[1] X o Y (del padre).
    // AR (ligado al X): en XY, a[1] no existe (el Y no lo lleva) y se ignora.
    uint8_t a[NUM_GENES][2] = {};
};

inline bool esXY(const Genoma& g) { return g.a[G_SRY][1] == 1; }

// Cuantas copias del alelo `alelo` hay (0, 1 o 2), respetando el X en XY.
inline int copias(const Genoma& g, int gen, int alelo) {
    const InfoGen& in = infoGen(gen);
    if (in.herencia == LIGADO_X && esXY(g)) return g.a[gen][0] == alelo ? 1 : 0;
    return (g.a[gen][0] == alelo ? 1 : 0) + (g.a[gen][1] == alelo ? 1 : 0);
}

// Los alelos del genotipo, en texto: "G / A", "X / Y", "+ / -" (hemicigoto).
inline std::string genotipo(const Genoma& g, int gen) {
    const InfoGen& in = infoGen(gen);
    std::string s = in.alelo[g.a[gen][0] % in.nAlelos];
    s += " / ";
    if (in.herencia == LIGADO_X && esXY(g)) s += "(Y)";
    else s += in.alelo[g.a[gen][1] % in.nAlelos];
    return s;
}

// Un genoma de partida corriente (un mestizo mexicano promedio, varon).
inline Genoma genomaInicial() {
    Genoma g;
    g.a[G_SRY][0] = 0; g.a[G_SRY][1] = 1;          // XY
    g.a[G_HERC2][0] = 0; g.a[G_HERC2][1] = 0;      // ojos marrones
    g.a[G_SLC24A5][0] = 1; g.a[G_SLC24A5][1] = 0;  // un Thr111
    g.a[G_EDAR][0] = 1;                            // un Ala370
    g.a[G_HMGA2][0] = 1;
    g.a[G_GDF5][1] = 1;
    g.a[G_LCT][0] = 1;                             // tolera la leche
    g.a[G_ABO][0] = 2; g.a[G_ABO][1] = 2;          // O
    return g;
}

// ----------------------------------------------------------------------------
// EL FENOTIPO: lo que resulta de genes + ajustes + edad
// ----------------------------------------------------------------------------
struct Color3 { float r, g, b; };

struct Fenotipo {
    bool varon = true;
    bool albino = false;
    Color3 piel{ 0.86f, 0.67f, 0.53f };
    Color3 iris{ 0.32f, 0.20f, 0.11f };
    Color3 pelo{ 0.12f, 0.08f, 0.06f };
    float pecas = 0.0f;          // 0..1
    float rizo = 0.0f;           // 0 lacio .. 1 muy rizado
    float grosorPelo = 1.0f;
    float canas = 0.0f;          // 0..1
    float calvicie = 0.0f;       // 0..1 (de la coronilla y las entradas)
    float musculo = 1.0f;        // multiplica el volumen muscular
    float potencia = 1.0f;       // fibra rapida (ACTN3)
    float alturaCm = 176.0f;
    bool toleraLactosa = true;
    char grupo[4] = "O";
    // Lo que se lleva puesto y la forma (de los ajustes).
    Color3 camisa{ 0.24f, 0.45f, 0.66f };
    Color3 shortC{ 0.20f, 0.22f, 0.30f };
    float pelo_cm = 4.0f, barba_mm = 0.0f, una_mm = 1.0f;
};

inline Color3 mezcla(Color3 a, Color3 b, float t) {
    return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t };
}
inline float sat01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

// Un color de tela a partir de tono (0..1 por el circulo) y claridad.
inline Color3 tela(float tono, float claro) {
    const float h = (tono - std::floor(tono)) * 6.0f;
    const float x = 1.0f - std::fabs(std::fmod(h, 2.0f) - 1.0f);
    Color3 c{ 0, 0, 0 };
    if (h < 1) c = { 1, x, 0 }; else if (h < 2) c = { x, 1, 0 }; else if (h < 3) c = { 0, 1, x };
    else if (h < 4) c = { 0, x, 1 }; else if (h < 5) c = { x, 0, 1 }; else c = { 1, 0, x };
    // Saturacion moderada: es algodon, no plastico.
    const float s = 0.55f;
    return { (1.0f - s + s * c.r) * claro * 0.9f, (1.0f - s + s * c.g) * claro * 0.9f,
             (1.0f - s + s * c.b) * claro * 0.9f };
}

struct Datos {
    float v[NUM_PARAMS];
    Genoma genes;
    double ultimoDia = -1.0;     // el dia del mundo en que se aplico el crecimiento

    Datos() { reiniciar(); }
    void reiniciar() {
        for (int i = 0; i < NUM_PARAMS; ++i) v[i] = info(i).defecto;
        genes = genomaInicial();
    }
    float operator[](int p) const { return v[p]; }
    void poner(int p, float x) {
        const InfoParam& in = info(p);
        v[p] = x < in.minimo ? in.minimo : (x > in.maximo ? in.maximo : x);
    }

    // Cambia el alelo `lado` (0 padre, 1 madre) del gen al siguiente. Lo que
    // no es posible en biologia no se permite: el SRY solo puede venir del
    // padre, y la madre siempre da un X.
    void ciclarAlelo(int gen, int lado) {
        const InfoGen& in = infoGen(gen);
        if (gen == G_SRY) {
            if (lado == 1) genes.a[G_SRY][1] ^= 1;
            genes.a[G_SRY][0] = 0;
            return;
        }
        if (in.herencia == LIGADO_X && esXY(genes) && lado == 1) return;   // no hay segundo X
        genes.a[gen][lado] = (uint8_t)((genes.a[gen][lado] + 1) % in.nAlelos);
    }

    // ⭐ EL CRECIMIENTO: el pelo y las unas crecen con los dias del mundo.
    // Un dia del mundo es un dia (el pelo humano, ~1,2 cm al mes; las unas
    // de la mano, ~3,5 mm). La barba crece solo si hay testosterona (XY o
    // aspecto masculino) y menos en la adolescencia.
    void crecer(double diaMundo) {
        if (ultimoDia < 0.0 || diaMundo < ultimoDia) { ultimoDia = diaMundo; return; }
        const double dias = diaMundo - ultimoDia;
        if (dias < 1e-4) return;
        ultimoDia = diaMundo;
        const float meses = (float)(dias / 30.0);
        poner(PELO_LARGO_CM, v[PELO_LARGO_CM] + v[PELO_CRECE] * meses);
        poner(UNA_LARGO_MM, v[UNA_LARGO_MM] + v[UNA_CRECE] * meses);
        if (esXY(genes) && v[CUERPO_EDAD] >= 16.0f)
            poner(BARBA_LARGO_MM, v[BARBA_LARGO_MM] + 12.0f * meses);
    }

    Fenotipo fenotipo() const {
        Fenotipo f;
        const Genoma& g = genes;
        f.varon = esXY(g);
        const float edad = v[CUERPO_EDAD];

        // --- PIGMENTACION ---
        // Melanina de la piel: base oscura, cada alelo aclarador la baja.
        float melanina = 0.62f;
        melanina -= 0.13f * copias(g, G_SLC24A5, 1);
        melanina -= 0.09f * copias(g, G_SLC45A2, 1);
        melanina -= 0.03f * copias(g, G_IRF4, 1);
        melanina -= 0.05f * (copias(g, G_MC1R, 1) == 2 ? 1.0f : 0.0f);
        melanina = sat01(melanina);
        f.albino = copias(g, G_TYR, 1) == 2;
        if (f.albino) melanina = 0.0f;
        const Color3 clara{ 0.96f, 0.80f, 0.70f }, media{ 0.80f, 0.58f, 0.42f },
                     oscura{ 0.36f, 0.22f, 0.15f };
        f.piel = melanina < 0.4f ? mezcla(clara, media, melanina / 0.4f)
                                 : mezcla(media, oscura, (melanina - 0.4f) / 0.6f);
        if (f.albino) f.piel = { 0.98f, 0.86f, 0.80f };

        // Ojos: HERC2/OCA2 decide cuanto pigmento hay en el estroma.
        const int azul = copias(g, G_HERC2, 1);
        if (f.albino) f.iris = { 0.62f, 0.68f, 0.80f };
        else if (azul == 2) f.iris = { 0.30f, 0.48f, 0.70f };
        else if (azul == 1) f.iris = { 0.36f, 0.42f, 0.22f };       // verde avellana
        else f.iris = { 0.30f, 0.18f, 0.09f };

        // Pelo: eumelanina (oscura) contra feomelanina (roja, la de MC1R).
        float eumel = 0.92f - 0.16f * copias(g, G_SLC45A2, 1) - 0.18f * copias(g, G_IRF4, 1)
                    - 0.08f * copias(g, G_SLC24A5, 1);
        const int mc1r = copias(g, G_MC1R, 1);
        if (mc1r == 2) eumel *= 0.25f;
        eumel = sat01(eumel);
        const Color3 rubio{ 0.80f, 0.66f, 0.40f }, castano{ 0.30f, 0.19f, 0.10f },
                     negro{ 0.07f, 0.055f, 0.05f }, rojo{ 0.62f, 0.25f, 0.10f };
        f.pelo = eumel < 0.5f ? mezcla(rubio, castano, eumel / 0.5f) : mezcla(castano, negro, (eumel - 0.5f) / 0.5f);
        if (mc1r == 2) f.pelo = mezcla(rojo, f.pelo, 0.12f);
        else if (mc1r == 1) f.pelo = mezcla(f.pelo, rojo, 0.08f);
        if (f.albino) f.pelo = { 0.95f, 0.93f, 0.86f };
        f.pecas = sat01(0.45f * mc1r + 0.15f * copias(g, G_IRF4, 1));

        // --- EL PELO: rizo y grosor ---
        f.rizo = sat01(0.15f + 0.35f * copias(g, G_TCHH, 1) - 0.12f * copias(g, G_EDAR, 1) + v[PELO_RIZO_EXTRA]);
        f.grosorPelo = 1.0f + 0.15f * copias(g, G_EDAR, 1);

        // --- LA EDAD ---
        // Canas desde los ~35, la mitad hacia los 55, casi todas a los 75.
        f.canas = sat01((edad - 35.0f) / 40.0f);
        // Calvicie androgenica: necesita androgenos (varon) y el alelo del X.
        if (f.varon && copias(g, G_AR, 1) >= 1) f.calvicie = sat01((edad - 22.0f) / 40.0f);
        else if (f.varon) f.calvicie = sat01((edad - 45.0f) / 60.0f);

        // --- EL MUSCULO ---
        const int mstn = copias(g, G_MSTN, 1);
        f.musculo = (mstn == 2 ? 1.45f : (mstn == 1 ? 1.12f : 1.0f));
        f.musculo *= 0.85f + 0.35f * v[CUERPO_FUERZA];
        if (f.varon) f.musculo *= 1.06f;
        // Con la edad se pierde masa (sarcopenia: ~1 % al ano desde los 50).
        if (edad > 50.0f) f.musculo *= 1.0f - 0.01f * (edad - 50.0f);
        const int x577 = copias(g, G_ACTN3, 1);
        f.potencia = x577 == 2 ? 0.88f : (x577 == 1 ? 0.97f : 1.0f);

        // --- LA ALTURA ---
        float h = f.varon ? 173.0f : 160.0f;
        h += 0.4f * copias(g, G_HMGA2, 1) + 0.3f * copias(g, G_GDF5, 1)
           + 0.3f * copias(g, G_LCORL, 1) + 0.3f * copias(g, G_ZBTB38, 1);
        // Hasta los 18 se sigue creciendo; desde los 40, ~1 cm por decada.
        if (edad < 18.0f) h *= 0.88f + 0.12f * (edad - 14.0f) / 4.0f;
        if (edad > 40.0f) h -= (edad - 40.0f) * 0.1f;
        f.alturaCm = h + v[CUERPO_ALTURA_EXTRA];

        // --- OTROS RASGOS ---
        f.toleraLactosa = copias(g, G_LCT, 1) >= 1;
        const bool A = copias(g, G_ABO, 0) > 0, B = copias(g, G_ABO, 1) > 0;
        std::snprintf(f.grupo, sizeof(f.grupo), "%s", A && B ? "AB" : (A ? "A" : (B ? "B" : "O")));

        f.camisa = tela(v[ROPA_CAMISA_TONO], v[ROPA_CAMISA_CLARO]);
        f.shortC = tela(v[ROPA_SHORT_TONO], v[ROPA_SHORT_CLARO]);
        f.pelo_cm = v[PELO_LARGO_CM] * (1.0f - 0.8f * f.calvicie);
        f.barba_mm = v[BARBA_LARGO_MM];
        f.una_mm = v[UNA_LARGO_MM];
        // Las canas aclaran el pelo hacia el gris.
        f.pelo = mezcla(f.pelo, Color3{ 0.78f, 0.78f, 0.76f }, f.canas * 0.85f);
        return f;
    }

    // ------------------------------------------------------------------
    // GUARDAR Y CARGAR: "clave valor" por linea, legible
    // ------------------------------------------------------------------
    std::string texto() const {
        std::string s;
        char linea[96];
        for (int i = 0; i < NUM_PARAMS; ++i) {
            std::snprintf(linea, sizeof(linea), "p%d %.4f\n", i, v[i]);
            s += linea;
        }
        for (int gi = 0; gi < NUM_GENES; ++gi) {
            std::snprintf(linea, sizeof(linea), "g%d %d %d\n", gi, genes.a[gi][0], genes.a[gi][1]);
            s += linea;
        }
        std::snprintf(linea, sizeof(linea), "dia %.4f\n", ultimoDia);
        s += linea;
        return s;
    }
    void leer(const std::string& s) {
        size_t i = 0;
        while (i < s.size()) {
            size_t f = s.find('\n', i);
            if (f == std::string::npos) f = s.size();
            const std::string l = s.substr(i, f - i);
            i = f + 1;
            int k = 0, a0 = 0, a1 = 0;
            float x = 0.0f;
            double d = 0.0;
            if (std::sscanf(l.c_str(), "p%d %f", &k, &x) == 2 && k >= 0 && k < NUM_PARAMS) poner(k, x);
            else if (std::sscanf(l.c_str(), "g%d %d %d", &k, &a0, &a1) == 3 && k >= 0 && k < NUM_GENES) {
                const int n = infoGen(k).nAlelos;
                genes.a[k][0] = (uint8_t)(((a0 % n) + n) % n);
                genes.a[k][1] = (uint8_t)(((a1 % n) + n) % n);
            } else if (std::sscanf(l.c_str(), "dia %lf", &d) == 1) ultimoDia = d;
        }
        genes.a[G_SRY][0] = 0;   // la madre siempre da un X
    }
};

} // namespace Avatar
