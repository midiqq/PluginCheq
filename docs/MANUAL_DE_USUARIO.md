# MANUAL DE USUARIO — PLUGINCHEQ

**PluginCheq** es un laboratorio de medición y caracterización DSP en tiempo real para DAWs (formato VST3 en Windows), diseñado con precisión metrológica y una estética minimalista profesional.

---

## 1. ¿Qué es y qué mide PluginCheq?
PluginCheq compara en tiempo real la señal que entra a tus plugins (**Reference**) contra la señal que sale de ellos (**Measurement**).

El plugin cuenta con tres modos de análisis accesibles desde los botones de la barra superior:

### 1.1 Pestaña `Linear / EQ` (Función de Transferencia)
- **Magnitud (dB):** Muestra la curva de ecualización o balance tonal exacta que aplican tus plugins entre 20 Hz y 20 kHz.
- **Fase (Grados):** Muestra el desplazamiento de fase introducido por filtros y convertidores. Puede visualizarse envuelta (Wrapped: $\pm 180^\circ$) o desenrollada (Unwrapped).
- **Coherencia ($\gamma^2$ de 0 a 1):** Mide la linealidad y calidad de la medición. Un valor cercano a 1.0 indica que el audio de salida proviene fielmente de la entrada; caídas de coherencia señalan ruido, compresión agresiva o falta de señal.

### 1.2 Pestaña `Dynamics` (Curva de Transferencia Dinámica)
- Grafica el nivel de entrada (dBFS) versus nivel de salida (dBFS).
- Muestra una línea guía de ganancia unitaria 1:1 y resalta en sombreado la zona de compresión.
- Detecta y muestra automáticamente en badges el umbral estimado (**Threshold**) y la relación de compresión (**Ratio**).

### 1.3 Pestaña `Harmonics / THD` (Distorsión Armónica)
- Analizador espectral de altísima resolución con piso dinámico hasta -140 dBFS.
- Localiza automáticamente la frecuencia fundamental del tono de entrada.
- Identifica y etiqueta los armónicos del 2° al 8° orden (H2 a H8), mostrando su nivel relativo en dBc.
- Calcula en tiempo real la distorsión armónica total (**THD %** y **THD dB**) y la distorsión más ruido (**THD+N**).

---

## 2. Métodos de Conexión en tu DAW

### Método A: Doble Instancia con Bus Interno (Para Ableton Live y FL Studio)
1. Inserta un PluginCheq antes de los plugins a medir y configúralo en:  
   `Routing Mode: Tap Sender (Pre-FX)`.
2. Inserta un segundo PluginCheq después de los plugins a medir y configúralo en:  
   `Routing Mode: Plugin Tap Bus (Ableton)`.
3. Ambos plugins se sincronizan automáticamente por memoria compartida sin necesidad de configurar envíos en el DAW.

### Método B: Instancia Única con Sidechain (Reaper, Cubase, Studio One)
1. Inserta un único PluginCheq al final de tu cadena.
2. Envía la señal pre-efectos al canal de Sidechain de PluginCheq.
3. Configura `Routing Mode: DAW Sidechain`.

---

## 3. Controles Principales

- **`⚡ Auto-Align Delay`:** Detecta y compensa automáticamente la latencia introducida por tus plugins en fracciones de muestra, aplanando la curva de fase.
- **`Smoothing`:** Suavizado fraccional de octava (Off, 1/24, 1/12, 1/6, 1/3, 1/1 Oct) para obtener curvas legibles incluso con música compleja.
- **`Averaging (α)`:** Controla la velocidad de refresco del análisis espectral (Fast, Medium, Slow, Stable).
- **`Snapshots (A / B)`:** Permite congelar una curva de medición y contrastarla inmediatamente contra otra configuración.
- **`Swap`:** Invierte las entradas de Referencia y Medición en caso de haber ruteado los canales en orden inverso.
- **`Advanced`:** Despliega el cajón con controles de escala, rango dinámico (-12 a +24 dB), tipo de ventana espectral y canal analizado (L, R, Mid).

---

## 4. Nitidez en Pantallas HiDPI (Ableton Live)
Si la ventana del plugin se ve ligeramente difusa:
1. Haz click derecho sobre la barra superior de PluginCheq en la cadena de Ableton.
2. **Desmarca "Auto-scale plug-in window"** para disfrutar de gráficos vectoriales nítidos a resolución nativa.
