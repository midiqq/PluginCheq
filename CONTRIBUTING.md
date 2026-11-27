# Guía de Contribución y Estándares de Ingeniería — PluginCheq

Para mantener la calidad y el rigor científico exigido en software de audio profesional, todo cambio en **PluginCheq** debe adherir a los siguientes estándares:

---

## 1. Reglas Estrictas de Tiempo Real (Audio Thread Safety)
Cualquier código que se ejecute en `PluginProcessor::processBlock()` o en rutas de señal de audio debe cumplir:
1. **CERO Asignaciones de Memoria Dinámica:** No usar `malloc`, `free`, `new`, `delete`, ni métodos que puedan reasignar memoria (`std::vector::resize`, `std::string`, `push_back`).
2. **CERO Bloqueos y Primitivas de Sincronización:** No usar `std::mutex`, semáforos, variables de condición, ni `std::atomic` con espera activa (*spinlocks*).
3. **Comunicación Exclusiva por Buffers Lock-Free:** Todo intercambio de datos entre el hilo de audio y los hilos de fondo o GUI debe realizarse a través de `RingBuffer` (SPSC lock-free) o `TripleBuffer`.

---

## 2. Estándares de Código C++20
- **Estándar:** C++20 estricto con compilación limpia sin advertencias en MSVC `/W4` o `/utf-8`.
- **Precisión Matemática:**
  - Los cálculos espectrales, integrales y correlaciones deben ejecutarse en `double` para evitar acumulación de errores de redondeo.
  - El traspaso a GUI se realiza en `float` para optimizar el renderizado visual.
- **Rendimiento Gráfico:** El método `paint()` de los componentes JUCE debe limitarse a operaciones vectoriales de dibujo sin instanciar objetos pesados en cada cuadro.

---

## 3. Cobertura de Pruebas y Validación
- Todo nuevo algoritmo DSP debe acompañarse de su correspondiente caso de prueba en `tests/`.
- La suite completa de pruebas debe ejecutarse mediante `scripts/test.ps1` y alcanzar el 100% de éxito antes de realizar commits.
