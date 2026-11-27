# Guía de Enrutamiento en DAWs — PluginCheq

Esta guía detalla paso a paso cómo conectar **PluginCheq** para medir ecualizadores, compresores, saturadores o cadenas completas en los DAWs más comunes.

---

## 💡 Concepto Rápido: ¿Cómo Funciona la Medición?
Para graficar la curva exacta de un plugin o procesador de audio, PluginCheq necesita comparar dos señales:
1. **Reference (Limpia):** La señal de audio antes de entrar a tus efectos.
2. **Measurement (Procesada):** La señal de audio después de salir de tus efectos.

PluginCheq analiza ambas en tiempo real y grafica la diferencia con precisión de laboratorio.

---

## 🎛️ Método 1: Bus de 2 Instancias (Recomendado para Ableton Live y FL Studio)
*No requiere configurar ruteos complejos en el mezclador ni pistas de retorno.*

```text
TU PISTA (Ableton Live / FL Studio):
┌────────────────────┐     ┌────────────────────────┐     ┌────────────────────┐
│ 1. PluginCheq      │ ──► │ Tus Plugins a medir    │ ──► │ 2. PluginCheq      │
│ (Tap Sender Pre-FX)│     │ (FabFilter, Comp, etc.)│     │ (Receptor / GUI)   │
└─────────┬──────────┘     └────────────────────────┘     └─────────▲──────────┘
          │                                                         │
          └═══════════════ Bus Invisible (InterPluginBus) ══════════┘
                         (Transmite audio limpio sin latencia)
```

### Paso a paso:
1. Inserta el **primer PluginCheq** como el primer efecto de tu pista (antes de los plugins que quieres analizar).
2. En la barra superior, configura:
   - **`Routing Mode:`** `Tap Sender (Pre-FX)`
   - Verás el badge azul **`SENDING TAP...`**. Este plugin solo captura y transmite la señal limpia.
3. Inserta tus plugins de procesamiento en el medio de la pista.
4. Inserta el **segundo PluginCheq** al final de la cadena de efectos.
5. En la barra superior, configura:
   - **`Routing Mode:`** `Plugin Tap Bus (Ableton)`
   - El badge cambiará a verde: **`TAP BUS: LOCKED`**.
6. ¡Listo! Abre la interfaz del segundo PluginCheq:
   - El vúmetro **REF** mostrará el audio limpio que entra a tu cadena.
   - El vúmetro **MEAS** mostrará el audio procesado que sale de tus efectos.
   - En la pestaña **Linear / EQ**, haz click en **`⚡ Auto-Align Delay`** para compensar cualquier latencia y ver la curva perfecta.

---

## 🎚️ Método 2: Instancia Única vía Sidechain Nativo (Reaper, Cubase, Studio One)
*Ideal para DAWs que soportan ruteo flexible de pines o envíos directos de sidechain.*

```text
               ┌────────────────────────────────────────────────────────┐
               │ Pista de Audio Original                                │
               └───────────┬────────────────────────────────┬───────────┘
                           │ (Audio principal)              │ (Envío Pre-FX Sidechain)
                           ▼                                │
                 ┌──────────────────────┐                   │
                 │ Tus Plugins a medir  │                   │
                 └─────────┬────────────┘                   │
                           │                                │
                           ▼ (Measurement)                  ▼ (Reference)
                 ┌──────────────────────────────────────────────────────┐
                 │ PluginCheq (Instancia única - DAW Sidechain)         │
                 └──────────────────────────────────────────────────────┘
```

### Paso a paso (ejemplo en Reaper):
1. Inserta tus plugins de procesamiento en la pista.
2. Inserta **PluginCheq** al final de la pista.
3. En el routing de la pista, crea un envío de los canales 1/2 (Pre-FX) hacia los canales 3/4 de PluginCheq (Sidechain Input).
4. En PluginCheq, selecciona **`Routing Mode:`** `DAW Sidechain`.
5. El indicador mostrará **`SIDECHAIN: OK`** en verde.

---

## 🖥️ Configuración de Pantalla HiDPI en Ableton Live (Solución a Gráficos Borrosos)
Si trabajas en Windows con escalado de pantalla (125%, 150%, etc.):
1. Abre tu cadena de efectos en Ableton Live donde se encuentra PluginCheq.
2. Haz **click derecho sobre la barra de título superior del recuadro del plugin** en Ableton.
3. **Desmarca** la casilla **`Auto-scale plug-in window`** (Auto-escalar ventana de plug-in).
4. El plugin pasará instantáneamente a renderizado vectorial 1:1 ultra nítido.
