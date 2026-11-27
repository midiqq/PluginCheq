# PluginCheq — Arquitectura Técnica de Precisión DSP

Documento de referencia técnica sobre el diseño interno, la topología de hilos, los principios matemáticos y las garantías de tiempo real de **PluginCheq**.

---

## 1. Topología de Hilos y Seguridad en Tiempo Real (Real-Time Safety)

PluginCheq está estructurado en tres niveles de ejecución desacoplados mediante estructuras de datos concurrentes sin bloqueos (lock-free):

```text
┌────────────────────────────────────────────────────────────────────────┐
│                          Audio Host (DAW)                              │
│                                                                        │
│   processBlock() ───[ Bit-Accurate Passthrough ]───► Audio Output     │
│         │                                                              │
│         ├────────► [ Main Signal ]       ──┐                           │
│         └────────► [ Sidechain / Bus ]   ──┴─► Lock-Free SPSC Queue    │
└───────────────────────────────────────────────────────┬────────────────┘
                                                        │ (RingBuffer.h)
                                                        ▼
┌────────────────────────────────────────────────────────────────────────┐
│                   Analysis Worker Thread (Background)                  │
│                                                                        │
│   - Radix-2 64-bit FFT Engine (FftEngine.h)                           │
│   - Transfer Function Engine: H1(f), H2(f), Coherence γ²               │
│   - Sub-sample GCC-PHAT Delay Finder (DelayFinder.h)                   │
│   - 1/24 to 1/1 Octave Spectrum Smoothing (O(N) Prefix Sums)           │
│   - Dynamics Engine: Threshold & Ratio piecewise linear regression     │
│   - Harmonic Engine: 7-term Blackman-Harris, H1..H8 extraction, THD    │
└───────────────────────────────────────────────────────┬────────────────┘
                                                        │ (TripleBuffer.h)
                                                        ▼
┌────────────────────────────────────────────────────────────────────────┐
│                         UI Thread (60 FPS)                             │
│                                                                        │
│   - Monotonically updated snapshot consumption (Zero allocations)      │
│   - High-DPI Vector Graphics via JUCE Graphics Context                 │
│   - Pinned delta readout cursors & A/B snapshot comparisons            │
└────────────────────────────────────────────────────────────────────────┘
```

### Garantías del Hilo de Audio (`processBlock`):
1. **Zero Allocations:** No existen llamadas a `malloc`, `free`, `new`, `delete`, ni redimensionamiento de vectores (`std::vector::resize` o `push_back`).
2. **Zero Locks:** No se emplean `std::mutex`, `std::unique_lock`, ni primitivas bloqueantes del sistema operativo.
3. **Cero Coloración:** La señal principal se copia directamente a la salida del plugin con precisión binaria exacta (bit-accurate passthrough).

---

## 2. Motores Matemáticos de Medición

### 2.1 Función de Transferencia Dual-Channel ($H_1$, $H_2$, Coherencia)
- **Densidades Espectrales Cruzadas:**
  $$\hat{S}_{xx}(f) = \alpha \hat{S}_{xx}(f) + (1-\alpha) |X(f)|^2$$
  $$\hat{S}_{yy}(f) = \alpha \hat{S}_{yy}(f) + (1-\alpha) |Y(f)|^2$$
  $$\hat{S}_{xy}(f) = \alpha \hat{S}_{xy}(f) + (1-\alpha) \big( X^*(f) \cdot Y(f) \big)$$
- **Estimador $H_1(f)$ (Insesgado ante ruido en la salida):**
  $$H_1(f) = \frac{\hat{S}_{xy}(f)}{\hat{S}_{xx}(f)}$$
- **Coherencia Ordinaria $\gamma^2(f) \in [0, 1]$:**
  $$\gamma^2(f) = \frac{|\hat{S}_{xy}(f)|^2}{\hat{S}_{xx}(f) \cdot \hat{S}_{yy}(f)}$$
  Permite validar científicamente qué porción de la señal de salida se debe causal y linealmente a la señal de entrada, detectando bandas con baja relación señal-ruido o comportamiento no lineal.

### 2.2 Estimador de Retardo Sub-Muestra (GCC-PHAT)
- **Transformada de Fase Generalizada:**
  $$R_{xy}(\tau) = \mathcal{F}^{-1} \left\{ \frac{X^*(f) \cdot Y(f)}{|X^*(f) \cdot Y(f)|} \right\}$$
- **Interpolación Sinc Sub-Muestra:**
  Alrededor del pico entero $\tau_{\text{int}}$, se evalúa la función de interpolación continua mediante una ventana sinc ponderada y búsqueda de sección áurea, logrando una precisión sub-muestra de hasta $\pm 0.05$ muestras.
- **Compensación de Fase:**
  Multiplicación en frecuencia por $e^{j 2 \pi f \cdot \Delta \tau}$ para aplanar la pendiente lineal inducida por el tiempo de propagación.

### 2.3 Suavizado Espectral Fraccional de Octava ($O(N)$)
- Utiliza una tabla de suma de prefijos circular calculada sobre los bines de potencia en dominio logarítmico, reduciendo la complejidad algorítmica de $O(N^2)$ a $O(N)$ estricto por cuadro.

### 2.4 Motor de Dinámica (Dynamics Engine)
- Medición de la curva de transferencia Entrada vs Salida (dBFS).
- Estimación robusta del umbral (*threshold*) y relación de compresión (*ratio*) mediante regresión lineal segmentada por mínimos cuadrados en los tramos pre-umbral y post-umbral.

### 2.5 Motor de Distorsión Armónica (Harmonic Engine)
- **Ventana Blackman-Harris de 7 términos:**
  Con coeficientes optimizados para lograr una atenuación de lóbulos secundarios de $-180\text{ dB}$, impidiendo que fugas espectrales de la fundamental contaminen la medición de armónicos de bajo nivel (H2 a H8).
- **Interpolación Parabólica en Dominio Lineal:**
  Ajuste del vértice del pico en amplitud lineal para cuantificar la frecuencia y amplitud fundamental sin sesgos de muestreo de bin.
- **Cálculo de THD y THD+N:**
  $$\text{THD} = \frac{\sqrt{\sum_{k=2}^{8} |A_k|^2}}{|A_1|}$$
  $$\text{THD+N} = \frac{\sqrt{P_{\text{total}} - |A_1|^2}}{|A_1|}$$

---

## 3. Bus de Intercomunicación (`InterPluginBus`)
Para superar la falta de ruteo de sidechain interno en pistas individuales en DAWs como Ableton Live:
- Implementa un búfer circular lock-free en memoria de proceso accesible por cualquier instancia de PluginCheq en la misma sesión.
- Dispone de 8 canales independientes para que múltiples cadenas de plugins puedan monitorearse simultáneamente sin colisiones de datos.
