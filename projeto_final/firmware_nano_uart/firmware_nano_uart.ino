/*
 * ---------------------------------------------------------
 *  Arduino Nano - Detector de Visitante
 *  Sensor: HC-SR04
 *  Comunicação: UART (9600 baud)
 *  Envia: "VISITA\n"
 * ---------------------------------------------------------
 */

#include <Ultrasonic.h>

// ---------------- CONFIGURAÇÕES ----------------

// Pinos do HC-SR04
#define PIN_TRIGGER  8
#define PIN_ECHO     9

// Distância limite para considerar visitante (cm)
#define DIST_VISITANTE_CM 120

// Número mínimo de leituras consecutivas válidas
#define CONFIRMACAO_LEITURAS 2

// Tempo mínimo entre envios (ms)
#define TEMPO_MIN_ENTRE_EVENTOS 2000

// ------------------------------------------------

Ultrasonic ultrasonic(PIN_TRIGGER, PIN_ECHO);

bool visitante_presente = false;
uint8_t leituras_validas = 0;
unsigned long ultimo_envio = 0;

void setup() {
  Serial.begin(9600);
  delay(500);

  Serial.println("Nano Ultrassom Iniciado");
}

void loop() {

  long distancia = ultrasonic.read(); // retorna em cm

  if (distancia > 0 && distancia <= DIST_VISITANTE_CM) {

    if (!visitante_presente) {
      leituras_validas++;

      if (leituras_validas >= CONFIRMACAO_LEITURAS) {

        unsigned long agora = millis();

        if (agora - ultimo_envio > TEMPO_MIN_ENTRE_EVENTOS) {

          Serial.println("VISITA");

          ultimo_envio = agora;
          visitante_presente = true;
        }

        leituras_validas = 0;
      }
    }

  } else {
    visitante_presente = false;
    leituras_validas = 0;
  }

  delay(100); // pequena pausa entre leituras
}
