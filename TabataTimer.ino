#include <M5StickC.h>

// Pins for Speaker HAT
const int SPEAKER_PIN = 26;
const int SD_PIN = 0;

// Tabata settings
const int WORK_TIME = 20;
const int REST_TIME = 10;
const int PREP_TIME = 5;
const int ROUNDS = 8;

enum State { READY, PREP, WORK, REST, FINISHED };

State currentState = READY;
State lastState = FINISHED; 
int lastTimerValue = -1;
int currentRound = 1;
int timerValue = 0;
unsigned long lastTick = 0;
bool paused = false;

// Audio Configuration
const uint8_t LEDC_CHANNEL = 0;
const uint8_t LEDC_RESOLUTION = 8;

// Sprite for Double Buffering
TFT_eSprite canvas = TFT_eSprite(&M5.Lcd);

void setup() {
  M5.begin();
  M5.Lcd.setRotation(0); 
  
  canvas.createSprite(80, 160);
  canvas.setRotation(0);

  pinMode(SD_PIN, OUTPUT);
  digitalWrite(SD_PIN, LOW); 
  
  ledcSetup(LEDC_CHANNEL, 2000, LEDC_RESOLUTION);
  ledcAttachPin(SPEAKER_PIN, LEDC_CHANNEL);

  M5.Lcd.fillScreen(BLACK);
}

void playTone(int freq, int duration) {
  if (freq > 0) {
    digitalWrite(SD_PIN, HIGH); 
    ledcWriteTone(LEDC_CHANNEL, freq);
    delay(duration);
    ledcWriteTone(LEDC_CHANNEL, 0);
    digitalWrite(SD_PIN, LOW); 
  } else {
    delay(duration);
  }
}

void beepShort() { playTone(1800, 150); }
void beepMid()   { playTone(1200, 300); }
void beepStart() { playTone(2500, 600); }

void playFanfare() {
  playTone(523, 150); // C5
  playTone(659, 150); // E5
  playTone(784, 150); // G5
  playTone(1046, 400); // C6
  delay(100);
  playTone(784, 150); // G5
  playTone(1046, 600); // C6
}

void drawUI() {
  canvas.fillSprite(BLACK);
  
  uint16_t headColor = DARKGREY;
  if (currentState == PREP) headColor = ORANGE;
  if (currentState == WORK) headColor = RED;
  if (currentState == REST) headColor = GREEN;
  if (currentState == FINISHED) headColor = BLUE;
  
  canvas.fillRect(0, 0, 80, 22, headColor);
  canvas.setTextColor(currentState == REST || (currentState == PREP && headColor == ORANGE) ? BLACK : WHITE);
  canvas.setTextSize(2);
  
  const char* title = "TABATA";
  if (currentState == PREP) title = "PREP";
  if (currentState == WORK) title = "WORK";
  if (currentState == REST) title = "REST";
  if (currentState == FINISHED) title = "DONE";
  canvas.drawCentreString(title, 40, 3, 1);

  if (currentState == READY) {
    canvas.setTextColor(CYAN);
    canvas.setTextSize(2);
    canvas.drawCentreString("READY", 40, 55, 1);
    canvas.setTextColor(WHITE);
    canvas.setTextSize(1);
    canvas.drawCentreString("[A] to START", 40, 120, 1);
  } else if (currentState == FINISHED) {
    canvas.setTextColor(YELLOW);
    canvas.setTextSize(2);
    canvas.drawCentreString("FINISH", 40, 50, 1);
    canvas.setTextColor(WHITE);
    canvas.setTextSize(1);
    canvas.drawCentreString("Good job!", 40, 80, 1);
    canvas.drawCentreString("[B] to RESET", 40, 120, 1);
  } else {
    // Round indicator - MADE LARGER AND MORE VISIBLE
    canvas.setTextColor(WHITE);
    canvas.setTextSize(2);
    char roundStr[16];
    sprintf(roundStr, "%d / %d", currentRound, ROUNDS);
    canvas.drawCentreString(roundStr, 40, 30, 1);
    canvas.setTextSize(1);
    canvas.setTextColor(LIGHTGREY);
    canvas.drawCentreString("ROUND", 40, 50, 1);

    // Progress Bar
    int total = (currentState == WORK) ? WORK_TIME : (currentState == PREP ? PREP_TIME : REST_TIME);
    int progressWidth = map(timerValue, 0, total, 0, 70);
    canvas.drawRect(5, 145, 70, 6, WHITE);
    uint16_t barColor = (currentState == WORK ? RED : (currentState == PREP ? ORANGE : GREEN));
    canvas.fillRect(5, 145, progressWidth, 6, barColor);

    // Large Timer
    uint16_t timerColor = (paused) ? DARKGREY : (timerValue <= 3 ? RED : WHITE);
    canvas.setTextColor(timerColor);
    canvas.setTextSize(6);
    int xPos = (timerValue >= 10) ? 5 : 23;
    canvas.setCursor(xPos, 60); // Moved up to 60 to avoid overlap
    canvas.print(timerValue);
    
    if (paused) {
      canvas.setTextColor(YELLOW);
      canvas.setTextSize(2);
      canvas.drawCentreString("PAUSE", 40, 120, 1);
    }
  }

  canvas.pushSprite(0, 0);
}

void loop() {
  M5.update();

  if (M5.BtnA.wasPressed()) {
    if (currentState == READY) {
      currentState = PREP;
      timerValue = PREP_TIME;
      lastTick = millis();
    } else if (currentState != FINISHED) {
      paused = !paused;
    }
  }

  if (M5.BtnB.wasPressed()) {
    currentState = READY;
    currentRound = 1;
    timerValue = 0;
    paused = false;
  }

  drawUI();

  if (currentState != READY && currentState != FINISHED && !paused) {
    if (millis() - lastTick >= 1000) {
      lastTick = millis();
      timerValue--;

      if (timerValue >= 1 && timerValue <= 3) {
        beepShort();
      }

      if (timerValue == 0) {
        if (currentState == PREP) {
          currentState = WORK;
          timerValue = WORK_TIME;
          beepStart();
        } else if (currentState == WORK) {
          // CHECK IF LAST ROUND
          if (currentRound >= ROUNDS) {
            currentState = FINISHED;
            drawUI(); // Force draw before fanfare delays
            playFanfare();
          } else {
            currentState = REST;
            timerValue = REST_TIME;
            beepMid();
          }
        } else if (currentState == REST) {
          currentRound++;
          currentState = WORK;
          timerValue = WORK_TIME;
          beepStart();
        }
      }
    }
  }
  delay(10);
}
