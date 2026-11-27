# GUÍA DE USUARIO: CÓMO FUNCIONA PLUGINCHEQ

---

## 1. LA IDEA FUNDAMENTAL (¿Qué hace y cómo piensa este software?)

Imagina que tienes una caja negra (por ejemplo: un ecualizador, un compresor o una cadena de plugins).

Para saber **exactamente qué le hace esa caja al sonido**, necesitas comparar dos cosas:
1. **La señal LIMPIA (Reference):** El audio tal como entra a la caja.
2. **La señal PROCESADA (Measurement):** El audio tal como sale de la caja.

**PluginCheq es el juez:** recibe las dos señales al mismo tiempo, las resta matemáticamente en tiempo real y te dibuja la diferencia exacta:
- Si el EQ subió +3 dB en 1 kHz, PluginCheq te muestra una curva subiendo +3 dB en 1 kHz.
- Si el compresor o saturador desfasa o retrasa el audio, PluginCheq te muestra los grados de fase y los milisegundos exactos de retardo.

> **Regla de oro:** PluginCheq **NUNCA** modifica el audio. El audio lo atraviesa intacto. Su único trabajo es escuchar y medir.

---

## 2. DÓNDE SE PONEN LOS PLUGINS (Los 2 métodos de conexión)

Hay dos maneras de usar PluginCheq según cómo te guste trabajar en tu DAW:

---

### MÉTODO 1: El método de las 2 instancias (Recomendado para Ableton Live y FL Studio)
*No requiere configurar ruteos de sidechain extraños en el DAW.*

Pones **DOS** PluginCheq en la misma pista: uno **antes** de tus plugins y otro **después**.

```text
 TU PISTA EN ABLETON:
 ─────────────────────────────────────────────────────────────────────────
 [ 1. PluginCheq ]  ──►  [ Tus Plugins: EQ, Comp, Saturador ]  ──►  [ 2. PluginCheq ]
   (EMISOR / TAP)                                                     (ANALIZADOR)
 ─────────────────────────────────────────────────────────────────────────
        │                                                                   ▲
        │═════════════════ Cable invisible por dentro ══════════════════════│
                 (Envía la señal limpia automáticamente)
```

#### Paso a paso exacto:
1. En tu pista, inserta el primer **PluginCheq**.
   - En la esquina superior izquierda pon: **`Routing Mode: Tap Sender (Pre-FX)`**.
   - Verás que dice en azul: `SENDING TAP...`.
   - *Este plugin solo captura el audio limpio antes de que entre a tus efectos.*
2. En medio, pon todos los plugins que quieras probar (FabFilter Pro-Q, un compresor, etc.).
3. Al final de la pista, inserta el segundo **PluginCheq**.
   - En la esquina superior izquierda pon: **`Routing Mode: Plugin Tap Bus (Ableton)`**.
   - Verás que el indicador cambia a verde: **`TAP BUS: LOCKED`**.
4. ¡Listo! Abre la ventana del segundo PluginCheq:
   - Verás que el vúmetro **`REF (Clean)`** se mueve con la señal limpia.
   - Verás que el vúmetro **`MEAS (Proc)`** se mueve con la señal procesada.
   - En el gráfico verás inmediatamente la curva exacta que están aplicando tus plugins.

---

### MÉTODO 2: El método de 1 sola instancia con Sidechain tradicional (Reaper, Cubase, Studio One, o Ableton con envíos)

Usas **UN SOLO** PluginCheq colocado al final de la cadena.

```text
 PISTA ORIGINAL ────────────────────────► Envío al Sidechain de PluginCheq (REF)
        │
        ▼
 [ Tus Plugins: EQ, Comp ]
        │
        ▼
 PISTA PROCESADA ───────────────────────► Entrada Principal de PluginCheq (MEAS)
```

#### Paso a paso:
1. Pon PluginCheq al final de la pista o en el Master.
2. En el plugin, pon: **`Routing Mode: DAW Sidechain`**.
3. En tu DAW, envía la señal de la pista antes de procesar hacia la entrada Sidechain de PluginCheq.
4. El indicador mostrará **`SIDECHAIN: OK`** en verde.

---

### MÉTODO 3: Modo Autónomo / Standalone (Sin DAW, calibrando hardware o cables)

Si abres **`PluginCheq.exe`** directamente en Windows sin abrir ningún DAW:
1. En el menú superior de la ventana configuras tu interfaz de audio (ej: Focusrite, Universal Audio).
2. En PluginCheq seleccionas: **`Routing Mode: Internal Generator`**.
3. En el selector **`Generator`**, eliges **Pink Noise** o **Sine Sweep**.
4. El plugin genera el sonido de prueba internamente y lo usa como Referencia para medir cualquier equipo analógico conectado a las entradas/salidas de tu interfaz.

---

## 3. CÓMO SE USA EN EL DÍA A DÍA (3 Botones mágicos)

Una vez que el audio está corriendo y los dos vúmetros (`REF` y `MEAS`) se mueven:

1. **`⚡ Auto-Align Delay` (El botón más importante):**
   Los plugins digitales y los convertidores agregan pequeñas muestras o milisegundos de latencia. Esa latencia hace que la curva de fase salga inclinada.  
   👉 **Haces click en `⚡ Auto-Align Delay`:** PluginCheq calcula el retardo exacto y lo compensa. La curva de fase se aplana inmediatamente a 0° y la medición queda perfecta.
2. **`Smoothing` (Suavizado):**
   Si la curva de EQ se ve muy ruidosa o con demasiados piquitos por la música real, cambia el selector `Smoothing` a **1/12 Oct** o **1/3 Oct**. La curva se volverá suave y clara como la pantalla de un ecualizador comercial.
3. **`Save A` y `Save B` (Comparar configuraciones):**
   - Ajustas tu compresor con ataque rápido → click en **`Save A`** (se congela una curva gris).
   - Ahora cambias el compresor a ataque lento → click en **`Save B`** (se dibuja una curva naranja).
   - Puedes ver las dos curvas superpuestas para comparar exactamente qué cambió.

---

## 4. QUÉ SIGNIFICA CADA GRÁFICO (Lectura fácil)

Los tres paneles van de 20 Hz (graves) a 20 kHz (agudos):

### 1. Panel de Arriba: MAGNITUDE (dB)
Es la respuesta en frecuencia.
- **En la línea de 0 dB:** El procesador no altera el volumen en esa frecuencia.
- **Curva arriba de 0 dB (+dB):** El procesador está subiendo volumen (boost).
- **Curva abajo de 0 dB (-dB):** El procesador está cortando volumen (cut / filtro).

### 2. Panel del Medio: PHASE (Grados)
Muestra si el procesador cambia la fase o la alineación en el tiempo.
- Una línea horizontal en 0° (después de apretar *Auto-Align Delay*) significa fase perfecta.
- Giros o curvas pronunciadas muestran los desfases que introducen los filtros analógicos o emulaciones vintage.

### 3. Panel de Abajo: COHERENCE (0 a 1)
Es el "semáforo de verdad" de la medición.
- **Cerca de 1.0 (Verde alto):** La medición es 100% matemática, lineal y confiable. Lo que ves en el gráfico es la verdad absoluta.
- **Por debajo de 0.5 (Cae a cero):** En esa zona hay mucho ruido de fondo, saturación no lineal, o la canción no tiene instrumentos tocando en esa frecuencia en ese instante.
