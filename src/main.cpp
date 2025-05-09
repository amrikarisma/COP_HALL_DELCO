#include <Arduino.h>
const int HALL_PINS[] = {A0, A1, A2, A3}; // PC0, PC1, PC2, PC3 (Analog pins A0-A3 on Arduino Nano)
const int COIL_PINS[] = {3, 4, 5, 6};     // PD3, PD4, PD5, PD6
const int TACHO_PIN = 7;                  // PD7
const int IGT_PIN = A4;                   // PC4 (Analog pin A4)

// Timing constants
const unsigned long DWELL_TIME = 2500; // 2.5ms in microseconds
const unsigned long SAFE_FAIL_TIME = 5000; // 5ms in microseconds

// Variables to track coil activation times
unsigned long coilStartTimes[4] = {0, 0, 0, 0};

// Simulation variables
bool simulationMode = false; // Aktifkan mode simulasi
int simulatedIGTSignal = LOW; // Simulasi sinyal IGT (LOW/HIGH)
int simulatedHallSignals[4] = {HIGH, HIGH, HIGH, HIGH}; // Simulasi sinyal Hall (HIGH = standby, LOW = aktif)


void simulateSignals();
void serialEvent();

void setup() {
  for (int i = 0; i < 4; i++) {
    pinMode(HALL_PINS[i], INPUT);
    pinMode(COIL_PINS[i], OUTPUT);
    digitalWrite(COIL_PINS[i], LOW);
  }

  pinMode(TACHO_PIN, OUTPUT);
  digitalWrite(TACHO_PIN, LOW);

  pinMode(IGT_PIN, INPUT);

  // Serial monitor untuk mengontrol simulasi
  Serial.begin(115200); // Kecepatan baud rate 115200
  Serial.println("Simulator started. Use Serial Monitor to control signals.");
}

void loop() {
  if (simulationMode) {
    // Simulasi sinyal IGT dan Hall
    simulateSignals();
  } else {
    // Operasi normal
    int igtSignal = digitalRead(IGT_PIN);

    for (int i = 0; i < 4; i++) {
      if (digitalRead(HALL_PINS[i]) == LOW) { // Sensor Hall aktif (0V)
        if (coilStartTimes[i] == 0) {
          // Start dwell time
          coilStartTimes[i] = micros();
          digitalWrite(COIL_PINS[i], HIGH); // Mulai mengisi coil (dwell)
        }

        // Safe fail: Ensure coil is not active for more than SAFE_FAIL_TIME
        if (micros() - coilStartTimes[i] > SAFE_FAIL_TIME) {
          digitalWrite(COIL_PINS[i], LOW); // Matikan coil
          coilStartTimes[i] = 0;          // Reset timing
        }
      } else {
        // Check if dwell time has passed
        if (coilStartTimes[i] != 0 && micros() - coilStartTimes[i] >= DWELL_TIME) {
          digitalWrite(COIL_PINS[i], LOW); // Lepaskan energi coil (trigger pengapian)
          coilStartTimes[i] = 0;          // Reset timing
        }
      }
    }

    // Generate tacho signal (mirroring IGT signal)
    digitalWrite(TACHO_PIN, igtSignal);
  }
}

// Fungsi untuk mensimulasikan sinyal IGT dan Hall
void simulateSignals() {
  // Simulasikan sinyal IGT
  digitalWrite(IGT_PIN, simulatedIGTSignal);

  // Simulasikan sinyal Hall
  for (int i = 0; i < 4; i++) {
    if (simulatedHallSignals[i] == LOW) {
      if (coilStartTimes[i] == 0) {
        // Start dwell time
        coilStartTimes[i] = micros();
        digitalWrite(COIL_PINS[i], HIGH); // Mulai mengisi coil (dwell)
      }

      // Safe fail: Ensure coil is not active for more than SAFE_FAIL_TIME
      if (micros() - coilStartTimes[i] > SAFE_FAIL_TIME) {
        digitalWrite(COIL_PINS[i], LOW); // Matikan coil
        coilStartTimes[i] = 0;          // Reset timing
      }
    } else {
      // Check if dwell time has passed
      if (coilStartTimes[i] != 0 && micros() - coilStartTimes[i] >= DWELL_TIME) {
        digitalWrite(COIL_PINS[i], LOW); // Lepaskan energi coil (trigger pengapian)
        coilStartTimes[i] = 0;          // Reset timing
      }
    }
  }

  // Generate tacho signal (mirroring simulated IGT signal)
  digitalWrite(TACHO_PIN, simulatedIGTSignal);

  // Debugging output
  Serial.print("IGT Signal: ");
  Serial.print(simulatedIGTSignal == HIGH ? "HIGH" : "LOW");
  Serial.print(" | Hall Signals: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(simulatedHallSignals[i] == HIGH ? "HIGH " : "LOW ");
  }
  Serial.println();
  delay(500); // Delay untuk debugging
}

void serialEvent() {
  while (Serial.available()) {
    char command = Serial.read();

    
    if (command == 'h') {
      simulatedIGTSignal = HIGH;
    } else if (command == 'l') {
      simulatedIGTSignal = LOW;
    }

    if (command >= '1' && command <= '4') { // '1' to '4' for Hall 1 to 4
      int hallIndex = command - '1';
      simulatedHallSignals[hallIndex] = LOW;
    } else if (command >= 'q' && command <= 't') { // 'q' to 't' for Hall 1 to 4
      int hallIndex = command - 'q';
      simulatedHallSignals[hallIndex] = HIGH; 
    }
  }
}