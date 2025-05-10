#include <Arduino.h>
const int HALL_PINS[] = {A0, A1, A2, A3}; // PC0, PC1, PC2, PC3 (Analog pins A0-A3 on Arduino Nano)
const int COIL_PINS[] = {3, 4, 5, 6};	  // PD3, PD4, PD5, PD6
const int TACHO_PIN = 7;				  // PD7
const int IGT_PIN = A4;					  // PC4 (Analog pin A4)

// Timing constants
const unsigned long DWELL_TIME = 2500;	   // 2.5ms in microseconds
const unsigned long SAFE_FAIL_TIME = 5000; // 5ms in microseconds

// Variables to track coil activation times
unsigned long coilStartTimes[4] = {0, 0, 0, 0};

// Simulation variables
bool simulationMode = false;							// Aktifkan mode simulasi
int simulatedIGTSignal = LOW;							// Simulasi sinyal IGT (LOW/HIGH)
int simulatedHallSignals[4] = {HIGH, HIGH, HIGH, HIGH}; // Simulasi sinyal Hall (HIGH = standby, LOW = aktif)
unsigned long lastDebugTime = 0;						// Waktu terakhir debugging dilakukan
const unsigned long DEBUG_INTERVAL = 500;				// Interval debugging dalam milidetik (500ms)

void simulateSignals();
void serialEvent();

void setup()
{
	for (int i = 0; i < 4; i++)
	{
		pinMode(HALL_PINS[i], INPUT_PULLUP); // Set HALL pins as input with pull-up resistor
		pinMode(COIL_PINS[i], OUTPUT);
		digitalWrite(COIL_PINS[i], LOW);
	}

	pinMode(TACHO_PIN, OUTPUT);
	digitalWrite(TACHO_PIN, LOW);

	pinMode(IGT_PIN, INPUT);

	Serial.begin(115200); // Kecepatan baud rate 115200
	if (simulationMode)
	{
		Serial.println("Simulation mode activated. Use 'h' for HIGH, 'l' for LOW, '1'-'4' for Hall signals.");
	}
	else
	{
		Serial.println("Normal mode activated.");
	}
}

void loop()
{
	if (simulationMode)
	{
		// Simulasi sinyal IGT dan Hall
		simulateSignals();
	}
	else
	{
		// Operasi normal
		int igtSignal = digitalRead(IGT_PIN);

		for (int i = 0; i < 4; i++)
		{
			if (digitalRead(HALL_PINS[i]) == LOW)
			{ // Sensor Hall aktif (0V)
				if (igtSignal == HIGH)
				{
					if (coilStartTimes[i] == 0)
					{
						// Start dwell time
						coilStartTimes[i] = micros();
						digitalWrite(COIL_PINS[i], HIGH); // Mulai mengisi coil (dwell)
					}

					// Safe fail: Ensure coil is not active for more than SAFE_FAIL_TIME
					if (micros() - coilStartTimes[i] > SAFE_FAIL_TIME)
					{
						digitalWrite(COIL_PINS[i], LOW); // Matikan coil
						coilStartTimes[i] = 0;			 // Reset timing
					}
				}
				else
				{
					// Lepaskan energi coil (trigger pengapian) jika IGT LOW
					if (coilStartTimes[i] != 0)
					{
						digitalWrite(COIL_PINS[i], LOW);
						coilStartTimes[i] = 0; // Reset waktu coil
					}
				}
			}
			else
			{
				digitalWrite(COIL_PINS[i], LOW);
				coilStartTimes[i] = 0;
			}
		}

		// Generate tacho signal (mirroring IGT signal)
		digitalWrite(TACHO_PIN, igtSignal);

		// Debugging output
		unsigned long currentTime = millis();
		// if (currentTime - lastDebugTime >= DEBUG_INTERVAL)
		// {
		lastDebugTime = currentTime;
		Serial.print("Hall Signals: ");
		for (int i = 0; i < 4; i++)
		{
			Serial.print(digitalRead(HALL_PINS[i]) == HIGH ? "1 " : "0 ");
		}
		Serial.print(" | Coil States: ");
		for (int i = 0; i < 4; i++)
		{
			Serial.print(digitalRead(COIL_PINS[i]) == HIGH ? "1 " : "0 ");
		}
		Serial.print(" | Dwell Times: ");
		for (int i = 0; i < 4; i++)
		{
			if (coilStartTimes[i] != 0)
			{
				// Hitung dwell time jika coil aktif
				Serial.print((micros() - coilStartTimes[i]) / 1000); // Konversi ke milidetik
				Serial.print("ms ");
			}
			else
			{
				Serial.print("0ms ");
			}
			if (i < 3)
				Serial.print(", ");
		}
		Serial.print(" | IGT Signal: ");
		Serial.print(igtSignal == HIGH ? "1" : "0");
		Serial.println();
		// }
	}
}

// Fungsi untuk mensimulasikan sinyal IGT dan Hall
void simulateSignals()
{
	// Simulasikan sinyal IGT
	digitalWrite(IGT_PIN, simulatedIGTSignal);

	// Simulasikan sinyal Hall
	for (int i = 0; i < 4; i++)
	{
		if (simulatedHallSignals[i] == LOW)
		{
			if (coilStartTimes[i] == 0)
			{
				// Start dwell time
				coilStartTimes[i] = micros();
				digitalWrite(COIL_PINS[i], HIGH); // Mulai mengisi coil (dwell)
			}

			// Safe fail: Ensure coil is not active for more than SAFE_FAIL_TIME
			if (micros() - coilStartTimes[i] > SAFE_FAIL_TIME)
			{
				digitalWrite(COIL_PINS[i], LOW); // Matikan coil
				coilStartTimes[i] = 0;			 // Reset timing
			}
		}
		else
		{
			// Check if dwell time has passed
			if (coilStartTimes[i] != 0 && micros() - coilStartTimes[i] >= DWELL_TIME)
			{
				digitalWrite(COIL_PINS[i], LOW); // Lepaskan energi coil (trigger pengapian)
				coilStartTimes[i] = 0;			 // Reset timing
			}
		}
	}

	// Generate tacho signal (mirroring simulated IGT signal)
	digitalWrite(TACHO_PIN, simulatedIGTSignal);

	// Debugging output
	Serial.print("IGT Signal: ");
	Serial.print(simulatedIGTSignal == HIGH ? "1" : "0");
	Serial.print(" | Hall Signals: ");
	for (int i = 0; i < 4; i++)
	{
		Serial.print(simulatedHallSignals[i] == HIGH ? "1 " : "0 ");
	}
	Serial.println();
	delay(500); // Delay untuk debugging
}

void serialEvent()
{
	while (Serial.available())
	{
		char command = Serial.read();

		if (command == 'h')
		{
			simulatedIGTSignal = HIGH;
		}
		else if (command == 'l')
		{
			simulatedIGTSignal = LOW;
		}

		if (command >= '1' && command <= '4')
		{ // '1' to '4' for Hall 1 to 4
			int hallIndex = command - '1';
			simulatedHallSignals[hallIndex] = LOW;
		}
		else if (command >= 'q' && command <= 't')
		{ // 'q' to 't' for Hall 1 to 4
			int hallIndex = command - 'q';
			simulatedHallSignals[hallIndex] = HIGH;
		}
	}
}