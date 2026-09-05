#include "fauna/PecariSpawn.h"
#include <cstdio>
using namespace Fauna; using namespace TerrainGen;
int main(){
    PecariSpawn s(12345);
    // Area de 2km x 2km reales = 3333 x 3333 bloques
    const int L = 3333;
    int manadas=0, individuos=0, minM=999, maxM=0;
    for(int x=0;x<L;++x) for(int z=0;z<L;++z){
        ManadaSpawn m = s.ConsultarManada(x,z,BIOME_FOREST,50.0f,0.1f);
        if(m.existe){ manadas++; individuos+=m.miembros;
            if(m.miembros<minM)minM=m.miembros; if(m.miembros>maxM)maxM=m.miembros; }
    }
    double km2 = (L*0.60)*(L*0.60)/1e6;
    printf("Area analizada: %.2f km2 reales (%d x %d bloques)\n", km2, L, L);
    printf("Manadas: %d  -> %.2f manadas/km2\n", manadas, manadas/km2);
    printf("Individuos: %d -> %.2f ind/km2\n", individuos, individuos/km2);
    printf("Tamano manada: min=%d max=%d media=%.2f\n", minM, maxM, (double)individuos/manadas);
    printf("\nDato MEDIDO Quintana Roo: 0.2 manadas/km2, 1.9 ind/km2, media 9.5\n");
    printf("Distancia media entre manadas vecinas: ~%.0f m\n", 1000.0/ (manadas/km2 > 0 ? sqrt(manadas/km2) : 1));
    return 0;
}
