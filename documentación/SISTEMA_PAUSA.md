# Sistema de Pausa - Voxel World

## 🎮 Funcionalidad Implementada

El juego ahora tiene un sistema de pausa completo que congela toda la acción y muestra un overlay visual atractivo.

---

## ⌨️ Controles

| Tecla | Acción |
|-------|--------|
| **ESC** | Pausar el juego |
| **ESC** (de nuevo) | Reanudar el juego |

---

## 🎯 Comportamiento del Sistema

### **Cuando Presionas ESC (Pausar):**

1. ✅ **El juego se congela completamente:**
   - No hay actualización de físicas
   - El jugador no se mueve
   - La gravedad se detiene
   - Los chunks no se generan
   - El tiempo se detiene

2. ✅ **El cursor se vuelve visible:**
   - Puedes mover el mouse libremente
   - El cursor ya no está bloqueado

3. ✅ **Aparece el overlay visual:**
   - Filtro oscuro semi-transparente (50% opacidad)
   - Caja central con texto "PAUSA"
   - Instrucción "PRESIONA ESC"
   - El crosshair desaparece

### **Cuando Presionas ESC de Nuevo (Reanudar):**

1. ✅ **El juego se reanuda:**
   - Las físicas continúan
   - El jugador puede moverse
   - Todo vuelve a la normalidad

2. ✅ **El cursor se oculta de nuevo:**
   - Vuelve al modo bloqueado (FPS)
   - La cámara vuelve a responder al mouse

3. ✅ **El overlay desaparece:**
   - Todo vuelve a ser visible
   - El crosshair reaparece

---

## 🎨 Diseño Visual

### **Overlay Oscuro:**
```
Color: Negro (0, 0, 0)
Opacidad: 50% (0.5 alpha)
Tamaño: Pantalla completa
Efecto: Oscurece ligeramente todo el fondo
```

### **Caja de Texto Principal:**
```
Color de fondo: Gris muy oscuro (0.1, 0.1, 0.1) con 80% opacidad
Borde: Blanco (1.0, 1.0, 1.0) de 3px
Tamaño: 300x60 pixels
Posición: Centrada en la pantalla
```

### **Texto "PAUSA":**
```
Color: Amarillo brillante (1.0, 1.0, 0.0)
Estilo: Bloques pixelados (estilo retro)
Letras: P-A-U-S-A (representado con rectángulos)
Posición: Centro de la caja
```

### **Caja de Instrucción:**
```
Color: Gris claro (0.8, 0.8, 0.8)
Texto: "PRESIONA ESC"
Posición: Debajo de la caja principal
Tamaño: 200x30 pixels
```

---

## 🔧 Implementación Técnica

### **Estructura GameState:**

```cpp
struct GameState {
    Player player;
    World world;
    bool keys[256];
    double lastMouseX;
    double lastMouseY;
    bool firstMouse;
    bool cursorLocked;
    bool isPaused;  // ← NUEVO

    GameState() : firstMouse(true), cursorLocked(true), isPaused(false) {
        for (int i = 0; i < 256; i++) keys[i] = false;
    }
};
```

### **Manejo de Tecla ESC:**

```cpp
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        // Alternar pausa
        g_gameState->isPaused = !g_gameState->isPaused;

        if (g_gameState->isPaused) {
            // Pausado: mostrar cursor
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            g_gameState->cursorLocked = false;
        } else {
            // Reanudar: ocultar cursor
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            g_gameState->cursorLocked = true;
            g_gameState->firstMouse = true;
        }
    }

    // No procesar teclas de movimiento si está pausado
    if (g_gameState->isPaused) return;

    // ... resto del manejo de teclas (W/A/S/D/SPACE)
}
```

### **Actualización Condicional de Físicas:**

```cpp
while (!glfwWindowShouldClose(window)) {
    double currentTime = glfwGetTime();
    float deltaTime = (float)(currentTime - lastTime);
    lastTime = currentTime;

    // Solo actualizar físicas si no está pausado
    if (!g_gameState->isPaused) {
        updatePlayerPhysics(g_gameState->player, g_gameState->world, deltaTime, g_gameState->keys);
        g_gameState->world.updateChunks(g_gameState->player.position);
    }

    // ... renderizado continúa normalmente
}
```

### **Renderizado del Overlay:**

```cpp
// Después del renderizado 3D, en modo 2D ortográfico:

if (g_gameState->isPaused) {
    // Habilitar transparencia
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 1. Overlay oscuro de pantalla completa
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(width, 0);
    glVertex2f(width, height);
    glVertex2f(0, height);
    glEnd();

    // 2. Caja de fondo para texto
    glColor4f(0.1f, 0.1f, 0.1f, 0.8f);
    glBegin(GL_QUADS);
    // ... dibujar caja centrada
    glEnd();

    // 3. Borde blanco de la caja
    glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
    glLineWidth(3);
    glBegin(GL_LINE_LOOP);
    // ... dibujar borde
    glEnd();

    // 4. Texto "PAUSA" (usando rectángulos)
    glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
    // ... dibujar letras P-A-U-S-A con GL_QUADS

    // 5. Caja de instrucción
    glColor4f(0.8f, 0.8f, 0.8f, 1.0f);
    // ... dibujar caja de texto abajo

    glDisable(GL_BLEND);
}
```

---

## 📊 Elementos Renderizados

### **Cuando NO está Pausado:**
```
✅ Mundo 3D (chunks, bloques)
✅ Crosshair (mira en el centro)
✅ Cursor oculto
✅ Físicas actualizándose
```

### **Cuando SÍ está Pausado:**
```
✅ Mundo 3D (congelado en el último frame)
✅ Overlay oscuro semi-transparente
✅ Caja de texto "PAUSA"
✅ Instrucción "PRESIONA ESC"
✅ Cursor visible
❌ Crosshair oculto
❌ Físicas detenidas
❌ Teclas de movimiento no responden
```

---

## 🎯 Casos de Uso

### **1. Pausa Rápida:**
```
Usuario: *Presiona ESC*
Juego: Se pausa instantáneamente
Usuario: *Lee información, toma un respiro*
Usuario: *Presiona ESC de nuevo*
Juego: Reanuda exactamente donde estaba
```

### **2. Explorar el Mundo sin Moverte:**
```
Usuario: *Mueve la cámara a una vista interesante*
Usuario: *Presiona ESC*
Resultado: El mundo queda congelado en esa vista
Usuario: Puede admirar el terreno sin que el jugador caiga o se mueva
```

### **3. Multitarea:**
```
Usuario: *Presiona ESC*
Usuario: *Abre otra ventana, chatea, etc.*
Juego: Permanece pausado y seguro
```

---

## 🔍 Detalles de Diseño

### **Transparencia con OpenGL:**

El sistema usa **blending** para lograr la transparencia:

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

- **GL_SRC_ALPHA:** Color del overlay multiplicado por su alpha
- **GL_ONE_MINUS_SRC_ALPHA:** Color de fondo multiplicado por (1 - alpha del overlay)
- **Resultado:** Mezcla suave entre overlay y fondo

### **Por qué 50% de Opacidad:**

```
0% opacidad = Transparente (no se ve el overlay)
50% opacidad = Balance perfecto (se ve el fondo oscurecido pero claro)
100% opacidad = Completamente negro (no se ve el mundo de fondo)
```

El 50% (0.5 alpha) permite:
- ✅ Ver el mundo de fondo claramente
- ✅ Crear sensación de "congelamiento"
- ✅ Destacar el texto de pausa
- ✅ No ocultar completamente el progreso

---

## 📈 Mejoras Futuras Posibles

El sistema de pausa está listo para expandirse con:

- 📋 **Menú de pausa con opciones:**
  - Reanudar
  - Opciones
  - Salir al menú principal
  - Salir del juego

- ⚙️ **Opciones en pausa:**
  - Ajustar volumen
  - Cambiar distancia de render
  - Configurar controles
  - Ajustar brillo

- 💾 **Guardado rápido:**
  - Botón "Guardar" en el menú de pausa
  - Guardado automático al pausar

- 📊 **Estadísticas:**
  - Tiempo jugado
  - Bloques minados
  - Distancia recorrida

- 🎨 **Personalización:**
  - Cambiar color del overlay
  - Ajustar opacidad
  - Diferentes estilos de texto

---

## ✅ Características Implementadas

### **Completado:**
- ✅ Pausa y reanudación con ESC
- ✅ Congelamiento completo de físicas
- ✅ Overlay oscuro semi-transparente (50%)
- ✅ Texto visual "PAUSA" estilo pixelado
- ✅ Instrucción de cómo reanudar
- ✅ Cursor visible durante pausa
- ✅ Crosshair oculto durante pausa
- ✅ Teclas de movimiento deshabilitadas durante pausa

### **Comportamiento:**
- ✅ Alternar pausa: ESC → Pausa → ESC → Reanuda
- ✅ Estado del cursor sincronizado con pausa
- ✅ Blending correcto para transparencia
- ✅ Renderizado 2D overlay sobre mundo 3D
- ✅ Sin impacto en rendimiento (solo renderizado extra cuando pausado)

---

## 🎮 Cómo Probar

1. **Ejecuta el juego:** `build/bin/Release/VoxelWorld.exe`
2. **Presiona ESC:** Verás el overlay de pausa aparecer
3. **Observa:**
   - El mundo se congela
   - Aparece un filtro oscuro
   - Se muestra "PAUSA" en amarillo
   - El cursor se vuelve visible
4. **Presiona ESC de nuevo:** Todo vuelve a la normalidad
5. **Prueba durante movimiento:** Pausa mientras corres → Todo se congela instantáneamente

---

## 🎊 Conclusión

El sistema de pausa proporciona:

- ⏸️ **Control total del flujo del juego**
- 🎨 **Feedback visual claro y profesional**
- 🖱️ **Cursor visible para futuras opciones**
- ⚡ **Respuesta instantánea (sin lag)**
- 🔒 **Congelamiento completo y seguro**

**¡Presiona ESC en cualquier momento para pausar tu aventura!** 🎮✨
