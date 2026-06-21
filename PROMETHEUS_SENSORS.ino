/*
 * Creador: Ernesto Torregrosa Palacio Exalumno CEU
 * Licencia: Creative Commons BY-NC-SA 4.0
 * (Prohibido su uso comercial. Se permiten modificaciones citando al autor original)
 * 
 * PROMETHEUS_SENSORS.ino
 * 
 * Descripcion:
 * Integracion de sensores magnetico y de color (TCS34725) junto con telemetria 
 * Bluetooth Serial. Mantiene el control por mando de Xbox para traccion 
 * y brazo robotico. Disenado para misiones de suministro donde la lectura 
 * del entorno es critica.
 */

#include <ESP32Servo.h>
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>
#include "Adafruit_TCS34725.h"
#include "BluetoothSerial.h"
#include <Wire.h>

const int ledConexion = 2;
const int buzzerPin = 15;
const int servoSteeringPin = 16;
const int servoSensorArmPin = 27;

const int sdaPin = 21;      
const int sclPin = 22;      
const int magneticPin = 14; 

const int motorA_Fwd = 26; 
const int motorA_Bwd = 25; 
const int motorB_Bwd = 19; 
const int motorB_Fwd = 18; 

XboxSeriesXControllerESP32_asukiaaa::Core xboxController;
BluetoothSerial SerialBT; 
Servo ServoSteering;
Servo ServoSensorArm;
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

const int STEERING_CENTER = 94; 
const int STEERING_LEFT = 49;   
const int STEERING_RIGHT = 139; 

const int SENSOR_UP = 20;
const int SENSOR_DOWN = 160;
int posArm = SENSOR_UP;

const int MOTOR_MIN_PWM = 75; 
bool sportMode = false;
bool tankMode = false;
bool btnLB_prev = false;
bool btnDirUp_prev = false;

unsigned long lastSensorRead = 0;
unsigned long lastTelemetry = 0;

void setMotorPWM(int fwdA, int bwdA, int fwdB, int bwdB) {
  static int lastFwdA = -1, lastBwdA = -1, lastFwdB = -1, lastBwdB = -1;
  if (fwdA != lastFwdA) { analogWrite(motorA_Fwd, fwdA); lastFwdA = fwdA; }
  if (bwdA != lastBwdA) { analogWrite(motorA_Bwd, bwdA); lastBwdA = bwdA; }
  if (fwdB != lastFwdB) { analogWrite(motorB_Fwd, fwdB); lastFwdB = fwdB; }
  if (bwdB != lastBwdB) { analogWrite(motorB_Bwd, bwdB); lastBwdB = bwdB; }
}

void stopMotors() {
  setMotorPWM(0, 0, 0, 0);
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("PROMETHEUS_TELEMETRY"); 
  
  pinMode(ledConexion, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(motorA_Fwd, OUTPUT);
  pinMode(motorA_Bwd, OUTPUT);
  pinMode(motorB_Bwd, OUTPUT);
  pinMode(motorB_Fwd, OUTPUT);

  Wire.begin(sdaPin, sclPin);
  pinMode(magneticPin, INPUT);
  tcs.begin();

  ServoSteering.setPeriodHertz(50);
  ServoSteering.attach(servoSteeringPin, 500, 2400);
  ServoSteering.write(STEERING_CENTER);

  ServoSensorArm.setPeriodHertz(50);
  ServoSensorArm.attach(servoSensorArmPin, 500, 2400);
  ServoSensorArm.write(posArm);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  xboxController.begin();
}

void loop() {
  xboxController.onLoop();

  if (!xboxController.isConnected()) {
    stopMotors();
    digitalWrite(ledConexion, LOW);
    return;
  }
  digitalWrite(ledConexion, HIGH);

  static int lastWrittenUS = -1;
  int targetUS = 1500;
  float normalizedSteer = 0.0;
  
  int joyLX = (int)xboxController.xboxNotif.joyLHori - 32767;
  const int JOY_DEADZONE = 8000;
  
  if (abs(joyLX) < JOY_DEADZONE) {
    targetUS = 1500;
    normalizedSteer = 0.0;
  } else {
    normalizedSteer = (float)joyLX / 32767.0;
    float expoFactor = normalizedSteer * normalizedSteer * (normalizedSteer > 0 ? 1.0 : -1.0);
    targetUS = 1500 + (int)(expoFactor * 500); 
    targetUS = constrain(targetUS, 1000, 2000);
  }

  if (abs(targetUS - lastWrittenUS) >= 3) {
    ServoSteering.writeMicroseconds(targetUS);
    lastWrittenUS = targetUS;
  }

  int trigRT = xboxController.xboxNotif.trigRT;
  int trigLT = xboxController.xboxNotif.trigLT;
  int currentBasePWM = 0; 

  if (trigRT > 10 || trigLT > 10) {
    bool moveForward = (trigRT > 10);
    int rawTrigger = moveForward ? trigRT : trigLT;
    int topSpeed = sportMode ? 255 : 140; 
    
    int rawMapped = constrain(rawTrigger, 10, 400);
    int basePWM = map(rawMapped, 10, 400, MOTOR_MIN_PWM, topSpeed);
    currentBasePWM = basePWM;

    int pwmA = basePWM;
    int pwmB = basePWM;

    float diffAssist = 1.0;
    if (abs(normalizedSteer) > 0.4) diffAssist = 0.7; 
    if (abs(normalizedSteer) > 0.8) diffAssist = 0.3; 
    
    if (normalizedSteer > 0.1) pwmA = max(MOTOR_MIN_PWM, (int)(pwmA * diffAssist));
    if (normalizedSteer < -0.1) pwmB = max(MOTOR_MIN_PWM, (int)(pwmB * diffAssist));

    if (moveForward) {
      int fwdA = pwmA, bwdA = 0;
      int fwdB = pwmB, bwdB = 0;
      
      if (tankMode && normalizedSteer > 0.85) { fwdA = 0; bwdA = max(MOTOR_MIN_PWM, (int)(basePWM * 0.3)); }
      if (tankMode && normalizedSteer < -0.85) { fwdB = 0; bwdB = max(MOTOR_MIN_PWM, (int)(basePWM * 0.3)); }
      
      setMotorPWM(fwdA, bwdA, fwdB, bwdB);
    } else {
      setMotorPWM(0, pwmA, 0, pwmB);
    }
  } else {
    stopMotors();
  }

  bool btnLB_now = xboxController.xboxNotif.btnLB;
  if (btnLB_now && !btnLB_prev) { sportMode = !sportMode; }
  btnLB_prev = btnLB_now;

  bool btnDirUp_now = xboxController.xboxNotif.btnDirUp;
  if (btnDirUp_now && !btnDirUp_prev) { tankMode = !tankMode; }
  btnDirUp_prev = btnDirUp_now;

  if (xboxController.xboxNotif.btnA) {
    posArm = SENSOR_DOWN;
    ServoSensorArm.write(posArm);
  } else if (xboxController.xboxNotif.btnB) {
    posArm = SENSOR_UP;
    ServoSensorArm.write(posArm);
  }

  if (xboxController.xboxNotif.btnStart) {
    stopMotors();
    ServoSensorArm.write(SENSOR_UP);
    tone(buzzerPin, 400, 500);
  }

  if (millis() - lastSensorRead > 200) {
    uint16_t r, g, b, c_val;
    tcs.getRawData(&r, &g, &b, &c_val);
    int magneto = digitalRead(magneticPin);

    String telemetryStr = "S:" + String(r) + "," + String(g) + "," + String(b) + "," + String(magneto == LOW ? 1 : 0);
    
    Serial.println(telemetryStr);
    if(SerialBT.hasClient()) SerialBT.println(telemetryStr);

    if (magneto == LOW) tone(buzzerPin, 2500, 50); 
    lastSensorRead = millis();
  }

  if (millis() - lastTelemetry > 300) {
     float v = ((float)currentBasePWM / 255.0) * 8.4; 
     String velStr = "V:" + String(v, 1);
     Serial.println(velStr);
     if(SerialBT.hasClient()) SerialBT.println(velStr);
     lastTelemetry = millis();
  }
}
