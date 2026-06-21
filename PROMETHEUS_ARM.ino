/*
 * Creador: Ernesto Torregrosa Palacio Exalumno CEU
 * Licencia: Creative Commons BY-NC-SA 4.0
 * (Prohibido su uso comercial. Se permiten modificaciones citando al autor original)
 * 
 * PROMETHEUS_ARM.ino
 * 
 * Descripcion:
 * Controla la version 'ARM Mode' (Modo Brazo Robot) con mando Xbox Series X/S.
 * Permite controlar un brazo robotico, una pinza y una compuerta.
 * Prioriza la estabilidad electrica de los servos para tareas de fuerza
 * aislando los timers PWM. Incluye macros de descarga automatica y vibracion.
 */

#include <ESP32Servo.h>
#include <XboxSeriesXControllerESP32_asukiaaa.hpp>

const int ledConexion = 2;
const int buzzerPin = 15;
const int servoSteeringPin = 16;
const int servoArmPin = 27;     
const int servoClawPin = 33;    
const int servoGatePin = 17;    

const int motorA_Fwd = 26; 
const int motorA_Bwd = 25; 
const int motorB_Bwd = 19; 
const int motorB_Fwd = 18; 

XboxSeriesXControllerESP32_asukiaaa::Core xboxController;
Servo ServoSteering;
Servo ServoArm;
Servo ServoClaw;
Servo ServoGate;

const int STEERING_CENTER = 96; 
const int STEERING_LEFT = 46;   
const int STEERING_RIGHT = 146; 

const int ARM_UP = 15;      
const int ARM_DOWN = 160;  

const int CLAW_OPEN = 90;
const int CLAW_CLOSE = 10;
const int GATE_OPEN = 118; 
const int GATE_CLOSE = 55;

bool clawOpen = true;
bool gateOpen = false;
bool btnY_prev = false;
bool btnX_prev = false;
bool btnA_prev = false; 
bool btnB_prev = false; 
bool btnRB_prev = false; 
unsigned long gateAutoTimer = 0;

float currentArmPos = 90.0;
int targetArmPos = 90;
const float ARM_STEP_UP = 5.0;        
const float ARM_STEP_DOWN_FAST = 6.0;   
const float ARM_STEP_DOWN_SLOW = 2.0;   
const int ARM_SLOW_THRESHOLD = 130;    

const int MOTOR_MIN_PWM = 75; 
bool sportMode = false;
bool tankMode = false;
bool btnLB_prev = false;
bool btnDirUp_prev = false; 

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

unsigned long rumbleTimer = 0;
bool isRumbling = false;

void triggerRumble(uint8_t left, uint8_t right, int durationMs) {
  if (!xboxController.isConnected()) return;
  XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase repo;
  
  uint8_t leftPct = map(left, 0, 255, 0, 100);
  uint8_t rightPct = map(right, 0, 255, 0, 100);
  
  repo.setAllOff();
  if (leftPct > 0) { repo.v.select.left = true; repo.v.power.left = leftPct; }
  if (rightPct > 0) { repo.v.select.right = true; repo.v.power.right = rightPct; }
  repo.v.timeActive = min(255, durationMs / 10);
  
  xboxController.writeHIDReport(repo);
  rumbleTimer = millis() + durationMs;
  isRumbling = true;
}

void updateRumble() {
  if (isRumbling && millis() > rumbleTimer) {
    XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase repo;
    repo.setAllOff();
    xboxController.writeHIDReport(repo);
    isRumbling = false;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("--- PROMETHEUS ARM v4.1 POWER SPEC ---");
  
  pinMode(ledConexion, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(motorA_Fwd, OUTPUT);
  pinMode(motorA_Bwd, OUTPUT);
  pinMode(motorB_Bwd, OUTPUT);
  pinMode(motorB_Fwd, OUTPUT);

  ServoSteering.setPeriodHertz(50);
  ServoSteering.attach(servoSteeringPin, 500, 2400);
  
  ServoArm.setPeriodHertz(50);
  ServoArm.attach(servoArmPin, 600, 2400); 

  ServoClaw.setPeriodHertz(50);
  ServoClaw.attach(servoClawPin, 600, 2400);

  ServoGate.setPeriodHertz(50);
  ServoGate.attach(servoGatePin, 600, 2400);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  if (ServoClaw.attached() && ServoGate.attached()) {
    Serial.println("[OK] Servos de Fuerza enganchados en Pins 33(Garra)/17(Compuerta).");
  } else {
    Serial.println("[ERROR] Fallo al inicializar servos de potencia.");
  }

  xboxController.begin();
  
  ServoSteering.write(STEERING_CENTER);
  ServoArm.write((int)currentArmPos);
  ServoClaw.write(clawOpen ? CLAW_OPEN : CLAW_CLOSE);
  ServoGate.write(GATE_CLOSE);

  tone(buzzerPin, 1000, 200);
}

void loop() {
  xboxController.onLoop();
  updateRumble();

  static bool wasConnected = false;
  static unsigned long connectionTime = 0;
  static bool rumbleTriggered = false;
  bool isConnected = xboxController.isConnected();

  if (isConnected) {
    if (!wasConnected) {
      connectionTime = millis();
      wasConnected = true;
      rumbleTriggered = false;
    }
    if (!rumbleTriggered && (millis() - connectionTime > 2000)) {
      triggerRumble(200, 200, 400); 
      rumbleTriggered = true;
    }
  } else {
    wasConnected = false;
    rumbleTriggered = false;
  }

  if (!isConnected) {
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

  bool btnA_now = xboxController.xboxNotif.btnA;
  bool btnB_now = xboxController.xboxNotif.btnB;
  bool btnRB_now = xboxController.xboxNotif.btnRB;

  if (btnA_now && !btnA_prev) {
    targetArmPos = ARM_DOWN;
    Serial.println(">>> [BRAZO] Objetivo: DOWN");
  }

  bool btnLB_now = xboxController.xboxNotif.btnLB;
  if (btnLB_now && !btnLB_prev) {
    sportMode = !sportMode;
    if (sportMode) {
      tone(buzzerPin, 2000, 100);
      triggerRumble(255, 255, 300); 
    } else {
      tone(buzzerPin, 700, 100);
      triggerRumble(50, 50, 150);   
    }
  }
  btnLB_prev = btnLB_now;

  bool btnDirUp_now = xboxController.xboxNotif.btnDirUp;
  if (btnDirUp_now && !btnDirUp_prev) {
    tankMode = !tankMode;
    if (tankMode) {
      tone(buzzerPin, 1800, 100); 
      triggerRumble(255, 0, 400); 
      Serial.println(">>> [MODO TANQUE] ON");
    } else {
      tone(buzzerPin, 1500, 100); 
      triggerRumble(0, 100, 200); 
      Serial.println(">>> [MODO TANQUE] OFF");
    }
  }
  btnDirUp_prev = btnDirUp_now;

  if (btnB_now && !btnB_prev) {
    targetArmPos = ARM_UP;
    Serial.println(">>> [BRAZO] Objetivo: UP");
  }

  if (btnRB_now && !btnRB_prev) {
    targetArmPos = ARM_DOWN; 
    
    gateOpen = true;
    ServoGate.write(GATE_OPEN); 
    gateAutoTimer = 0; 
    
    clawOpen = true;
    ServoClaw.write(CLAW_OPEN);
    
    tone(buzzerPin, 1500, 200);
    Serial.println(">>> [MACRO RB] Secuencia de Descarga");
  }

  btnA_prev = btnA_now;
  btnB_prev = btnB_now;
  btnRB_prev = btnRB_now;

  static int lastArmPos = -1;
  currentArmPos = targetArmPos;
  if ((int)currentArmPos != lastArmPos) {
    ServoArm.write((int)currentArmPos);
    lastArmPos = (int)currentArmPos;
  }

  if (gateAutoTimer > 0 && millis() - gateAutoTimer > 1000) {
    gateOpen = false;
    ServoGate.write(GATE_CLOSE);
    gateAutoTimer = 0;
    Serial.println(">>> [AUTO-GATE] Cierre automatico");
  }

  bool btnY_now = xboxController.xboxNotif.btnY;
  if (btnY_now && !btnY_prev) {
    clawOpen = !clawOpen;
    int target = clawOpen ? CLAW_OPEN : CLAW_CLOSE;
    ServoClaw.write(target);
    Serial.print(">>> [GARRA] Pin 33 -> "); Serial.println(target);
    tone(buzzerPin, 1200, 100); 
  }
  btnY_prev = btnY_now;

  bool btnX_now = xboxController.xboxNotif.btnX;
  if (btnX_now && !btnX_prev) {
    gateOpen = !gateOpen;
    int target = gateOpen ? GATE_OPEN : GATE_CLOSE;
    ServoGate.write(target);
    gateAutoTimer = 0; 
    Serial.print(">>> [COMPUERTA] Pin 17 -> "); Serial.println(target);
  }
  btnX_prev = btnX_now;

  if (xboxController.xboxNotif.btnStart) {
    stopMotors();
    targetArmPos = 90;
    tone(buzzerPin, 400, 500);
  }

  delay(15); 
}
