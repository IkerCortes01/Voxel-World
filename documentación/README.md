# Voxel World - Juego Sandbox Infinito

Motor 3D extraído de AniWorld (sin keyframes ni timeline), diseñado específicamente para un juego sandbox de bloques con chunks infinitos y física de gravedad.

## Características

- **Motor 3D simple**: Extraído de AniWorld, solo el sistema de renderizado 3D básico
- **Sistema de bloques**: 8 tipos de bloques (Grass, Dirt, Stone, Wood, Leaves, Sand, Water)
- **Chunks infinitos**: Generación procedural con Perlin noise
- **Física de gravedad**: El jugador NO puede flotar, tiene gravedad realista
- **Colisiones AABB**: Detección de colisiones con bloques
- **Sistema de guardado**: Guarda/carga mundos en formato binario
- **OpenGL 2.1**: Compatible con hardware antiguo (Intel HD 4000+)

## Controles

- **WASD** - Movimiento
- **Mouse** - Rotar cámara
- **Espacio** - Saltar
- **ESC** - Bloquear/desbloquear cursor

## Compilar

```batch
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

## Ejecutar

```batch
build\bin\Release\VoxelWorld.exe
```

## Estructura del Proyecto

```
Voxel World/
├── src/
│   └── main.cpp           # Todo el código en un solo archivo
├── external/
│   ├── glfw/              # Ventanas y input
│   └── stb/               # Utilidades STB
├── projects/              # Mundos guardados
└── build/
    └── bin/Release/
        └── VoxelWorld.exe # Ejecutable compilado
```

## Sistema de Guardado

Los mundos se guardan en `projects/<nombre>/world.dat` con:
- Seed del mundo
- Todos los chunks cargados
- Bloques modificados por el jugador

## Diferencias con AniWorld

Este proyecto contiene SOLO el motor 3D de AniWorld, sin:
- ❌ Sistema de keyframes
- ❌ Timeline de animación
- ❌ Editor de curvas
- ❌ Sistema de objetos 3D complejos
- ❌ Simulación de fluidos
- ✅ Solo renderizado 3D, chunks, física básica y guardado

## Requisitos

- Windows 10/11 (64-bit)
- OpenGL 2.1+ (Intel HD 4000 o superior)
- Visual Studio 2022 (para compilar)

## Tecnologías

- C++20
- OpenGL 2.1 (fixed pipeline, display lists)
- GLFW 3.x
- Perlin Noise (generación procedural)

---

**Motor extraído de AniWorld por solicitud del usuario.**
