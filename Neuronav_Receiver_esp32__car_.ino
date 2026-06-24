//ESP32 RECEIVER / Car

#include <WiFi.h>
#include <esp_now.h>

typedef struct {
  int throttle;
  int steering;
} ControlData;

ControlData data;

// Motor pins
#define IN1 26
#define IN2 27
#define IN3 14
#define IN4 12

#define PWM_FREQ 1000
#define PWM_RES 8

#define CH1 0
#define CH2 1
#define CH3 2
#define CH4 3

unsigned long lastPacketTime = 0;
#define TIMEOUT 500  // ms

void onReceive(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&data, incomingData, sizeof(data));

  lastPacketTime = millis();

  Serial.print("Throttle: ");
  Serial.print(data.throttle);
  Serial.print(" Steering: ");
  Serial.println(data.steering);

  controlCar(data.throttle, data.steering);
}

void controlCar(int t, int s) {

  // 🔥 Deadzone (avoid jitter)
  if (abs(t) < 10) t = 0;
  if (abs(s) < 10) s = 0;

  // Map to PWM (0–255)
  int speed = constrain(abs(t), 0, 255);
  int turn = constrain(abs(s), 0, 255);

  // Forward / Backward
  if (t > 0) {
    ledcWrite(CH1, speed);
    ledcWrite(CH2, 0);
  } else if (t < 0) {
    ledcWrite(CH1, 0);
    ledcWrite(CH2, speed);
  } else {
    ledcWrite(CH1, 0);
    ledcWrite(CH2, 0);
  }

  // Steering
  if (s > 0) {
    ledcWrite(CH3, turn);
    ledcWrite(CH4, 0);
  } else if (s < 0) {
    ledcWrite(CH3, 0);
    ledcWrite(CH4, turn);
  } else {
    ledcWrite(CH3, 0);
    ledcWrite(CH4, 0);
  }
}

void stopCar() {
  ledcWrite(CH1, 0);
  ledcWrite(CH2, 0);
  ledcWrite(CH3, 0);
  ledcWrite(CH4, 0);
}

void setup() {
  Serial.begin(115200);

  // PWM setup
  ledcSetup(CH1, PWM_FREQ, PWM_RES);
  ledcSetup(CH2, PWM_FREQ, PWM_RES);
  ledcSetup(CH3, PWM_FREQ, PWM_RES);
  ledcSetup(CH4, PWM_FREQ, PWM_RES);

  ledcAttachPin(IN1, CH1);
  ledcAttachPin(IN2, CH2);
  ledcAttachPin(IN3, CH3);
  ledcAttachPin(IN4, CH4);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(onReceive);
}

void loop() {
  // 🛑 FAILSAFE
  if (millis() - lastPacketTime > TIMEOUT) {
    stopCar();
  }
}