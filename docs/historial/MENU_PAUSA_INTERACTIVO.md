# Menú de Pausa Interactivo - Voxel World

## 🎮 Sistema Completo Implementado

El juego ahora tiene un menú de pausa completamente interactivo con botones clicables, submenús y configuraciones ajustables en tiempo real.

---

## ⌨️ Controles

| Tecla | Acción |
|-------|--------|
| **ESC** | Abrir menú de pausa / Volver al menú anterior / Reanudar |
| **Click izquierdo** | Hacer click en botones |
| **Mouse** | Navegación (hover effects) |

---

## 📋 Menú Principal (4 Botones)

Cuando presionas **ESC**, aparece el menú principal con 4 opciones:

### **1. Reanudar Partida**
- **Color:** Verde (más brillante al hacer hover)
- **Función:** Cierra el menú y continúa el juego
- **Atajo:** ESC desde el menú principal

### **2. Gráficos**
- **Color:** Verde (más brillante al hacer hover)
- **Función:** Abre el submenú de configuración gráfica
- **Contenido:** Ajustar distancia de renderizado (render distance)

### **3. Sensibilidad**
- **Color:** Verde (más brillante al hacer hover)
- **Función:** Abre el submenú de configuración de sensibilidad
- **Contenido:** Ajustar sensibilidad del mouse

### **4. Salir**
- **Color:** Rojo (más brillante al hacer hover)
- **Función:** Cierra el juego completamente
- **Advertencia:** Sale inmediatamente sin guardar

---

## ⚙️ Submenú: Gráficos

### **Opciones Disponibles:**

#### **Distancia de Render (Render Distance)**
- **Valor por defecto:** 8 chunks
- **Rango:** 2 - 16 chunks
- **Controles:**
  - **Botón [-]**: Disminuir distancia de render (mínimo 2)
  - **Valor central**: Muestra el valor actual en amarillo
  - **Botón [+]**: Aumentar distancia de render (máximo 16)

#### **Efectos:**
- 📉 **Menor distancia:** Mayor rendimiento (FPS), menos chunks visibles
- 📈 **Mayor distancia:** Menor rendimiento, más chunks visibles, mejor vista panorámica

#### **Botón Volver:**
- Regresa al menú principal
- **Atajo:** Presiona ESC

---

## 🎯 Submenú: Sensibilidad

### **Opciones Disponibles:**

#### **Sensibilidad del Mouse**
- **Valor por defecto:** 0.15
- **Rango:** 0.05 - 0.50
- **Incremento:** 0.01 por click
- **Controles:**
  - **Botón [-]**: Disminuir sensibilidad (mínimo 0.05)
  - **Valor central**: Muestra el valor actual en amarillo
  - **Botón [+]**: Aumentar sensibilidad (máximo 0.50)

#### **Efectos:**
- 🐢 **Menor sensibilidad:** Movimientos más lentos y precisos
- 🚀 **Mayor sensibilidad:** Movimientos más rápidos y ágiles

#### **Botón Volver:**
- Regresa al menú principal
- **Atajo:** Presiona ESC

---

## 🎨 Diseño Visual

### **Overlay Oscuro:**
```
- Color: Negro (0, 0, 0)
- Opacidad: 50% (0.5 alpha)
- Tamaño: Pantalla completa
- Efecto: Oscurece el mundo de fondo
```

### **Botones:**

#### **Estados de Botón:**
1. **Normal:** Color base semi-transparente
2. **Hover:** Color más brillante (efecto de iluminación)
3. **Borde:** Blanco de 2px en todos los botones

#### **Colores por Tipo:**
- **Reanudar/Gráficos/Sensibilidad:** Verde oscuro → Verde brillante (hover)
- **Salir:** Rojo oscuro → Rojo brillante (hover)
- **Botones +/-:** Gris oscuro → Gris claro (hover)
- **Volver:** Gris medio → Gris claro (hover)

#### **Títulos:**
- **"PAUSA":** Amarillo brillante
- **"GRAFICOS":** Azul brillante
- **"SENSIBILIDAD":** Naranja brillante

---

## 🔧 Flujo de Navegación

```
[Jugando]
   ↓ (Presionar ESC)
[Menú Principal]
   ├─ Reanudar → [Jugando]
   ├─ Gráficos → [Submenú Gráficos]
   │               ├─ Ajustar Render Distance
   │               ├─ ESC → [Menú Principal]
   │               └─ Click "Volver" → [Menú Principal]
   ├─ Sensibilidad → [Submenú Sensibilidad]
   │                  ├─ Ajustar Sensibilidad Mouse
   │                  ├─ ESC → [Menú Principal]
   │                  └─ Click "Volver" → [Menú Principal]
   └─ Salir → [Cerrar Juego]
```

---

## 🖱️ Interacción del Mouse

### **Hover Effects:**
- Los botones cambian de color cuando el mouse pasa sobre ellos
- Feedback visual instantáneo
- Indica claramente qué botón se va a presionar

### **Click Detection:**
- Detección precisa de clicks dentro del área del botón
- Solo funciona cuando el juego está pausado
- Respuesta inmediata al click

### **Cursor:**
- **Jugando:** Oculto y bloqueado (modo FPS)
- **Pausado:** Visible y libre

---

## 📊 Configuraciones Guardadas en Memoria

Las configuraciones se guardan en `GameState` y se aplican instantáneamente:

```cpp
struct GameState {
    // ...
    int renderDistance;       // 2-16 chunks (default: 8)
    float mouseSensitivity;   // 0.05-0.50 (default: 0.15)
    // ...
};
```

### **Persistencia:**
- ✅ Las configuraciones persisten durante la sesión actual
- ❌ **NO** se guardan al cerrar el juego (futuro: guardar en archivo .cfg)

---

## 🎯 Casos de Uso

### **Caso 1: Ajustar Gráficos en Tiempo Real**
```
Usuario: *Nota lag, presiona ESC*
Usuario: *Click en "Gráficos"*
Usuario: *Click en [-] varias veces para reducir render distance*
Juego: FPS mejora inmediatamente
Usuario: *Presiona ESC dos veces para volver a jugar*
```

### **Caso 2: Ajustar Sensibilidad del Mouse**
```
Usuario: "El mouse está muy rápido"
Usuario: *Presiona ESC*
Usuario: *Click en "Sensibilidad"*
Usuario: *Click en [-] para reducir sensibilidad*
Usuario: *Prueba moviendo el mouse en el menú*
Usuario: *Ajusta hasta que se sienta cómodo*
Usuario: *Click en "Volver" → Click en "Reanudar"*
```

### **Caso 3: Navegación Rápida**
```
ESC → Abre menú principal
ESC → Reanudar juego

ESC → Abre menú principal
Click "Gráficos" → Submenú gráficos
ESC → Volver a menú principal
ESC → Reanudar juego
```

---

## 🔍 Detalles Técnicos

### **Estructura de Botón:**

```cpp
struct Button {
    float x, y, width, height;
    const char* text;
    bool isHovered;

    bool contains(float mouseX, float mouseY) const {
        return mouseX >= x && mouseX <= x + width &&
               mouseY >= y && mouseY <= y + height;
    }
};
```

### **Estados del Menú:**

```cpp
enum PauseMenuState {
    PAUSE_MENU_MAIN,        // Menú principal
    PAUSE_MENU_GRAPHICS,    // Submenú gráficos
    PAUSE_MENU_SENSITIVITY  // Submenú sensibilidad
};
```

### **Callback de Mouse Button:**

```cpp
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // Solo procesa clicks izquierdos cuando está pausado
    if (!g_gameState || !g_gameState->isPaused) return;
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;

    // Obtiene posición del mouse
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // Verifica qué botón fue clickeado según el menú actual
    // ... lógica de detección de clicks
}
```

### **Renderizado de Botones:**

```cpp
// Crear botón
Button btn(x, y, width, height, "Texto");

// Detectar hover
bool isHovered = btn.contains(mouseX, mouseY);

// Cambiar color según hover
glColor4f(
    isHovered ? 0.3f : 0.2f,  // Rojo
    isHovered ? 0.6f : 0.4f,  // Verde
    isHovered ? 0.3f : 0.2f,  // Azul
    0.9f                       // Alpha
);

// Dibujar fondo del botón
glBegin(GL_QUADS);
// ... vértices del botón
glEnd();

// Dibujar borde del botón
glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
glLineWidth(2);
glBegin(GL_LINE_LOOP);
// ... vértices del borde
glEnd();
```

---

## 📈 Mejoras Futuras Posibles

El sistema está preparado para:

### **Guardar Configuraciones:**
- 💾 Guardar render distance y sensibilidad en `config.ini`
- 💾 Cargar configuraciones al iniciar el juego
- 💾 Guardar al salir del juego

### **Más Opciones Gráficas:**
- 🌫️ Niebla (fog)
- 🌤️ Iluminación avanzada
- 🎨 Shaders personalizados
- 📺 Resolución y pantalla completa
- 🔊 Volumen de sonido (cuando se añada audio)

### **Más Opciones de Control:**
- ⌨️ Remapeo de teclas (W/A/S/D personalizables)
- 🎮 Soporte de gamepad
- 🖱️ Invertir ejes del mouse

### **Interfaz Mejorada:**
- 🔤 Texto renderizado (en lugar de bloques simples)
- 🎨 Texturas para botones
- ✨ Animaciones de transición entre menús
- 🎵 Sonidos de click

---

## ✅ Características Implementadas

### **Completado:**
- ✅ Menú principal con 4 botones funcionales
- ✅ Submenú de gráficos con ajuste de render distance
- ✅ Submenú de sensibilidad con ajuste de mouse
- ✅ Detección de clicks del mouse
- ✅ Hover effects en todos los botones
- ✅ Navegación con ESC (volver al menú anterior)
- ✅ Overlay oscuro semi-transparente (50%)
- ✅ Configuraciones aplicadas en tiempo real
- ✅ Cursor visible durante pausa
- ✅ Botones con colores diferenciados
- ✅ Títulos visuales para cada menú

### **Comportamiento:**
- ✅ Click izquierdo interactúa con botones
- ✅ ESC navega hacia atrás en menús
- ✅ ESC desde menú principal reanuda el juego
- ✅ Botón "Salir" cierra el juego
- ✅ Botones +/- ajustan valores con límites
- ✅ Hover effects indican interactividad

---

## 🎮 Cómo Probar

1. **Ejecuta el juego:** `build/bin/Release/VoxelWorld.exe`

2. **Presiona ESC:** Verás el menú principal con 4 botones

3. **Prueba cada botón:**
   - **Reanudar:** Click o presiona ESC de nuevo
   - **Gráficos:** Click → Ajusta render distance con +/- → ESC para volver
   - **Sensibilidad:** Click → Ajusta sensibilidad con +/- → ESC para volver
   - **Salir:** Click para cerrar el juego

4. **Observa los hover effects:** Mueve el mouse sobre los botones

5. **Prueba la navegación con ESC:**
   - ESC → Menú principal
   - Click "Gráficos" → ESC → Vuelve a menú principal
   - ESC → Reanudar juego

---

## 🎊 Conclusión

El menú de pausa interactivo proporciona:

- 🖱️ **Botones clicables con hover effects**
- 📋 **Menú principal con 4 opciones**
- ⚙️ **Submenús de configuración funcionales**
- 🎮 **Ajustes en tiempo real sin reiniciar**
- 🌑 **Overlay oscuro semi-transparente**
- ⌨️ **Navegación intuitiva con ESC**
- 🎨 **Diseño visual claro y atractivo**

**¡Presiona ESC y explora todas las opciones del menú de pausa!** 🎮✨

---

## 📝 Resumen de Botones

| Botón | Ubicación | Color | Función |
|-------|-----------|-------|---------|
| **Reanudar** | Menú principal | Verde | Continuar jugando |
| **Gráficos** | Menú principal | Verde | Abrir opciones gráficas |
| **Sensibilidad** | Menú principal | Verde | Abrir opciones de control |
| **Salir** | Menú principal | Rojo | Cerrar el juego |
| **[-]** | Submenús | Gris | Disminuir valor |
| **[+]** | Submenús | Gris | Aumentar valor |
| **Volver** | Submenús | Gris | Regresar al menú principal |

**¡Todo funcional y listo para usar!** 🚀
