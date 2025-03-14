#include <Wire.h>
#include <LCD_I2C.h>

LCD_I2C lcd(0x27, 16, 2); // Adresse I2C 0x27, écran 16x2

const int LED_CLIM = 8;
const int BOUTON_MODE = 2;
float RESISTANCE_REF = 10000; // Résistance de référence
float logResistance, resistanceNTC, temperatureKelvin, temperatureCelsius;

float coeff1 = 1.129148e-03, coeff2 = 2.34125e-04, coeff3 = 8.76741e-08; // Coefficients Steinhart-Hart
int etatClim = 0; // 0 = OFF, 1 = ON

unsigned long dernierMajTemperature = 0;
unsigned long dernierMajSerie = 0;
unsigned long dernierAppuiBouton = 0;
unsigned long dernierMajLCD = 0;

int modeAffichage = 0; // 0 = Température, 1 = Contrôle Joystick
bool etatBouton = HIGH, etatBoutonPrecedent = HIGH;

byte caractereNumero[8] = {
  0B11100,
  0B00100,
  0B11100,
  0B00111,
  0B11101,
  0B00111,
  0B00101,
  0B00111
};

byte caractereDegre[8] = {
  0B01000,
  0B10100,
  0B01000,
  0B00000,
  0B00000,
  0B00000,
  0B00000,
  0B00000
};

void afficherMessageDemarrage() {
    static unsigned long tempsAttente = 3000;

    lcd.createChar(0, caractereNumero);

    while (millis() < tempsAttente) {
        lcd.print("Serge Landry");

        lcd.setCursor(0, 1);
        lcd.print(char(0));

        lcd.setCursor(14, 1);
        lcd.print("38");
    }
}

void setup() {
    pinMode(LED_CLIM, OUTPUT);
    pinMode(BOUTON_MODE, INPUT_PULLUP); // Activer la résistance de pull-up interne
    Serial.begin(115200);

    lcd.begin(16, 2);
    lcd.backlight();
    
    // Affichage du message de démarrage
    afficherMessageDemarrage();
    lcd.clear();
}

void mesurerTemperature() {
    static unsigned long intervalleMaj = 500;

    if (millis() - dernierMajTemperature >= intervalleMaj) { 
        dernierMajTemperature = millis();

        int valeurNTC = analogRead(A0);
        resistanceNTC = RESISTANCE_REF * (1023.0 / valeurNTC - 1.0);
        logResistance = log(resistanceNTC);
        temperatureKelvin = (1.0 / (coeff1 + coeff2 * logResistance + coeff3 * logResistance * logResistance * logResistance));
        temperatureCelsius = temperatureKelvin - 273.15;

        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        lcd.print(temperatureCelsius);
        lcd.print(" C  "); 

        lcd.setCursor(0, 1);
        if (temperatureCelsius > 25) {
            digitalWrite(LED_CLIM, HIGH); 
            lcd.print("AC: ON  ");
            etatClim = 1;
        } else if (temperatureCelsius < 24) {
            digitalWrite(LED_CLIM, LOW);
            lcd.print("AC: OFF ");
            etatClim = 0;
        }
    }
}

void controlerJoystick() {
    static unsigned long intervalleMaj = 200;

    if (millis() - dernierMajLCD >= intervalleMaj) { 
        dernierMajLCD = millis();

        int valeurX = analogRead(A1);
        int valeurY = analogRead(A2);

        int vitesse = (valeurY > 512) ? map(valeurY, 512, 1023, 0, 120) : map(valeurY, 0, 511, -25, 0);
        int direction = map(valeurX, 0, 1023, 90, -90);

        lcd.setCursor(0, 0);
        lcd.print(vitesse > 0 ? "Avance: " : "Recule: "); 
        lcd.print(vitesse);
        lcd.print("km/h  "); 

        lcd.setCursor(0, 1);
        lcd.print(direction < 0 ? "Gauche:" : "Droite:  ");
        lcd.print(direction);
        lcd.print(" ");
        lcd.createChar(1, caractereDegre);
        lcd.setCursor(11, 1);
        lcd.print(char(1));
    }
}

void gererBoutonMode() {
    etatBouton = digitalRead(BOUTON_MODE);

    if (etatBouton == LOW && etatBoutonPrecedent == HIGH) {
        if (millis() - dernierAppuiBouton >= 200) { 
            dernierAppuiBouton = millis();
            modeAffichage = (modeAffichage + 1) % 2;
            lcd.clear();
        }
    }
    etatBoutonPrecedent = etatBouton;

    if (modeAffichage == 0) {
        mesurerTemperature();
    } else {
        controlerJoystick();
    }
}

void loop() {
    gererBoutonMode();
    
    static unsigned long intervalleEnvoiSerie = 100;
    if (millis() - dernierMajSerie >= intervalleEnvoiSerie) {
        dernierMajSerie = millis();

        int valeurX = analogRead(A1);
        int valeurY = analogRead(A2);

        Serial.print("etd:2405238,x:");
        Serial.print(valeurX);
        Serial.print(",y:"); 
        Serial.print(valeurY);
        Serial.print(",sys:"); 
        Serial.println(etatClim);
    }
}
