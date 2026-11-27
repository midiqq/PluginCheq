# Roadmap de Desarrollo — PluginCheq

El desarrollo de **PluginCheq** sigue un plan de ingeniería dividido en fases bien delimitadas:

---

## 🏁 Fase 1: Motor de Función de Transferencia Dual-Channel [Completada ✅]
- [x] Arquitectura de paso de audio transparente (bit-accurate passthrough).
- [x] Motor FFT Radix-2 en doble precisión con tablas precomputadas.
- [x] Estimador insesgado $H_1(f)$ y cálculo de coherencia ordinaria $\gamma^2(f)$.
- [x] Gráficos sincronizados de Magnitud (dB), Fase (Wrapped/Unwrapped) y Coherencia.
- [x] Conectividad lock-free multihilo (RingBuffer y TripleBuffer).

## ⚡ Fase 2: Sincronización Temporal y Suavizado Espectral [Completada ✅]
- [x] Estimador de retardo sub-muestra mediante GCC-PHAT y búsqueda sinc de sección áurea.
- [x] Botón `Auto-Align Delay` para compensación automática de fase.
- [x] Suavizado espectral fraccional de octava en $O(N)$ (Off a 1/1 Oct).
- [x] Bus de comunicación entre instancias `InterPluginBus` para Ableton Live y FL Studio.
- [x] Snapshots de comparación A/B y cursor delta anclado.

## 🔬 Fase 3: Laboratorio Dinámico y Armónico (Estilo PluginDoctor) [Completada ✅]
- [x] **Pestaña Dynamics:** Curva de transferencia entrada vs salida, guía unitaria 1:1, detección de umbral y ratio de compresión.
- [x] **Pestaña Harmonics / THD:** Ventana Blackman-Harris de 7 términos (-180 dB de piso), detección automática de armónicos H2..H8, cálculo de THD % y THD+N.
- [x] Eliminación de formato Standalone para concentrar la optimización en VST3.
- [x] Solución de problemas visuales y compatibilidad HiDPI en Ableton Live.

---

## 🚀 Fase 4: Generador de Excitación Automatizado [Próxima]
- [ ] Modos de excitación interna controlada: barrido senoidal logarítmico (Log Sine Sweep) y ráfagas de tonos (*Tone Bursts*).
- [ ] Medición automatizada de curvas de compresión y ataque/relajación sin pistas de audio externas.
- [ ] Curva continua de THD en función de la frecuencia (THD vs Freq Sweep).

## 🎛️ Fase 5: Integración de Presets y Calibración Avanzada
- [ ] Exportación de curvas de medición en CSV / JSON y captura gráfica en PNG vectorial.
- [ ] Presets de medición recomendados según el tipo de procesador (EQ Analógico, Compresor VCA/Opto, Saturador de Cinta).
- [ ] Compensación de no linealidades de convertidores hardware (calibración en bucle cerrado).
