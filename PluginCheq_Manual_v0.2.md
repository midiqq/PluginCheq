# PluginCheq — Manual de Usuario Detallado
### Versión 0.2 (Fase 1 + 2 Mejorada: Ruteo Inter-Plugin, Generador y Modo Didáctico)

---

## 1. INTRODUCCIÓN

**PluginCheq** es un laboratorio de metrología y medición DSP en tiempo real diseñado para insertarse en tu DAW (VST3) o ejecutarse como aplicación independiente (**Standalone**). 

Su propósito central es la **Función de Transferencia ($H_1 / H_2$)**: comparar con precisión científica qué alteraciones exactas introduce un procesador, ecualizador, saturador o cadena completa entre dos puntos:
- **REFERENCE (Señal Limpia / Test):** Punto previo a la cadena de audio.
- **MEASUREMENT (Señal Procesada):** Punto posterior a la cadena de audio.

El plugin **no colorea ni altera la señal de paso** (passthrough *bit-accurate*).

---

## 2. PROBLEMA Y SOLUCIÓN DE ESCALADO BORROSO EN ABLETON LIVE

### ¿Por qué se ve borroso en Ableton Live en Windows?
Ableton Live en Windows aplica por defecto una función llamada **"Auto-scale plug-in window"** en pantallas con escalado DPI (125%, 150%, 4K). Esta función fuerza a Windows a estirar la ventana del plugin como si fuera un bitmap estático, generando texto borroso y líneas difusas.

### Cómo solucionarlo en 5 segundos (100% Nítido):
1. En Ableton Live, haz **click derecho sobre la barra de título del plugin** (en la barra superior del dispositivo en la vista de pista de Ableton, no dentro de la ventana del plugin).
2. Desmarca la opción **"Auto-scale plug-in window"** (Auto-escalar ventana de plug-in).
3. Cierra y vuelve a abrir la ventana del plugin: **ahora se verá con nitidez vectorial perfecta a resolución nativa**.

---

## 3. ASIGNACIÓN Y ENRUTAMIENTO DE SEÑALES (REF vs MEAS)

Dado que DAWs como Ableton Live carecen de un menú nativo de Sidechain directo y flexible como Reaper, PluginCheq incorpora **4 MODOS DE ENRUTAMIENTO SELECCIONABLES DESDE EL PROPIO VST**:

En la esquina superior izquierda encontrarás el menú desplegable **Routing Mode**:

```
[ Routing Mode: ▼ ]
  ├─ 1. Plugin Tap Bus (Ableton)  <-- (Recomendado para Ableton Live y FL Studio)
  ├─ 2. Tap Sender (Pre-FX)       <-- (Instancia previa para capturar señal limpia)
  ├─ 3. DAW Sidechain             <-- (Para Reaper, Cubase, Studio One)
  └─ 4. Internal Generator        <-- (Para Standalone o pruebas autónomas)
```

### Método A: "Plugin Tap Bus" (El flujo ideal para Ableton Live)
Este método permite comunicar dos instancias de PluginCheq en tiempo real mediante memoria compartida *lock-free*, resolviendo por completo las limitaciones de ruteo de Ableton.

1. **Paso 1 (Captura Pre-FX):** Inserta una instancia de PluginCheq al principio de la pista (o antes de la cadena de plugins a medir). Configúralo en:
   - `Routing Mode:` **Tap Sender (Pre-FX)**
   - `Tap Bus Channel:` **Bus 1**
   *(El indicador mostrará `SENDING TAP...`. Esta instancia no consume CPU de análisis; simplemente retransmite la señal limpia).*
2. **Paso 2 (Analizador Post-FX):** Inserta otra instancia de PluginCheq al final de la cadena de efectos (o en el Master). Configúralo en:
   - `Routing Mode:` **Plugin Tap Bus (Ableton)**
   - `Tap Bus Channel:` **Bus 1**
3. **Resultado:** El analizador mostrará en verde **`TAP BUS: LOCKED`**. 
   - `REF (Clean)` recibe la señal anterior a los plugins.
   - `MEAS (Proc)` recibe la señal posterior tras pasar por la cadena.
   - El botón **`⇄ Swap`** te permite invertir quién es Ref y quién es Meas si fuera necesario.

---

### Método B: "DAW Sidechain" (Para Reaper, Cubase, Studio One)
Para DAWs que permiten enviar canales 3/4 o envíos auxiliares directos al Sidechain del plugin:
1. En PluginCheq selecciona `Routing Mode:` **DAW Sidechain**.
2. Envía la señal pre-procesamiento al bus Sidechain del plugin.
3. El indicador mostrará **`SIDECHAIN: OK`** en verde.

---

### Método C: "Internal Generator" (Para Standalone o pruebas con hardware)
Si estás usando la versión **Standalone** o deseas calibrar una interfaz, cable de audio o procesador externo:
1. En PluginCheq selecciona `Routing Mode:` **Internal Generator**.
2. En la barra de controles selecciona el estímulo en el combo `Generator`:
   - **Pink Noise:** Ruido rosa calibrado (−3 dB/octava), ideal para transferencia de sistemas acústicos y altavoces.
   - **White Noise:** Energía plana por hercio, ideal para filtros digitales y EQs rápidos.
   - **Sine Sweep:** Barrido senoidal logarítmico continuo de 20 Hz a 20 kHz.
   - **Multitone:** 31 frecuencias estándar ISO en 1/3 de octava con fases descorrelacionadas.
3. Ajusta el nivel con el control `Gen Gain (dB)` (por defecto a −18 dBFS).

---

## 4. GUÍA DIDÁCTICA DE LA INTERFAZ DE USUARIO

La interfaz de PluginCheq está organizada en dos niveles de profundidad para ofrecer una experiencia limpia y didáctica sin sacrificar el poder analítico profesional:

### Nivel 1: Barra Principal Didáctica
- **`⚡ Auto-Align Delay`:** Al hacer click, el algoritmo *GCC-PHAT* calcula el retardo en milisegundos y muestras entre la señal limpia y la procesada, y aplica la compensación de fase automáticamente. La curva de fase se aplanará instantáneamente eliminando la pendiente de retardo.
- **`Freeze`:** Congela el análisis en pantalla para inspeccionar curvas sin que cambien con la música.
- **`Reset`:** Reinicia los acumuladores espectrales para comenzar una nueva medición promediada limpia.
- **`Smoothing (Suavizado):`** Permite elegir entre lectura cruda (*Off*) o suavizado fraccional de octava (*1/12 Oct*, *1/3 Oct*, etc.) para lectura rápida de curvas de EQ.
- **`Save A / Save B`:** Guarda instantáneas comparativas de dos estados o procesadores distintos (Curva A en gris claro, Curva B en naranja).
- **`⚙ Advanced`:** Abre o cierra el cajón inferior de configuración matemática avanzada.
- **`? (Ayuda)`:** Despliega una tarjeta flotante de ayuda rápida en pantalla.

---

### Nivel 2: Cajón de Ajustes Avanzados (Advanced Drawer)
Al activar **`⚙ Advanced`**, se despliegan los controles de metrología profunda:
- **`FFT Size:`** Desde 1024 (muy rápido, ideal para transitorios y agudos) hasta 16384 (resolución milimétrica en frecuencias sub-graves de 20 a 100 Hz).
- **`Window:`** Ventana espectral (*Blackman-Harris* con −92 dB de rechazo de lóbulos para máxima precisión, *Hann*, o *Rectangular*).
- **`Averaging Mode & Alpha:`** Modo Exponencial (ajuste continuo con factor Alpha de velocidad) o Lineal (promedio exacto de $N$ tramas).
- **`Unwrap Phase:`** Desenrolla la fase eliminando los saltos visuales de ±180° para ver el retardo de grupo continuo.
- **`Show H2:`** Superpone el estimador $H_2$ en amarillo. Donde $H_1$ y $H_2$ se separan, existe ruido no correlacionado en la cadena.
- **`Manual Delay Comp (ms):`** Slider para micro-ajustar el retardo de análisis en milisegundos.

---

## 5. INTERPRETACIÓN DE LOS GRÁFICOS

Los 3 paneles comparten el mismo eje horizontal de frecuencia logarítmico (20 Hz a 20 kHz):

1. **MAGNITUDE (H1) [dB]:**
   - **Línea en 0 dB:** No hay cambio de amplitud entre entrada y salida.
   - **Por encima de 0 dB:** El procesador está amplificando esas frecuencias (boost).
   - **Por debajo de 0 dB:** El procesador está atenuando esas frecuencias (corte o filtro).
2. **PHASE [Grados]:**
   - Indica el desplazamiento temporal relativo por frecuencia.
   - Una línea horizontal plana en 0° (tras presionar *Auto-Align Delay*) indica un sistema de fase lineal.
   - Rotaciones o pendientes residuales revelan la distorsión de fase propia de filtros IIR, ecualizadores analógicos o inductores.
3. **COHERENCE ($\gamma^2$) [0.0 a 1.0]:**
   - **1.0 (Verde sólido):** La relación entre la señal limpia y la procesada es 100% lineal y matemáticamente perfecta. La medición es totalmente confiable.
   - **Menor a 0.7:** Presencia de ruido, saturación/distorsión no lineal, o falta de energía en esa banda.

### Cursor Interactivo y Delta Cursor
- **Mover el ratón:** Muestra en tiempo real la frecuencia exacta, ganancia en dB, fase y coherencia en ese punto.
- **Click izquierdo:** Clava un cursor de referencia fijo. Al mover el ratón ahora verás la diferencia delta exacta ($\Delta \text{ dB}, \Delta^\circ$) respecto a la frecuencia fijada.
- **Click derecho:** Quita el cursor fijo.

---

## 6. UBICACIÓN DE ARCHIVOS GENERADOS

- **Plugin VST3 (Windows x64):**
  `C:\Users\tomas\.gemini\antigravity\scratch\dsp-analysis-toolkit\build-vst3\PluginCheq_artefacts\Release\VST3\PluginCheq.vst3`
- **Ejecutable Standalone:**
  `C:\Users\tomas\.gemini\antigravity\scratch\dsp-analysis-toolkit\build-vst3\PluginCheq_artefacts\Release\Standalone\PluginCheq.exe`
