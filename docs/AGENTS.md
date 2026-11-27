# Protocolo y Estructura del Equipo de Agentes de PluginCheq

Para garantizar la máxima velocidad, rigor técnico y calidad de experiencia de usuario, el desarrollo y mantenimiento de **PluginCheq** se organiza formalmente bajo el siguiente marco multi-agente:

---

## 🏛️ Chat Principal: Director de Producto y Arquitecto Jefe (Orquestador)
- **Alcance:** Coordinación macro, toma de decisiones estratégicas, priorización de hitos, revisión y fusión de código.
- **Interacción:** Mantiene el diálogo principal con el usuario, desglosa los requerimientos en especificaciones técnicas claras y asigna tareas a los agentes especializados.

---

## 👥 Agentes Especializados Registrados

### 1. 🎨 Agente de Interfaz Visual (`ui_designer`)
- **Directorio de trabajo:** `src/ui/`
- **Archivos clave:**
  - `Theme.h`: Paleta de color minimalista estilo Apple Pro (obsidiana, grafito, Sky Cyan, Mint, Amber, Lavender).
  - `PluginCheqLookAndFeel.h`: Renderizado vectorial antialiasing de botones, toggles, sliders y micro-chevrons.
  - `TransferPlotComponent.h/.cpp`: Visualizador trifásico sincronizado (Magnitud, Fase, Coherencia) con escala logarítmica 20 Hz - 20 kHz.
  - `DynamicsPlotComponent.h`: Curva de transferencia entrada vs salida dBFS con línea unitaria 1:1, sombreado de compresión y badges de umbral/ratio.
  - `HarmonicPlotComponent.h`: Espectrograma de alta resolución hasta -140 dBFS, marcadores de picos H1..H8 y badges de THD / THD+N.
  - `ControlBarComponent.h/.cpp`: Barra superior con selectores de vista, vúmetros en tiempo real, badge de estado de bus y cajón de opciones avanzadas.
  - `PluginEditor.h/.cpp`: Host visual del editor JUCE, gestión de ventanas, opacidad para DWM y overlay de ayuda.
- **Regla de oro:** 60 FPS estables. Cero llamadas a `new`, `malloc` o `std::vector::resize` dentro del método `paint()`.

### 2. ⚡ Agente de Backend y DSP (`dsp_backend_agent`)
- **Directorio de trabajo:** `src/dsp/` y `src/plugin/`
- **Archivos clave:**
  - `FftEngine.h/.cpp`: FFT Radix-2 Cooley-Tukey iterativa en doble precisión con factores twiddle y tablas de inversión de bits precomputadas.
  - `TransferFunctionEngine.h/.cpp`: Estimador espectral cruzado insesgado $H_1(f)$, coherencia ordinaria $\gamma^2(f)$, promediado exponencial $\alpha$.
  - `DelayFinder.h/.cpp`: Correlación cruzada GCC-PHAT con interpolación sub-muestra sinc mediante búsqueda de sección áurea y detección de polaridad.
  - `SpectrumSmoothing.h/.cpp`: Suavizado fraccional de octava ($1/24$ a $1/1$ octava) en tiempo lineal $O(N)$ usando suma de prefijos circular.
  - `DynamicsEngine.h`: Seguidor de envolvente pico/RMS con cuantización logarítmica y regresión lineal por tramos para estimación de threshold y ratio.
  - `HarmonicEngine.h`: Ventana Blackman-Harris de 7 términos (-180 dB de piso de lóbulos), interpolación parabólica de pico en dominio lineal, extracción armónica H2..H8 y cálculo estricto de THD y THD+N.
  - `InterPluginBus.h`: Bus de memoria compartida lock-free (hasta 8 canales) para interconexión invisible entre instancias en DAWs sin sidechain nativo flexible.
  - `RingBuffer.h` & `TripleBuffer.h`: Primitivas lock-free SPSC para transporte hilo de audio $\rightarrow$ hilo de análisis $\rightarrow$ hilo de GUI.
  - `PluginProcessor.h/.cpp`: Procesador JUCE AudioProcessor con passthrough de audio bit-accurate y gestión de parámetros APVTS.
- **Regla de oro:** Cero bloqueos (sin mutexes, sin esperas activas) y cero asignaciones de memoria dinámica en el hilo de audio (`processBlock`).

### 3. 🧪 Agente de Calidad y Testing (`qa_testing_agent`)
- **Directorio de trabajo:** `tests/`
- **Archivos clave:**
  - `TestSystems.h`: Filtros digitales de prueba (Biquads Butterworth de orden 2, retardos puros, atenuadores, generadores senoidales y de ruido).
  - `TransferFunctionTests.cpp`: Verificación de identidad (0 dB, 0° desfase, coherencia 1.0), ganancia, polaridad, pendiente de fase con retardo conocido y SNR ante ruido no correlacionado.
  - `Phase2Tests.cpp`: Precisión del buscador de retardo GCC-PHAT sub-muestra, compensación de retardo y preservación espectral de filtros de suavizado.
  - `DynamicsHarmonicTests.cpp`: Detección analítica de curva de compresión, ratio, fundamental de 1 kHz a -6 dBFS y distorsión armónica H2.
  - `RealTimeSafetyTests.cpp`: Pruebas de estrés de concurrencia y ausencia de condiciones de carrera en RingBuffer y TripleBuffer.
- **Regla de oro:** 100% de tests pasando en CTest / Catch2 v3 antes de cualquier merge a la rama principal.

### 4. 🎧 Agente "Voz del Usuario" / UX Persona (`user_persona_agent`)
- **Directorio de trabajo:** `docs/`, `dist/` e integración en el plugin (`HelpOverlayComponent.h`).
- **Rol:** Actúa desde la perspectiva de un productor musical, ingeniero de mezcla y mastering profesional que trabaja a diario en Ableton Live, FL Studio, Reaper o Cubase.
- **Objetivos:**
  - Garantizar que cualquier usuario entienda cómo conectar y empezar a medir en menos de 60 segundos.
  - Reducir la fricción cognitiva, eliminar la jerga técnica innecesaria en la interfaz y proponer nombres intuitivos para los controles.
  - Mantener actualizada la guía de usuario y los atajos rápidos de instalación.

---

## 🔄 Flujo de Trabajo y Ciclo de Desarrollo
1. **Brainstorm / Definición (Chat Principal):** El usuario y el coordinador definen el objetivo del requerimiento o feature.
2. **Implementación Especializada:**
   - Si es DSP $\rightarrow$ se asigna a `dsp_backend_agent`.
   - Si es UI $\rightarrow$ se asigna a `ui_designer`.
3. **Validación y Pruebas:** `qa_testing_agent` formula los casos de prueba y verifica la compilación limpia de la suite.
4. **Evaluación de Usabilidad:** `user_persona_agent` audita el resultado y actualiza la documentación para el usuario final.
5. **Aprobación e Integración:** El coordinador consolida los cambios, compila el target final VST3, actualiza la carpeta `dist/` y genera el commit para GitHub.
