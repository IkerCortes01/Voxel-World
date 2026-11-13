# 🚀 QUICKSTART - IA PARA VOXEL WORLD

## 🎯 LO QUE SE INSTALÓ

### 1. 🎮 Game Developer Skill
**Para qué sirve:** Optimizar tu motor de juego con patterns profesionales

**Ejemplos de uso:**
```
"Optimiza el ChunkSystem para garantizar 60 FPS"
"Implementa object pooling para el ParticleSystem"
"Crea un state machine para el menú de pausa"
"Analiza cuellos de botella en el rendering"
```

---

### 2. 🧠 Graphify - Memoria del Código
**Para qué sirve:** Claude "recuerda" toda la estructura de tu código

**Usa esto cuando:**
- ❓ "¿Dónde se usa la función X?"
- ❓ "¿Cómo se relaciona ChunkSystem con SaveSystem?"
- ❓ "Muéstrame todas las clases que usan BlockType"

**Comando en Claude Code:**
```bash
/graphify .
```

**Estadísticas de tu proyecto:**
- 📊 8,059 símbolos mapeados
- 🔗 17,228 relaciones detectadas
- 🏘️ 462 comunidades de código

---

### 3. 📚 1000+ Skills Adicionales
**Ubicación:** `~/.claude/skills/awesome-skills-temp/`

Explora y copia los que necesites.

---

## ⚡ COMANDOS RÁPIDOS

### Ver Knowledge Graph:
```bash
# Abrir el reporte
cat graphify-out/GRAPH_REPORT.md

# Buscar en el graph
graphify query "sistema de chunks"

# Encontrar conexiones
graphify path "Chunk" "World"
```

### Regenerar Graph (después de cambios):
```bash
graphify . --code-only
graphify cluster-only .
```

---

## 🎯 PROMPTS ÚTILES PARA TU PROYECTO

### Optimización:
```
"Analiza el sistema de rendering y sugiere optimizaciones para 60 FPS"
"Implementa greedy meshing en buildChunkMesh()"
"Optimiza el sistema de colisiones AABB"
```

### Nuevas Features:
```
"Agrega un sistema de biomas más avanzado usando PerlinNoise"
"Implementa un sistema de lighting dinámico"
"Crea un inventario con drag & drop visual"
```

### Debugging:
```
"¿Por qué el FPS baja cuando genero muchos chunks?"
"Analiza memory leaks en el ChunkManager"
"Encuentra race conditions en el threading system"
```

### Arquitectura:
```
"¿Cómo puedo separar mejor el rendering del game logic?"
"Sugiere una arquitectura ECS para las entidades"
"¿Debería usar un pattern observer para los eventos?"
```

---

## 📂 ARCHIVOS IMPORTANTES

### Skills:
- `~/.claude/skills/game-developer/` - Skill de game dev
- `~/.claude/skills/graphify/` - Skill de Graphify
- `~/.claude/CLAUDE.md` - Configuración de Graphify

### Knowledge Graph:
- `graphify-out/graph.json` - Graph completo (9.7 MB)
- `graphify-out/GRAPH_REPORT.md` - Análisis de comunidades
- `graphify-out/manifest.json` - Metadata

### Documentación:
- `INSTALLED_TOOLS.md` - Guía completa de instalaciones
- Este archivo - Quickstart rápido

---

## ✅ PRÓXIMOS PASOS

1. **Reinicia Claude Code** para cargar los skills
2. Prueba: `/graphify .` 
3. Pregunta algo sobre tu código
4. Usa prompts de game development
5. ¡Desarrolla con superpoderes! 🚀

---

**Tip:** Graphify funciona mejor cuando:
- Haces preguntas específicas sobre relaciones entre código
- Buscas símbolos o funciones específicas
- Quieres entender flujos de datos o llamadas

**El Game Developer Skill es mejor para:**
- Implementar nuevas features
- Optimizar rendimiento
- Aplicar game design patterns
- Debugging y profiling

---

🎮 **¡AHORA TIENES UN ASISTENTE DE IA QUE ENTIENDE TU CÓDIGO COMPLETO!**
