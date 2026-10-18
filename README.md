# Protección de frecuencia para chasis M3129 (tri-sync 15/24/31 kHz)

Placa de reemplazo para el módulo de selección de modo **IC6 (3129X001, PIC16C54C)** del chasis M3129, basada en un Arduino Nano. Se enchufa en el mismo conector de 7 pines del módulo original, así que la modificación es reversible.

El esquemático de referencia es el del Makvision **M3129DF-TS / M3129DB** (Huai I Electronics).

## Problema que resuelve

Con frecuencias horizontales intermedias (fuera de 15.7, 24 y 31.5 kHz), el módulo original no logra decidir el modo y el relé conmuta sin parar. Eso hace oscilar el B+ y castiga la salida horizontal (Q36) y los contactos del relé.

Esta placa:

- Mide la frecuencia horizontal y solo acepta EGA o VGA dentro de ventanas válidas.
- Ante cualquier frecuencia fuera de rango, o sin señal, deja el chasis en **CGA**, el modo de B+ más bajo (~72 V).
- **Baja rápido y sube lento:** si la frecuencia cae por debajo del modo actual, pasa a CGA en unos 60 ms; para subir de modo exige 500 ms de señal estable y al menos 2 s desde el último cambio.
- Usa histéresis para que una señal en el borde de una ventana no haga saltar el modo.

### Por qué CGA es el estado seguro

El ancho de la imagen y el alto voltaje dependen de la relación entre el B+ y la frecuencia horizontal. Un B+ alto con una frecuencia baja aumenta la corriente en el yugo, el pico de voltaje en Q36 y el alto voltaje: esa es la condición peligrosa. Con B+ bajo y una frecuencia que no engancha, el oscilador corre libre como cuando no hay señal, una condición para la que el chasis está diseñado. La imagen puede verse inestable, pero sin riesgo.

## Esquema de conexión

```
  Conector IC6 en la placa principal          Arduino Nano
  (módulo 3129X001 retirado)                  (ATmega328P, 5 V, 16 MHz)
  ┌───────────────────────┐                   ┌──────────────────┐
  │ 1  salida a Q17       ├───────────────────┤ D2               │
  │ 2  salida a Q18       ├───────────────────┤ D3               │
  │ 3  salida a Q21       ├───────────────────┤ D4               │
  │ 4  entrada vertical   ├── sin conectar    │                  │
  │ 5  entrada horizontal ├───────────────────┤ D5  (entrada T1) │
  │ 6  +5 V               ├───────────────────┤ 5V               │
  │ 7  GND                ├───────────────────┤ GND              │
  └───────────────────────┘                   └──────────────────┘


  Resistencias de arranque (fijan CGA mientras el Nano inicia):

        D2 ───[ 10K ]─── GND
        D3 ───[ 10K ]─── +5 V
        D4 ───[ 10K ]─── +5 V
```

| Conector IC6 | Función | Arduino Nano |
|---|---|---|
| 1 | Salida a Q17 | D2 |
| 2 | Salida a Q18 | D3 |
| 3 | Salida a Q21 | D4 |
| 4 | Entrada vertical | Sin conectar |
| 5 | Entrada horizontal | D5 (T1) |
| 6 | +5 V | 5V |
| 7 | GND | GND |

## Tabla de verdad

| Modo | Pin 1 (Q17) | Pin 2 (Q18) | Pin 3 (Q21) | B+ aproximado |
|---|---|---|---|---|
| CGA (15 kHz) | 0 | 1 | 1 | 72 V |
| EGA (24 kHz) | 1 | 1 | 0 | 110 V |
| VGA (31 kHz) | 1 | 0 | 1 | 137 V |

Esta tabla proviene de una hoja agregada al esquemático, no del fabricante. **Verifícala** midiendo los pines 1, 2 y 3 del módulo original en cada modo antes de instalar la placa, y corrige la constante `TABLA` del código si no coincide.

**No uses las otras combinaciones de los pines 1-3.** Solo tres están definidas; las demás activarían mezclas de selección de B+, relés y frecuencia que el chasis no espera.

## Parámetros del firmware

| Constante | Valor | Función |
|---|---|---|
| `VENTANA` | 14.8–16.6 / 23.5–25.5 / 30.0–32.5 kHz | Frecuencias aceptadas por modo |
| `HISTERESIS_HZ` | 300 Hz | Ensancha la ventana del modo actual |
| `COMPUERTA_MS` | 20 ms | Tiempo de conteo por medición |
| `CONFIRMA_SUBIR` | 500 ms | Señal estable requerida para subir de modo |
| `CONFIRMA_BAJAR` | 60 ms | Tiempo para caer a CGA |
| `PAUSA_MIN_RELE` | 2000 ms | Tiempo mínimo antes de volver a subir |

Ajusta las ventanas a lo que tu chasis realmente engancha.

### Frecuencias intermedias (experimental)

El código incluye, comentada, una tabla alternativa que redondea cualquier frecuencia al **modo inferior** (por ejemplo 18 kHz → CGA, 29 kHz → EGA). Nunca se redondea hacia arriba, porque eso pondría un B+ demasiado alto para la frecuencia.

Esta opción solo cambia el modo elegido; no mueve la frecuencia central del oscilador de IC9 (3129X002). La imagen se estabiliza solo si la frecuencia cae dentro del rango de captura del AFC de ese modo, y aun así se verá más angosta. Si no engancha, la imagen queda inestable pero sin riesgo.

## Instalación y pruebas

1. Programa el Nano **fuera del monitor**. No conectes el USB a una computadora con el Nano instalado y el monitor encendido.
2. Mide y anota la tabla de verdad del módulo original.
3. Retira el módulo 3129X001 e instala la placa.
4. Primera prueba con **lámpara en serie** en la entrada de AC y una fuente de video de 15 kHz.
5. Confirma con osciloscopio que la señal del pin 5 sea lógica de 0–5 V.
6. Prueba 15, 24 y 31 kHz y verifica el B+ en cada modo.
7. Prueba las frecuencias intermedias problemáticas: el relé debe quedar quieto en CGA.
8. Con una frecuencia intermedia aplicada durante 10–15 minutos, comprueba que la frecuencia en el colector de Q36 se mantenga cerca de 15–16 kHz y que su disipador no caliente más que en uso normal.

## Advertencia

Este monitor maneja más de 300 VDC en el primario de la fuente y decenas de kilovoltios en el ánodo del tubo, que puede conservar carga mucho tiempo después de apagado. Trabaja con transformador de aislamiento, descarga los capacitores y el tubo antes de manipular la placa, y usa esta modificación bajo tu propia responsabilidad.
