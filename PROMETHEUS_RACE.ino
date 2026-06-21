/*
 * Creador: Ernesto Torregrosa Palacio Exalumno CEU
 * Licencia: Creative Commons BY-NC-SA 4.0
 * (Prohibido su uso comercial. Se permiten modificaciones citando al autor original)
 * 
 * PROMETHEUS_RACE.ino
 * 
 * Descripcion:
 * Modo Carrera (Race Mode). Elimina los suavizados y rampas de aceleracion
 * para ofrecer una respuesta de motores 1:1 ultrarrapida e instantanea a 
 * los gatillos del mando Xbox, priorizando la velocidad y agilidad del Coche.
 */

#include <ESP32Servo.h>
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>

const int ledConexion = 2;
const int buzzerPin = 15;
const int servoSteeringPin = 16; 

const int motorA_Fwd = 26;
const int motorA_Bwd = 25;
const int motorB_Bwd = 19;
const int motorB_Fwd = 18;

XboxSeriesXControllerESP32_asukiaaa::Core xboxController;
Servo ServoSteering;

const int STEERING_CENTER = 95;
const int STEERING_LEFT   = 46;
const int STEERING_RIGHT  = 146;

const int MOTOR_MIN_PWM = 75; 
bool sportMode = false; 
bool tankMode = false;
bool btnLB_prev = false;
bool btnRB_prev = false;

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
  pinMode(ledConexion, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(motorA_Fwd, OUTPUT);
  pinMode(motorA_Bwd, OUTPUT);
  pinMode(motorB_Bwd, OUTPUT);
  pinMode(motorB_Fwd, OUTPUT);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  ServoSteering.setPeriodHertz(50);
  ServoSteering.attach(servoSteeringPin, 500, 2400);
  ServoSteering.write(STEERING_CENTER);

  xboxController.begin();
  Serial.println("[OK] PROMETHEUS RACE MK-IV: Ready.");
}

void loop() {
  xboxController.onLoop();

  bool isConnected = xboxController.isConnected();
  digitalWrite(ledConexion, isConnected ? HIGH : LOW);

  if (!isConnected) {
    stopMotors();
    return;
  }

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

  if (trigRT > 10 || trigLT > 10) {
    bool moveForward = (trigRT > 10);
    int rawTrigger = moveForward ? trigRT : trigLT;
    int topSpeed = sportMode ? 255 : 140; 
    
    int rawMapped = constrain(rawTrigger, 10, 400);
    int basePWM = map(rawMapped, 10, 400, MOTOR_MIN_PWM, topSpeed);

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
  if (btnLB_now && !btnLB_prev) {
    sportMode = !sportMode;
  }
  btnLB_prev = btnLB_now;

  bool btnRB_now = xboxController.xboxNotif.btnRB;
  if (btnRB_now && !btnRB_prev) {
    tankMode = !tankMode;
  }
  btnRB_prev = btnRB_now;

  if (xboxController.xboxNotif.btnStart) {
    stopMotors();
  }

  delay(10); 
}
