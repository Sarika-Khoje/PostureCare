# Firmware

Arduino sketch for ESP32 + MPU6050.

1. Install Arduino IDE and the ESP32 board package (Espressif Systems).
2. Open `posturecare/posturecare.ino`, select "ESP32 Dev Module", upload.
3. Open Serial Monitor at 115200 baud to see live pitch/roll.
4. Sit or stand in correct posture, hold the button 2 s: the baseline is saved to flash.
5. Slouch past the limit: vibration after 3 s, buzzer after 8 s if still uncorrected.

Tune `PITCH\_LIMIT\_DEG`, `ROLL\_LIMIT\_DEG`, `VIBRATE\_AFTER\_MS`, `BUZZER\_AFTER\_MS` at the top of the sketch.
Axis note: pitch/roll assume the MPU6050 X axis points forward and Z points out of the back.
If your mounting differs, swap or negate axes in `updateAngles()`.


