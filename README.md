# PluginCheq

**PluginCheq** es un laboratorio de medición y caracterización DSP en tiempo real para DAWs (VST3 / Standalone), diseñado con una arquitectura lock-free de precisión científica y una estética minimalista oscura.

## Estado actual: Fase 1 — Transfer Function

- **Dual-Channel Transfer Function:** Comparación en tiempo real entre **Reference** y **Measurement** mediante entrada **Main** + **Sidechain** (con botón *Swap Ref / Meas* y selección de canal L / R / Mid).
- **Passthrough Bit-Accurate:** La señal principal atraviesa el plugin sin alteración ni coloración.
- **Estimadores $H_1(f)$ y $H_2(f)$:**
  - **Magnitude (dB)** en escala logarítmica (`20 Hz – 20 kHz`).
  - **Phase (deg)** envuelta y desenrollada (*Wrapped / Unwrapped*).
  - **Coherence ($\gamma^2 \in [0, 1]$)** para validar la relación lineal y detectar ruido o falta de excitación.
- **Arquitectura Real-Time Safe:** SPSC Lock-Free Ring Buffers (`processBlock` $\rightarrow$ `AnalysisWorker`) y Triple Buffer Lock-Free (`AnalysisWorker` $\rightarrow$ GUI).

## Estructura del proyecto

```text
├── CMakeLists.txt
├── src/
│   ├── dsp/        # Núcleo matemático C++20 puro (sin dependencias JUCE)
│   ├── plugin/     # AudioProcessor VST3/Standalone + AnalysisWorker
│   └── ui/         # Interfaz minimalista (Apple-inspired dark + light-gray)
└── tests/          # Suite de validación científica en Catch2 v3
```

## Enrutamiento rápido en DAW (Modo Sidechain)

1. Inserta **PluginCheq** al final de la cadena (o en el punto posterior) que deseas medir (**Measurement**).
2. Envía la señal previa al procesamiento (**Reference**) hacia la entrada **Sidechain** del plugin (Pre-FX).
3. Si necesitas invertir qué señal entra por Main y cuál por Sidechain, usa el botón **Swap Ref / Meas** en la barra superior.

## Compilación y Pruebas

### 1. Pruebas científicas del núcleo DSP (Catch2)
Funciona con GCC / Clang / MSVC y CMake 3.24+:
```bash
cmake -B build-tests -DCMAKE_BUILD_TYPE=Release -DBUILD_PLUGIN=OFF
cmake --build build-tests -j 8
ctest --test-dir build-tests --output-on-failure
```

### 2. Plugin VST3 y Standalone (Windows + MSVC / Visual Studio 2022)
JUCE 8 requiere Visual Studio 2022 (MSVC + Windows SDK) en Windows:
```powershell
cmake -B build-vst3 -G "Visual Studio 17 2022" -A x64
cmake --build build-vst3 --config Release
```
Los artefactos se generan en `build-vst3/PluginCheq_artefacts/Release/`.
