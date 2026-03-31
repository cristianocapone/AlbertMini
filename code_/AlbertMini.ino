#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <math.h>

#if defined(ESP32)
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;
#endif

// =====================================================
// CONFIGURATION
// =====================================================
#define SERVOMIN     100
#define SERVOMAX     500
#define CENTER       325
#define NUM_SERVOS   8       
#define NECK_SERVO   15      
#define DELAY_TIME   5       
#define SPEED_FACTOR 0.5f    
#define MAX_STEP     8       
#define MOVE_STEPS   35      

// =====================================================
// GAIT PARAMETERS (Realistic Diagonal Trot)
// =====================================================
const float FREQUENCY = 0.005f;
const float AMPLITUDE = 45.0f;
const float phase[NUM_SERVOS] = {0, PI/2, PI, 3*PI/2, PI, 3*PI/2, 0, PI/2};
const float off_set_walk[NUM_SERVOS] = { 0, -80, 0, 80, -40, -40, 40, 40 };

unsigned long currentMillis = 0, oldMillis = 0, innerTime = 0;
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40); 
float currentPos[NUM_SERVOS], targetPos[NUM_SERVOS];
int lastPulse[NUM_SERVOS];

enum Mode { MODE_IDLE, MODE_WALK_FORWARD, MODE_SPIN_LEFT, MODE_SPIN_RIGHT };
Mode currentMode = MODE_IDLE;

// =====================================================
// CORE HELPERS
// =====================================================
void moveUntilReachedAll() {
  float startPos[NUM_SERVOS];
  for (int i = 0; i < NUM_SERVOS; i++) startPos[i] = currentPos[i];
  for (int step = 1; step <= MOVE_STEPS; step++) {
    float t = (float)step / (float)MOVE_STEPS;
    for (int i = 0; i < NUM_SERVOS; i++) {
      currentPos[i] = startPos[i] + t * (targetPos[i] - startPos[i]);
      lastPulse[i]  = (int)currentPos[i];
      pwm.setPWM(i, 0, lastPulse[i]);
    }
    delay(DELAY_TIME);
  }
}

void updateAllServos() {
  for (int i = 0; i < NUM_SERVOS; i++) {
    float diff = targetPos[i] - currentPos[i];
    if (fabsf(diff) > 0.5f) {
      currentPos[i] += constrain(diff * SPEED_FACTOR, -MAX_STEP, (float)MAX_STEP);
    }
    pwm.setPWM(i, 0, (int)currentPos[i]);
  }
  delay(DELAY_TIME);
}

void syncCurrentPos() {
  for (int i = 0; i < NUM_SERVOS; i++) {
    currentPos[i] = (float)lastPulse[i];
    targetPos[i]  = (float)lastPulse[i];
  }
}

void sendReply(String msg) { 
  Serial.println(msg); 
#if defined(ESP32) 
  SerialBT.println(msg); 
#endif 
}

// =====================================================
// THE TRICKS (Full Suite)
// =====================================================
void runUpSequence()   { for (int i=0; i<NUM_SERVOS; i++) targetPos[i]=CENTER; moveUntilReachedAll(); }

void runDownSequence() { 
  targetPos[0]=CENTER-125; targetPos[1]=CENTER+125; targetPos[2]=CENTER+125; targetPos[3]=CENTER-125; 
  targetPos[4]=CENTER-125; targetPos[5]=CENTER+125; targetPos[6]=CENTER+125; targetPos[7]=CENTER-125; 
  moveUntilReachedAll(); 
}

void runPushupsSequence() {
  sendReply("PUSHUPS...");
  for (int r=0; r<5; r++) { runDownSequence(); delay(200); runUpSequence(); delay(200); }
}

void runSwingSequence() {
  sendReply("SWING...");
  for (int i = 0; i < 4; i++) {
    targetPos[0]=CENTER; targetPos[1]=CENTER; targetPos[4]=CENTER; targetPos[5]=CENTER;
    targetPos[2]=CENTER+130; targetPos[3]=CENTER-130; targetPos[6]=CENTER+130; targetPos[7]=CENTER-130;
    moveUntilReachedAll(); delay(150);
    targetPos[0]=CENTER-130; targetPos[1]=CENTER+130; targetPos[4]=CENTER-130; targetPos[5]=CENTER+130;
    targetPos[2]=CENTER; targetPos[3]=CENTER; targetPos[6]=CENTER; targetPos[7]=CENTER;
    moveUntilReachedAll(); delay(150);
  }
  runUpSequence();
}

void runWormSequence() {
  sendReply("WORM...");
  for (int i = 0; i < 5; i++) {
    targetPos[0]=CENTER-100; targetPos[1]=CENTER+100; targetPos[2]=CENTER+100; targetPos[3]=CENTER-100;
    targetPos[4]=CENTER+130; targetPos[5]=CENTER-50;  targetPos[6]=CENTER-130; targetPos[7]=CENTER+50;
    moveUntilReachedAll(); delay(100);
    targetPos[4]=CENTER-100; targetPos[5]=CENTER+100; targetPos[6]=CENTER+100; targetPos[7]=CENTER-100;
    targetPos[0]=CENTER+130; targetPos[1]=CENTER-50;  targetPos[2]=CENTER-130; targetPos[3]=CENTER+50;
    moveUntilReachedAll(); delay(100);
  }
  runUpSequence();
}

void runGallopSequence() {
  sendReply("GALLOP...");
  for (int i = 0; i < 8; i++) {
    targetPos[0]=CENTER+150; targetPos[1]=CENTER-100; targetPos[2]=CENTER-150; targetPos[3]=CENTER+100;
    targetPos[4]=CENTER-150; targetPos[5]=CENTER+50;  targetPos[6]=CENTER+150; targetPos[7]=CENTER-50;
    moveUntilReachedAll(); delay(50);
    targetPos[0]=CENTER-100; targetPos[1]=CENTER+50;  targetPos[2]=CENTER+100; targetPos[3]=CENTER-50;
    targetPos[4]=CENTER+150; targetPos[5]=CENTER-100; targetPos[6]=CENTER-150; targetPos[7]=CENTER+100;
    moveUntilReachedAll(); delay(50);
  }
  runUpSequence();
}

void runJumpSequence() {
  sendReply("JUMP!");
  runDownSequence(); delay(500); 
  for (int i = 0; i < NUM_SERVOS; i++) {
    int p = (i % 4 < 2) ? CENTER + 150 : CENTER - 150;
    pwm.setPWM(i, 0, p); currentPos[i] = targetPos[i] = p;
  }
  delay(300); runUpSequence();
}

void runDanceSequence() {
  sendReply("DANCE!");
  for (int i = 0; i < 3; i++) {
    pwm.setPWM(NECK_SERVO, 0, CENTER+120); targetPos[0]=CENTER+100; targetPos[2]=CENTER-50; moveUntilReachedAll(); delay(100);
    pwm.setPWM(NECK_SERVO, 0, CENTER-120); targetPos[0]=CENTER-50; targetPos[2]=CENTER+100; moveUntilReachedAll(); delay(100);
  }
  pwm.setPWM(NECK_SERVO, 0, CENTER); runDownSequence(); delay(500); runUpSequence();
}
void runUndulateSequence() {
  sendReply("UNDULATING: Smooth waves...");
  
  // 1. INITIAL STRETCH
  // Front legs reach way out, back legs reach way back
  targetPos[0] = CENTER + 140; targetPos[1] = CENTER - 60;
  targetPos[2] = CENTER - 140; targetPos[3] = CENTER + 60;
  targetPos[4] = CENTER - 140; targetPos[5] = CENTER + 60;
  targetPos[6] = CENTER + 140; targetPos[7] = CENTER - 60;
  moveUntilReachedAll();
  delay(200);

  // 2. THE WAVE (8 cycles of organic motion)
  for (int i = 0; i < 400; i++) {
    float wave = sin(i * 0.05) * 60.0; // The "wiggle" factor
    
    // Front Hips (0, 2) and Back Hips (4, 6) wiggle out of phase
    pwm.setPWM(0, 0, CENTER + 140 + (int)wave);
    pwm.setPWM(2, 0, CENTER - 140 + (int)wave);
    pwm.setPWM(4, 0, CENTER - 140 - (int)wave);
    pwm.setPWM(6, 0, CENTER + 140 - (int)wave);
    
    delay(10); // Very fast updates for smoothness
  }

  runUpSequence();
  sendReply("UNDULATE DONE");
}
// =====================================================
// GAIT ENGINE
// =====================================================
void runGait(const float ampScale[NUM_SERVOS]) {
  oldMillis = currentMillis; currentMillis = millis(); innerTime += currentMillis - oldMillis;
  float t = FREQUENCY * innerTime;
  for (int s = 0; s < NUM_SERVOS; s += 2) { 
    int pulse = CENTER + (int)off_set_walk[s] + (int)(ampScale[s] * AMPLITUDE * cosf(t + phase[s]));
    pwm.setPWM(s, 0, pulse); lastPulse[s] = pulse;
  }
  for (int s = 1; s < NUM_SERVOS; s += 2) { 
    float c = fmaxf(0.0f, cosf(t + phase[s]));
    int pulse = CENTER + (int)off_set_walk[s] + (int)(ampScale[s] * (AMPLITUDE + 20) * c);
    pwm.setPWM(s, 0, pulse); lastPulse[s] = pulse;
  }
  delay(DELAY_TIME);
}

// =====================================================
// SETUP & LOOP
// =====================================================
void setup() {
  Serial.begin(115200);
#if defined(ESP32)
  SerialBT.begin("Albert_RobotDog");
#endif
  pwm.begin(); pwm.setPWMFreq(50);
  for (int i = 0; i < NUM_SERVOS; i++) { currentPos[i] = targetPos[i] = lastPulse[i] = CENTER; pwm.setPWM(i, 0, CENTER); }
  pwm.setPWM(NECK_SERVO, 0, CENTER);
  sendReply("Albert 100% Loaded!");
}

void loop() {
  String input = "";
  if (Serial.available() > 0) input = Serial.readStringUntil('\n');
#if defined(ESP32)
  else if (SerialBT.available() > 0) input = SerialBT.readStringUntil('\n');
#endif
  input.trim();

  if (input.length() > 0) {
    input.toUpperCase();
    if      (input == "WALK")    currentMode = MODE_WALK_FORWARD;
    else if (input == "LEFT")    currentMode = MODE_SPIN_LEFT;
    else if (input == "RIGHT")   currentMode = MODE_SPIN_RIGHT;
    else if (input == "STOP")    currentMode = MODE_IDLE;
    else if (input == "PUSHUPS") { currentMode = MODE_IDLE; syncCurrentPos(); runPushupsSequence(); }
    else if (input == "DANCE")   { currentMode = MODE_IDLE; syncCurrentPos(); runDanceSequence(); }
    else if (input == "SWING")   { currentMode = MODE_IDLE; syncCurrentPos(); runSwingSequence(); }
    else if (input == "WORM")    { currentMode = MODE_IDLE; syncCurrentPos(); runWormSequence(); }
    else if (input == "GALLOP")  { currentMode = MODE_IDLE; syncCurrentPos(); runGallopSequence(); }
    else if (input == "JUMP")    { currentMode = MODE_IDLE; syncCurrentPos(); runJumpSequence(); }
    else if (input == "UP")      { currentMode = MODE_IDLE; syncCurrentPos(); runUpSequence(); }
    else if (input == "DOWN")    { currentMode = MODE_IDLE; syncCurrentPos(); runDownSequence(); }
    else if (input == "UNDULATE") { currentMode = MODE_IDLE; syncCurrentPos(); runUndulateSequence(); }
  }

  if (currentMode == MODE_WALK_FORWARD) {
    static const float a[] = { 1.0f, 1.2f, 1.0f, 1.2f, 1.0f, 1.2f, 1.0f, 1.2f };
    runGait(a);
  } 
  else if (currentMode == MODE_SPIN_LEFT) {
    static const float a[] = { -1.2f, 1.2f, 1.2f, 1.2f, -1.2f, 1.2f, 1.2f, 1.2f };
    runGait(a);
  }
  else if (currentMode == MODE_SPIN_RIGHT) {
    static const float a[] = { 1.2f, 1.2f, -1.2f, 1.2f, 1.2f, 1.2f, -1.2f, 1.2f };
    runGait(a);
  }
  else {
    updateAllServos();
  }
}