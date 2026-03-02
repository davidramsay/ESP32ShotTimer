#include <Wire.h>
#include <LiquidCrystal_I2C.h>

  LiquidCrystal_I2C lcd(0x27, 16, 2);  // 0x20 is common MCP23008 address

// ---- Pins ----
const int SHOT_INPUT_PIN = D1;
const int BUZZER_PIN = D2;
const int START_STOP_PIN = D3;
const int UP_BUTTON_PIN = D7;
const int DOWN_BUTTON_PIN = D9;

// ---- Timing ----
unsigned long randomDelayStart;
unsigned long randomDelayDuration;
unsigned long buzzerTime;
unsigned long lastShotTime;
unsigned long stopHoldStart;
unsigned long lastScrollTime = 0;

bool firstShotCaptured = false;
bool lastShotState = HIGH;
bool lastButtonState = HIGH;
bool lastUpState = HIGH;
bool lastDownState = HIGH;

#define MAX_SPLITS 50
float splits[MAX_SPLITS];
int splitCount = 0;

int scrollIndex = 0;

enum State {
  IDLE,
  WAIT_RANDOM,
  BUZZER_ON,
  TIMING,
  REVIEW
};

State currentState = IDLE;

// ------------------------------------------------

void setup()
{
  pinMode(START_STOP_PIN, INPUT_PULLUP);
  pinMode(UP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(SHOT_INPUT_PIN, INPUT);

  digitalWrite(BUZZER_PIN, LOW);
  Wire.begin(SDA, SCL);
  Wire.setClock(100000);   // 100kHz
  Serial.begin(9600);

  randomSeed(analogRead(A0));

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Shot Timer Ready");
  // lcd.setBacklight(1);
  // lcd.print("READY");
}

// ------------------------------------------------

void loop()
{
  unsigned long now = millis();

  bool buttonState = digitalRead(START_STOP_PIN);
  bool shotState = digitalRead(SHOT_INPUT_PIN);
  bool upState = digitalRead(UP_BUTTON_PIN);
  bool downState = digitalRead(DOWN_BUTTON_PIN);

  switch (currentState)
  {
    case IDLE:
      if (buttonState == LOW && lastButtonState == HIGH)
      {
        randomDelayDuration = random(1000, 5000);
        randomDelayStart = now;
        lcd.clear();
        lcd.print("Standby...");
        currentState = WAIT_RANDOM;
      }
      break;

    case WAIT_RANDOM:
      if (now - randomDelayStart >= randomDelayDuration)
      {
        digitalWrite(BUZZER_PIN, HIGH);
        buzzerTime = now;
        lastShotTime = now;
        firstShotCaptured = false;
        splitCount = 0;
        currentState = BUZZER_ON;
      }
      break;

    case BUZZER_ON:
      if (now - buzzerTime >= 100)
      {
        digitalWrite(BUZZER_PIN, LOW);
        lcd.clear();
        lcd.print("GO!");
        currentState = TIMING;
      }
      break;

    case TIMING:

      // Shot rising edge
      if (shotState == LOW && lastShotState == HIGH)
      {
        if (splitCount < MAX_SPLITS)
        {
          unsigned long split;

          if (!firstShotCaptured)
          {
            split = now - buzzerTime;
            firstShotCaptured = true;
          }
          else
          {
            split = now - lastShotTime;
          }

          lastShotTime = now;

          splits[splitCount] = split / 1000.0;
          splitCount++;
        }
      }

      // Stop pressed once
      if (buttonState == LOW && lastButtonState == HIGH)
      {
        scrollIndex = 0;
        currentState = REVIEW;
        displaySplits();
      }

      break;

    case REVIEW:

  // Scroll UP
  if (upState == LOW && lastUpState == HIGH && millis() - lastScrollTime > 150)
  {
    if (scrollIndex > 0)
      scrollIndex--;

    displaySplits();
    lastScrollTime = millis();
  }

  // Scroll DOWN
  if (downState == LOW && lastDownState == HIGH && millis() - lastScrollTime > 150)
  {
    int maxPage;

    if (splitCount == 0)
      maxPage = 0;
    else
      maxPage = 1 + ((splitCount + 1) / 2);  
      // 0 = TOT
      // 1 = AVG/FAST
      // remaining = split pages (2 splits per page)

    if (scrollIndex < maxPage)
      scrollIndex++;

    displaySplits();
    lastScrollTime = millis();
  }

  // Hold START to reset
  if (buttonState == LOW)
  {
    if (stopHoldStart == 0)
      stopHoldStart = now;

    if (now - stopHoldStart >= 2000)
      resetTimer();
  }
  else
  {
    stopHoldStart = 0;
  }

  break;
  }

  lastShotState = shotState;
  lastButtonState = buttonState;
  lastUpState = upState;
  lastDownState = downState;
}

// ------------------------------------------------
float calculateTotalTime()
{
  float total = 0.0;
  for (int i = 0; i < splitCount; i++)
    total += splits[i];
  return total;
}

float calculateAverageMinusFirst()
{
  if (splitCount <= 1) return 0.0;

  float total = 0.0;
  for (int i = 1; i < splitCount; i++)
    total += splits[i];

  return total / (splitCount - 1);
}

float findFastestSplit()
{
  if (splitCount <= 1) return 0.0;

  float fastest = splits[1];   // ignore first shot
  for (int i = 2; i < splitCount; i++)
  {
    if (splits[i] < fastest)
      fastest = splits[i];
  }

  return fastest;
}

int findFastestSplitIndex()
{
  if (splitCount <= 1) return -1;

  int fastestIndex = 1;   // exclude first shot
  float fastest = splits[1];

  for (int i = 2; i < splitCount; i++)
  {
    if (splits[i] < fastest)
    {
      fastest = splits[i];
      fastestIndex = i;
    }
  }

  return fastestIndex;
}

void displaySplits()
{
  // Clear rows
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");

  if (splitCount == 0)
  {
    lcd.setCursor(0, 0);
    lcd.print("NO SHOTS");
    lcd.setCursor(0, 1);
    lcd.print("RECORDED");
    return;
  }

  float total = calculateTotalTime();
  float avg = calculateAverageMinusFirst();
  int fastestIndex = findFastestSplitIndex();
  float fastest = findFastestSplit();

  // ---------- PAGE 0: TOTAL ----------
  if (scrollIndex == 0)
  {
    lcd.setCursor(0, 0);
    lcd.print("TOT:");
    lcd.print(total, 2);
    return;
  }

  // ---------- PAGE 1: AVG + FAST ----------
  if (scrollIndex == 1)
  {
    lcd.setCursor(0, 0);
    lcd.print("AVG:");
    lcd.print(avg, 2);

    lcd.setCursor(0, 1);
    lcd.print("FAST:");
    lcd.print(fastest, 2);
    return;
  }

  // ---------- SPLIT PAGES ----------
  int index = scrollIndex - 2;

  if (index < splitCount)
  {
    lcd.setCursor(0, 0);
    lcd.print(index + 1);
    lcd.print(": ");
    lcd.print(splits[index], 2);

    if (index == fastestIndex)
      lcd.print("*");
  }

  if (index + 1 < splitCount)
  {
    lcd.setCursor(0, 1);
    lcd.print(index + 2);
    lcd.print(": ");
    lcd.print(splits[index + 1], 2);

    if (index + 1 == fastestIndex)
      lcd.print("*");
  }
}

// ------------------------------------------------

void resetTimer()
{
  splitCount = 0;
  scrollIndex = 0;
  firstShotCaptured = false;
  stopHoldStart = 0;

  lcd.clear();
  lcd.print("READY");

  currentState = IDLE;
}