/*
  Mini 3D Scanner - Mark 2
  ------------------------
  Firmware skeleton for a turntable scanner:

    * a distance sensor measures the radius of the object,
    * a stepper rotates the turntable (theta / columns),
    * a second stepper raises the sensor after each layer (z / rows).

  Serial output format (115200 baud), one value per line:

    7.30          distance in cm, printed with 2 decimals
    ...
    9999.00       end-of-layer delimiter

  This matches the layout expected by src/calcGetData.py, where each
  layer holds READINGS_PER_REV values and 9999.00 separates layers.
  Capture the serial output and save it as data/farmer.txt (or any file
  you point INPUT_FILE to in the Python script).

  TODO: adjust the pin map, motor steps and sensor code to your build.
*/

// ---------------------------------------------------------------- pin map

const uint8_t TRIG_PIN = 9;
const uint8_t ECHO_PIN = 10;

const uint8_t TURNTABLE_STEP_PIN = 3;
const uint8_t TURNTABLE_DIR_PIN = 4;
const uint8_t TURNTABLE_ENABLE_PIN = 5;

const uint8_t LIFT_STEP_PIN = 6;
const uint8_t LIFT_DIR_PIN = 7;
const uint8_t LIFT_ENABLE_PIN = 8;

// ------------------------------------------------------- scan geometry
// These values must stay in sync with src/calcGetData.py.

const uint16_t READINGS_PER_REV = 200;  // columns in data/farmer.txt
const uint8_t LAYERS = 78;              // rows in data/farmer.txt
const float LAYER_HEIGHT_MM = 1.0;      // zDelta = 0.1 cm in the script

// ---------------------------------------------------------------- motion

const uint16_t MOTOR_STEPS_PER_REV = 200;  // 1.8 deg stepper
const uint8_t MICROSTEPS = 16;

const uint16_t STEPS_PER_READING =
    MOTOR_STEPS_PER_REV * MICROSTEPS / READINGS_PER_REV;

// Steps needed to raise the sensor by 1 mm (depends on lead screw pitch,
// e.g. 8 mm lead: 200 * 16 / 8 = 400).
const uint16_t LIFT_STEPS_PER_MM = 400;

const uint16_t STEP_DELAY_US = 800;  // motor speed; lower = faster
const uint16_t SETTLE_MS = 150;      // wait for vibration to fade

// ---------------------------------------------------------------- sensor

const uint16_t ECHO_TIMEOUT_US = 30000;  // ~5 m round trip
const float ERROR_VALUE = -1.0;          // ignored/normalised by the script

// ------------------------------------------------------------------ code

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    // Wait for the host to open the port (native USB boards only).
  }

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(TURNTABLE_STEP_PIN, OUTPUT);
  pinMode(TURNTABLE_DIR_PIN, OUTPUT);
  pinMode(TURNTABLE_ENABLE_PIN, OUTPUT);

  pinMode(LIFT_STEP_PIN, OUTPUT);
  pinMode(LIFT_DIR_PIN, OUTPUT);
  pinMode(LIFT_ENABLE_PIN, OUTPUT);

  digitalWrite(TURNTABLE_ENABLE_PIN, LOW);  // LOW = driver enabled
  digitalWrite(LIFT_ENABLE_PIN, LOW);
  digitalWrite(TRIG_PIN, LOW);

  delay(1000);  // let the mechanics and sensor settle
}

void loop() {
  for (uint8_t layer = 0; layer < LAYERS; layer++) {
    for (uint16_t reading = 0; reading < READINGS_PER_REV; reading++) {
      float distance = readDistanceCm();

      Serial.print(distance, 2);
      Serial.println();

      stepMotor(TURNTABLE_STEP_PIN, TURNTABLE_DIR_PIN, STEPS_PER_READING);
      delay(SETTLE_MS);
    }

    Serial.println("9999.00");
    moveLift(LAYER_HEIGHT_MM);
  }

  Serial.println("Scan complete");

  // One scan per reset so the output stays a single clean dataset.
  while (true) {
    delay(1000);
  }
}

/*
  Trigger one measurement and return the distance in centimetres.
  Returns ERROR_VALUE when the echo times out.
*/
float readDistanceCm() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);

  if (duration == 0) {
    return ERROR_VALUE;
  }

  return duration * 0.0343f / 2.0f;
}

/*
  Step a driver a fixed number of times, keeping the direction pin as-is.
*/
void stepMotor(uint8_t stepPin, uint8_t dirPin, uint16_t steps) {
  digitalWrite(dirPin, HIGH);

  for (uint16_t i = 0; i < steps; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(STEP_DELAY_US);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(STEP_DELAY_US);
  }
}

/*
  Raise the sensor by the given number of millimetres.
*/
void moveLift(float millimetres) {
  uint32_t steps = (uint32_t)(millimetres * LIFT_STEPS_PER_MM + 0.5f);
  stepMotor(LIFT_STEP_PIN, LIFT_DIR_PIN, (uint16_t)steps);
  delay(SETTLE_MS);
}
