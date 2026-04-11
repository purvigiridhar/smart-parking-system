#include <Servo.h>

Servo barrier;

// Pins
int entrySensor = 6;
int exitSensor = 7;
int servoPin = 9;
int buzzer = 13;   // 🔊 buzzer pin

// Parking
int totalSlots = 4;
int carCount = 0;

// Sensor states (edge detection)
bool entryState = HIGH;
bool lastEntryState = HIGH;

bool exitState = HIGH;
bool lastExitState = HIGH;

void setup() {
  pinMode(entrySensor, INPUT);
  pinMode(exitSensor, INPUT);
  pinMode(buzzer, OUTPUT);

  digitalWrite(buzzer, HIGH); // 🔥 ensure buzzer OFF at start

  barrier.attach(servoPin);
  barrier.write(0); // barrier closed

  Serial.begin(9600);
}

void loop() {

  // Read sensors
  entryState = digitalRead(entrySensor);
  exitState = digitalRead(exitSensor);

  // ================= ENTRY =================
  if (entryState == LOW && lastEntryState == HIGH) {

    if (carCount < totalSlots) {
      carCount++;
      Serial.println("Car Entered");

      digitalWrite(buzzer, HIGH); // ensure OFF

      barrier.write(90);   // open
      delay(3000);         
      barrier.write(0);    

    } 
    else {
      Serial.println("Parking FULL");

      digitalWrite(buzzer, LOW); // 🔊 buzzer ON

      delay(2000);                // beep duration
      digitalWrite(buzzer, HIGH);  // turn OFF

      // barrier stays closed
    }
  }

  // ================= EXIT =================
  if (exitState == LOW && lastExitState == HIGH) {

    if (carCount > 0) {
      carCount--;
      Serial.println("Car Exited");

      digitalWrite(buzzer, HIGH); // ensure OFF

      barrier.write(90);   
      delay(3000);
      barrier.write(0);
    }
  }

  // ================= STATUS =================
  Serial.print("Cars Inside: ");
  Serial.println(carCount);

  // Save previous states
  lastEntryState = entryState;
  lastExitState = exitState;

  delay(200);
}
