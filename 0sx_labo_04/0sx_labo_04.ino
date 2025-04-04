#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Stepper.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int stepsPerRevolution = 2048;
Stepper moteur(stepsPerRevolution, 4, 5, 6, 7);

int anglePorte = 10;
int angleCible = 10;

const int trigPin = 9;
const int echoPin = 8;
float distance = 0;

enum AppState { STOP, STATE_A };
AppState appState = STOP;

unsigned long currentTime = 0;
bool affichageInitialFait = false;
unsigned long tempsDebut = 0;

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  moteur.setSpeed(13);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("2412384");
  lcd.setCursor(0, 1);
  lcd.print("Labo 4A");
  tempsDebut = millis();
}

void loop() {
  currentTime = millis();

  if (!affichageInitialFait && (currentTime - tempsDebut >= 2000)) {
    affichageInitialFait = true;
    lcd.clear();
  }

  if (affichageInitialFait) {
    lireDistance(currentTime);
    afficherLCD(currentTime);
    envoyerSerie(currentTime);
    stateManager(currentTime);
  }
}


void stateManager(unsigned long ct) {
  switch (appState) {
    case STOP:
      appState = STATE_A;
      break;
    case STATE_A:
      stateA(ct);
      break;
  }
}

void stateA(unsigned long ct) {
  static unsigned long debutMouvement = 0;
  static unsigned long dernierStep = 0;
  static enum SousEtat { IDLE, OUVERTURE, FERMETURE } sousEtat = IDLE;

  const int totalAngle = 160;
  const int dureeMouvement = 2000;
  const int intervalleParPas = dureeMouvement / totalAngle;

  if (sousEtat == IDLE) {
    if (distance < 30 && anglePorte < 170) {
      angleCible = 170;
      debutMouvement = ct;
      dernierStep = ct;
      sousEtat = OUVERTURE;
    } else if (distance > 60 && anglePorte > 10) {
      angleCible = 10;
      debutMouvement = ct;
      dernierStep = ct;
      sousEtat = FERMETURE;
    }
  }

  if (sousEtat == OUVERTURE) {
    if (ct - dernierStep >= intervalleParPas && anglePorte < angleCible) {
      moteur.step(2);
      anglePorte++;
      dernierStep = ct;
    }
    if ((ct - debutMouvement >= dureeMouvement) || anglePorte >= angleCible) {
      detachMoteur();
      sousEtat = IDLE;
    }
  }

  if (sousEtat == FERMETURE) {
    if (ct - dernierStep >= intervalleParPas && anglePorte > angleCible) {
      moteur.step(-2);
      anglePorte--;
      dernierStep = ct;
    }
    if ((ct - debutMouvement >= dureeMouvement) || anglePorte <= angleCible) {
      detachMoteur();
      sousEtat = IDLE;
    }
  }
}

float lireDistance(unsigned long ct) {
  static unsigned long lastTime = 0;
  if (ct - lastTime < 50) return distance;
  lastTime = ct;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2.0;
  return distance;
}

void afficherLCD(unsigned long ct) {
  static unsigned long lastTime = 0;
  if (ct - lastTime < 100) return;
  lastTime = ct;

  lcd.setCursor(0, 0);
  lcd.print("Dist : ");
  if (distance < 10) lcd.print(" ");
  lcd.print((int)distance);
  lcd.print(" cm    ");

  lcd.setCursor(0, 1);
  lcd.print("Porte : ");
  if (anglePorte == 10) lcd.print("Fermee      ");
  else if (anglePorte == 170) lcd.print("Ouverte     ");
  else {
    lcd.print(anglePorte);
    lcd.print(" deg   ");
  }
}

void envoyerSerie(unsigned long ct) {
  static unsigned long lastTime = 0;
  if (ct - lastTime < 100) return;
  lastTime = ct;

  Serial.print("etd:2412384,dist:");
  Serial.print((int)distance);
  Serial.print(",deg:");
  Serial.println(anglePorte);
}

void detachMoteur() {
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
}
