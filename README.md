# PluginCheq

[![CI Tests](https://img.shields.io/badge/Catch2%20Tests-13%2F13%20Passed-brightgreen.svg)](tests/)
[![Format](https://img.shields.io/badge/Format-VST3%20(64--bit)-blue.svg)](dist/)
[![C++](https://img.shields.io/badge/Standard-C%2B%2B20-orange.svg)](CMakeLists.txt)
[![UI](https://img.shields.io/badge/Style-Apple%20Pro%20Dark-darkgray.svg)](src/ui/)

**PluginCheq** es un laboratorio de medición y caracterización DSP en tiempo real para DAWs (formato VST3 en Windows), diseñado con precisión metrológica científica, rendimiento garantizado de 60 FPS y una estética minimalista profesional.

---

## 🌟 Características Principales

1. **🔬 Pestaña `Linear / EQ` (Función de Transferencia Dual-Channel):**
   - Comparación en tiempo real entre señal de entrada (**Reference**) y procesada (**Measurement**).
   - Estimador insesgado $H_1(f)$ de Magnitud (dB) y Fase (Wrapped / Unwrapped).
   - Coherencia ordinaria ($\gamma^2 \in [0, 1]$) para validar linealidad e identificar bandas con ruido o atenuación excesiva.
   - **`Auto-Align Delay`:** Compensación de latencia sub-muestra automática mediante correlación cruzada GCC-PHAT.
   - Suavizado fraccional de octava en tiempo lineal $O(N)$ ($1/24$ a $1/1$ Oct).

2. **📊 Pestaña `Dynamics` (Curva de Transferencia y Compresión):**
   - Curva de transferencia Entrada vs Salida (dBFS) con guía de ganancia unitaria 1:1.
   - Sombreado de área de reducción de ganancia.
   - Detección automática del umbral (*Threshold*) y relación de compresión (*Ratio*).

3. **🎶 Pestaña `Harmonics / THD` (Distorsión y Espectro de Alta Precisión):**
   - Ventana Blackman-Harris de 7 términos con supresión de lóbulos a $-180\text{ dB}$.
   - Detección automática de la fundamental y de los armónicos del 2° al 8° orden (H2 a H8).
   - Cálculo en tiempo real de **THD (%)**, **THD (dB)** y **THD+N**.

4. **⚡ Arquitectura Lock-Free de Alta Velocidad:**
   - **Passthrough Bit-Accurate:** Cero coloración y cero latencia agregada a la señal de audio del DAW.
   - Estricto *Real-Time Safety*: cero asignaciones de memoria y cero mutexes en el hilo de audio.
   - Colas SPSC y búfer triple para transporte fluido a 60 FPS hacia la interfaz gráfica.

5. **🎛️ Conectividad Universal para DAWs:**
   - **Bus de 2 Instancias (`InterPluginBus`):** Permite medir cadenas en Ableton Live y FL Studio sin necesidad de ruteos complejos de sidechain.
   - **Sidechain Nativo:** Compatible con el ruteo estándar de Reaper, Cubase, Studio One.

---

## 📁 Estructura del Repositorio

```text
├── dist/                     # Paquete listo para el usuario final (VST3 + instalador)
│   ├── PluginCheq.vst3       # Bundle VST3 de producción
│   ├── INSTALAR_VST3.bat     # Instalador en 1 click para Windows
│   └── MANUAL_DE_USUARIO.md  # Guía de usuario rápida
├── docs/                     # Documentación técnica y operativa completa
│   ├── MANUAL_DE_USUARIO.md  # Manual completo de funciones y controles
│   ├── ARCHITECTURE.md       # Arquitectura DSP, topología de hilos y fórmulas
│   ├── ROUTING_GUIDE.md      # Guía ilustrada de ruteo en Ableton, FL Studio y Reaper
│   └── AGENTS.md             # Organización del equipo multi-agente y protocolos
├── scripts/                  # Scripts de automatización en PowerShell
│   ├── build.ps1             # Compilación en 1 comando (VST3 + Tests)
│   ├── test.ps1              # Ejecución de la suite de pruebas Catch2
│   └── package.ps1           # Empaquetado hacia dist/
├── src/
│   ├── dsp/                  # Motores matemáticos C++20 puros (sin dependencias JUCE)
│   ├── plugin/               # AudioProcessor VST3 y AnalysisWorker
│   └── ui/                   # Interfaz visual vectorial y LookAndFeel
├── tests/                    # Suite de 13 pruebas unitarias y de estrés en Catch2 v3
├── CMakeLists.txt            # Configuración principal de compilación CMake
├── CONTRIBUTING.md           # Estándares de desarrollo y seguridad en tiempo real
└── ROADMAP.md                # Fases del proyecto y próximos hitos
```

---

## 🚀 Inicio Rápido para Desarrolladores

### Requisitos:
- Windows 10/11 x64
- Visual Studio 2022 (MSVC v143 + Windows 10/11 SDK)
- CMake 3.24 o superior

### 1. Compilación
```powershell
.\scripts\build.ps1
```

### 2. Ejecutar Pruebas
```powershell
.\scripts\test.ps1
```

### 3. Empaquetar Distribución
```powershell
.\scripts\package.ps1
```

---

## 👥 Equipo de Desarrollo Multi-Agente
Este proyecto se gestiona mediante un equipo de subagentes especializados:
- 🎨 **`ui_designer`:** Diseño visual, LookAndFeel y renderizado a 60 FPS.
- ⚡ **`dsp_backend_agent`:** Motores matemáticos, FFT, coherencia y tiempo real.
- 🧪 **`qa_testing_agent`:** Validación científica, Catch2 y pruebas de concurrencia.
- 🎧 **`user_persona_agent`:** Usabilidad, flujos de trabajo en DAWs y documentación didáctica.

Consulta [docs/AGENTS.md](docs/AGENTS.md) para más detalles.

---

## 📄 Licencia
Desarrollado bajo estándares abiertos para la comunidad de ingeniería de audio.
