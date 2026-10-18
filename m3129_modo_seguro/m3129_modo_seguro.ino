/*
  Reemplazo del módulo de selección de modo 3129X001 (PIC16C54C)
  Makvision M3129 - placa con Arduino Nano (ATmega328P, 5 V, cristal 16 MHz)

  Conector del módulo original (7 pines):
    1 -> Q17   2 -> Q18   3 -> Q21
    4 = V in   5 = H in   6 = +5 V   7 = GND

  Conexiones al Nano:
    Pin 5 (H in)  -> D5 (entrada T1 del Timer1)
    Pin 1 (Q17)   -> D2
    Pin 2 (Q18)   -> D3
    Pin 3 (Q21)   -> D4
    Pin 6 (+5 V)  -> 5V
    Pin 7 (GND)   -> GND

  Resistencias de 10K para fijar CGA mientras el Nano arranca:
    D2 a GND, D3 a +5 V, D4 a +5 V

  IMPORTANTE: verificar la tabla de verdad midiendo el módulo original
  en cada modo antes de usar esta placa, y ajustar las ventanas.
*/

enum Modo : uint8_t { CGA = 0, EGA = 1, VGA = 2 };

const uint8_t PIN_Q17 = 2, PIN_Q18 = 3, PIN_Q21 = 4;

// Tabla de verdad (pin 1, pin 2, pin 3)
const uint8_t TABLA[3][3] = {
  {0, 1, 1},  // CGA  (B+ mas bajo)
  {1, 1, 0},  // EGA
  {1, 0, 1},  // VGA  (B+ mas alto)
};

// Ventanas permitidas en Hz (ajustar segun el chasis)
const uint32_t VENTANA[3][2] = {
  {14800, 16600},  // CGA
  {23500, 25500},  // EGA
  {30000, 32500},  // VGA
};

// Opcion experimental: frecuencias intermedias redondeadas al modo inferior.
// Reemplazar la tabla de arriba por esta solo despues de probar el chasis.
// const uint32_t VENTANA[3][2] = {
//   {14800, 23400},  // CGA
//   {23500, 29900},  // EGA
//   {30000, 32500},  // VGA
// };

const uint32_t HISTERESIS_HZ  = 300;   // ensancha la ventana del modo actual
const uint16_t COMPUERTA_MS   = 20;    // tiempo de conteo por medicion
const uint32_t CONFIRMA_SUBIR = 500;   // ms estables para subir de modo
const uint32_t CONFIRMA_BAJAR = 60;    // ms para caer a CGA (rapido)
const uint32_t PAUSA_MIN_RELE = 2000;  // ms minimos antes de volver a subir

Modo actual = CGA;
Modo candidato = CGA;
uint32_t candidatoDesde = 0;
uint32_t ultimoCambio = 0;

void aplicar(Modo m) {
  digitalWrite(PIN_Q17, TABLA[m][0]);
  digitalWrite(PIN_Q18, TABLA[m][1]);
  digitalWrite(PIN_Q21, TABLA[m][2]);
  actual = m;
  ultimoCambio = millis();
}

// Cuenta pulsos horizontales en D5 durante COMPUERTA_MS
uint32_t medirHz() {
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  TCCR1B = _BV(CS12) | _BV(CS11) | _BV(CS10);  // reloj externo T1, flanco de subida
  delay(COMPUERTA_MS);
  TCCR1B = 0;
  return (uint32_t)TCNT1 * 1000UL / COMPUERTA_MS;
}

Modo clasificar(uint32_t hz) {
  for (uint8_t m = CGA; m <= VGA; m++) {
    uint32_t lo = VENTANA[m][0], hi = VENTANA[m][1];
    if (m == actual) { lo -= HISTERESIS_HZ; hi += HISTERESIS_HZ; }
    if (hz >= lo && hz <= hi) return (Modo)m;
  }
  return CGA;  // sin senal o frecuencia prohibida: estado seguro
}

void setup() {
  aplicar(CGA);              // fija niveles antes de habilitar salidas
  pinMode(PIN_Q17, OUTPUT);
  pinMode(PIN_Q18, OUTPUT);
  pinMode(PIN_Q21, OUTPUT);
}

void loop() {
  Modo m = clasificar(medirHz());
  uint32_t ahora = millis();

  if (m != candidato) { candidato = m; candidatoDesde = ahora; }
  if (candidato == actual) return;

  uint32_t estable = ahora - candidatoDesde;

  if (candidato < actual) {
    // Frecuencia menor que la del modo actual: B+ demasiado alto.
    // Bajar rapido a CGA; si la senal es EGA, subira despues con calma.
    if (estable >= CONFIRMA_BAJAR) aplicar(CGA);
  } else if (estable >= CONFIRMA_SUBIR && ahora - ultimoCambio >= PAUSA_MIN_RELE) {
    aplicar(candidato);
  }
}
