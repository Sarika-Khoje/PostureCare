// PostureCare - Smart Posture Correction Belt
// Board: ESP32 Dev Module | Sensor: MPU6050 (I2C) | Alerts: vibration motor, then buzzer
// Hold the calibrate button for 2 s while in correct posture to store the baseline.

#include <Wire.h>
#include <Preferences.h>

const int PIN_SDA    = 21;
const int PIN_SCL    = 22;
const int PIN_MOTOR  = 25;   // via NPN transistor + flyback diode
const int PIN_BUZZER = 26;   // via NPN transistor (active buzzer)
const int PIN_BUTTON = 27;   // to GND, INPUT_PULLUP
const int PIN_LED    = 2;    // status LED

const uint8_t MPU_ADDR = 0x68;
float PITCH_LIMIT_DEG = 15.0;   // forward/backward slouch tolerance
float ROLL_LIMIT_DEG  = 12.0;   // sideways lean tolerance
const unsigned long VIBRATE_AFTER_MS = 3000;  // bad posture this long -> vibrate
const unsigned long BUZZER_AFTER_MS  = 8000;  // still bad this long -> buzzer
const unsigned long CALIB_HOLD_MS    = 2000;
const float ALPHA = 0.96;       // complementary filter weight (gyro)

Preferences prefs;
float basePitch = 0, baseRoll = 0;
float pitch = 0, roll = 0;
unsigned long lastMicros = 0;
unsigned long badSince = 0;
unsigned long btnDownAt = 0;
bool wasBad = false;

void mpuWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

bool mpuRead(int16_t &ax, int16_t &ay, int16_t &az, int16_t &gx, int16_t &gy, int16_t &gz) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)MPU_ADDR, 14) != 14) return false;
  ax = (Wire.read() << 8) | Wire.read();
  ay = (Wire.read() << 8) | Wire.read();
  az = (Wire.read() << 8) | Wire.read();
  Wire.read(); Wire.read();  // temperature, unused
  gx = (Wire.read() << 8) | Wire.read();
  gy = (Wire.read() << 8) | Wire.read();
  gz = (Wire.read() << 8) | Wire.read();
  return true;
}

void updateAngles() {
  int16_t ax, ay, az, gx, gy, gz;
  if (!mpuRead(ax, ay, az, gx, gy, gz)) return;

  unsigned long now = micros();
  float dt = (now - lastMicros) / 1000000.0;
  lastMicros = now;
  if (dt <= 0 || dt > 0.5) dt = 0.02;

  float axg = ax / 16384.0, ayg = ay / 16384.0, azg = az / 16384.0;   // +/-2 g
  float gxd = gx / 131.0,   gyd = gy / 131.0;                          // +/-250 deg/s

  float accPitch = atan2(-axg, sqrt(ayg * ayg + azg * azg)) * 180.0 / PI;
  float accRoll  = atan2(ayg, azg) * 180.0 / PI;

  pitch = ALPHA * (pitch + gyd * dt) + (1.0 - ALPHA) * accPitch;
  roll  = ALPHA * (roll  + gxd * dt) + (1.0 - ALPHA) * accRoll;
}

void saveBaseline() {
  basePitch = pitch;
  baseRoll = roll;
  prefs.putFloat("pitch", basePitch);
  prefs.putFloat("roll", baseRoll);
  for (int i = 0; i < 3; i++) {   // confirmation blinks and short beeps
    digitalWrite(PIN_LED, HIGH); digitalWrite(PIN_BUZZER, HIGH); delay(120);
    digitalWrite(PIN_LED, LOW);  digitalWrite(PIN_BUZZER, LOW);  delay(120);
  }
  Serial.printf("Baseline saved: pitch=%.1f roll=%.1f\n", basePitch, baseRoll);
}

void handleButton() {
  bool down = (digitalRead(PIN_BUTTON) == LOW);
  if (down && btnDownAt == 0) btnDownAt = millis();
  if (!down) btnDownAt = 0;
  if (down && btnDownAt && millis() - btnDownAt >= CALIB_HOLD_MS) {
    saveBaseline();
    btnDownAt = 0;
    badSince = 0;
  }
}

void setAlerts(bool motor, bool buzzer) {
  digitalWrite(PIN_MOTOR, motor ? HIGH : LOW);
  digitalWrite(PIN_BUZZER, buzzer ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_MOTOR, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  setAlerts(false, false);

  Wire.begin(PIN_SDA, PIN_SCL);
  mpuWrite(0x6B, 0x00);   // wake MPU6050
  mpuWrite(0x1A, 0x03);   // digital low-pass filter
  delay(100);

  prefs.begin("posture", false);
  basePitch = prefs.getFloat("pitch", 0);
  baseRoll  = prefs.getFloat("roll", 0);
  lastMicros = micros();
  Serial.println("PostureCare ready. Hold button 2 s in good posture to calibrate.");
}

void loop() {
  updateAngles();
  handleButton();

  float dPitch = pitch - basePitch;
  float dRoll  = roll - baseRoll;
  bool bad = (fabs(dPitch) > PITCH_LIMIT_DEG) || (fabs(dRoll) > ROLL_LIMIT_DEG);

  if (bad) {
    if (!wasBad) badSince = millis();
    unsigned long badFor = millis() - badSince;
    if (badFor >= BUZZER_AFTER_MS)        setAlerts(true, true);   // stage 2: buzzer reinforces
    else if (badFor >= VIBRATE_AFTER_MS)  setAlerts(true, false);  // stage 1: vibration
    else                                  setAlerts(false, false);
  } else {
    setAlerts(false, false);
    badSince = 0;
  }
  wasBad = bad;

  digitalWrite(PIN_LED, bad ? LOW : HIGH);   // LED on = good posture

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    lastPrint = millis();
    Serial.printf("pitch=%.1f roll=%.1f dP=%.1f dR=%.1f bad=%d\n", pitch, roll, dPitch, dRoll, bad);
  }
  delay(10);
}
