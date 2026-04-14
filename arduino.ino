#include <Servo.h>

Servo barrier;

// ── Pins ──────────────────────────────────────────────
const int entrySensor = 6;   // IR sensor OUTSIDE (entry side)
const int exitSensor  = 7;   // IR sensor INSIDE  (exit / near barrier)
const int servoPin    = 9;
const int buzzer      = 13;

// ── Parking ───────────────────────────────────────────
const int totalSlots  = 4;
int carCount          = 0;

// ── Timeout ───────────────────────────────────────────
#define SENSOR_TIMEOUT 15000  // 15 seconds

// ──────────────────────────────────────────────────────

void setup() {
  pinMode(entrySensor, INPUT);
  pinMode(exitSensor,  INPUT);
  pinMode(buzzer,      OUTPUT);

  digitalWrite(buzzer, HIGH);  // buzzer OFF (active LOW)

  barrier.attach(servoPin);
  barrier.write(0);            // start closed

  Serial.begin(9600);
  Serial.println("=== Smart Parking System Ready ===");
  Serial.print("Total Slots: ");
  Serial.println(totalSlots);
}

// ──────────────────────────────────────────────────────

void loop() {

  // ================= ENTRY =================
  if (digitalRead(entrySensor) == LOW) {

    delay(100);  // debounce
    if (digitalRead(entrySensor) != LOW) return;

    Serial.println("[ENTRY] Car detected at entry sensor");

    if (carCount < totalSlots) {

      barrier.write(90);
      Serial.println("[ENTRY] Barrier opened — waiting for car to pass through...");
      delay(500);

      // Step 1: wait for car to reach inner (exit) sensor
      if (!waitForSensor(exitSensor, LOW, SENSOR_TIMEOUT)) {
        Serial.println("[ENTRY] TIMEOUT — car did not pass through, closing barrier");
        Serial.println("TIMEOUT");                  // ← dashboard tag
        buzzerBeeps(3);
        barrier.write(0);
        waitForClear(entrySensor);
        return;
      }

      // Step 2: wait for car to fully clear inner sensor
      if (!waitForSensor(exitSensor, HIGH, SENSOR_TIMEOUT)) {
        Serial.println("[ENTRY] TIMEOUT — car stalled at inner sensor, closing barrier");
        Serial.println("TIMEOUT");                  // ← dashboard tag
        buzzerBeeps(3);
        barrier.write(0);
        return;
      }

      // Car fully inside
      barrier.write(0);
      carCount++;
      Serial.println("[ENTRY] Car fully inside — barrier closed");
      Serial.println("CAR_ENTERED");               // ← dashboard tag
      Serial.print("Cars Inside: ");
      Serial.println(carCount);

      if (carCount >= totalSlots) {
        Serial.println("PARKING_FULL");             // ← dashboard tag
      }

      printStatus();

    } else {
      Serial.println("[ENTRY] Parking FULL — denying entry");
      Serial.println("PARKING_FULL");              // ← dashboard tag
      buzzerBeeps(1);
      waitForClear(entrySensor);
    }
  }

  // ================= EXIT =================
  if (digitalRead(exitSensor) == LOW) {

    delay(100);  // debounce
    if (digitalRead(exitSensor) != LOW) return;

    Serial.println("[EXIT] Car detected at exit sensor");

    if (carCount > 0) {

      barrier.write(90);
      Serial.println("[EXIT] Barrier opened — waiting for car to exit...");
      delay(500);

      // Step 1: wait for car to reach outer (entry) sensor
      if (!waitForSensor(entrySensor, LOW, SENSOR_TIMEOUT)) {
        Serial.println("[EXIT] TIMEOUT — car did not pass through, closing barrier");
        Serial.println("TIMEOUT");                  // ← dashboard tag
        buzzerBeeps(3);
        barrier.write(0);
        waitForClear(exitSensor);
        return;
      }

      // Step 2: wait for car to fully clear outer sensor
      if (!waitForSensor(entrySensor, HIGH, SENSOR_TIMEOUT)) {
        Serial.println("[EXIT] TIMEOUT — car stalled at outer sensor, closing barrier");
        Serial.println("TIMEOUT");                  // ← dashboard tag
        buzzerBeeps(3);
        barrier.write(0);
        return;
      }

      // Car fully outside
      barrier.write(0);
      carCount--;
      Serial.println("[EXIT] Car fully outside — barrier closed");
      Serial.println("CAR_EXITED");                // ← dashboard tag
      Serial.print("Cars Inside: ");
      Serial.println(carCount);
      printStatus();

    } else {
      Serial.println("[EXIT] No cars to exit");
      waitForClear(exitSensor);
    }
  }
}

// ──────────────────────────────────────────────────────
bool waitForSensor(int pin, bool expectedState, unsigned long timeout) {
  unsigned long start = millis();
  while (digitalRead(pin) != expectedState) {
    if (millis() - start >= timeout) return false;
    delay(50);
  }
  return true;
}

void waitForClear(int pin) {
  while (digitalRead(pin) == LOW) {
    delay(50);
  }
  delay(200);
}

void buzzerBeeps(int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(buzzer, LOW);
    delay(250);
    digitalWrite(buzzer, HIGH);
    delay(200);
  }
}

void printStatus() {
  Serial.print("Cars Inside: ");
  Serial.print(carCount);
  Serial.print(" / ");
  Serial.println(totalSlots);

  if (carCount >= totalSlots) {
    Serial.println("Status: PARKING FULL");
  } else {
    Serial.print("Status: ");
    Serial.print(totalSlots - carCount);
    Serial.println(" slot(s) available");
  }

  Serial.println("----------------------------------");
}
