/*
 * Motor Cap Alignment — Sensor Indutivo com ESP32
 *
 * Sensor:  METALTEX I18-8-ANV (0–10V, range 8mm)
 * ADC:     ADS1115 via I2C — após divisor 3:1 (10V → 3.33V)
 * Display: LCD 1602 I2C (PCF8574, endereço 0x27)
 * Botão:   Botão discreto → GPIO 32 (INPUT_PULLUP, ativo em LOW)
 * MCU:     ESP32 NodeMCU-32 (30 pinos, 3.3V)
 *
 * ── Mapeamento de pinos ────────────────────────────────────────────────────
 *  I2C SDA     → GPIO 21  (ADS1115 + LCD I2C — barramento compartilhado)
 *  I2C SCL     → GPIO 22  (ADS1115 + LCD I2C — barramento compartilhado)
 *  Botão       → GPIO 32  (INPUT_PULLUP — botão liga GPIO ao GND)
 *
 * Libraries: Adafruit ADS1X15 | LiquidCrystal_I2C (Frank de Brabander)
 */

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <LiquidCrystal_I2C.h>

// ── Configuração LCD I2C ───────────────────────────────────────────────────
#define LCD_I2C_ADDR  0x27  // endereço típico PCF8574 (tente 0x3F se não funcionar)
#define LCD_COLS      16
#define LCD_ROWS      2

// ── Pino do botão discreto ────────────────────────────────────────────────
#define BTN_PIN  23   // GPIO23 — INPUT_PULLUP, pressão liga ao GND

// ── Constantes de conversão do sensor ────────────────────────────────────
constexpr float ADS_LSB_MV      = 0.1875f;  // GAIN_TWOTHIRDS: 1 LSB = 0.1875 mV
constexpr float DIVIDER_RATIO   = 3.0f;      // R1=10kΩ, R2=5kΩ → 10V → 3.33V
constexpr float SENSOR_RANGE_MM = 8.0f;      // range nominal I18-8-ANV
constexpr float SENSOR_MAX_V    = 10.0f;     // tensão de fundo de escala

// ── Parâmetros de operação ────────────────────────────────────────────────
constexpr unsigned long CAL_DURATION_MS = 10000UL;  // 10 s
constexpr float         TOLERANCE_MM   = 1.0f;      // ±1 mm OK/NOK
constexpr unsigned long DEBOUNCE_MS    = 60UL;
constexpr unsigned long DISPLAY_MS     = 150UL;

// ── Objetos ───────────────────────────────────────────────────────────────
Adafruit_ADS1115  ads;
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

// ── Estado global ─────────────────────────────────────────────────────────
float         referenceMM  = 0.0f;
bool          isCalibrated = false;
unsigned long lastDisplay  = 0;

// ─────────────────────────────────────────────────────────────────────────
float readDistanceMM() {
    int16_t raw        = ads.readADC_SingleEnded(0);
    float   voltADS    = (raw * ADS_LSB_MV) / 1000.0f;
    float   voltSensor = voltADS * DIVIDER_RATIO;
    return constrain((voltSensor / SENSOR_MAX_V) * SENSOR_RANGE_MM, 0.0f, SENSOR_RANGE_MM);
}

bool buttonPressed() {
    return digitalRead(BTN_PIN) == LOW;  // INPUT_PULLUP: LOW = pressionado
}

// ─────────────────────────────────────────────────────────────────────────
void runCalibration() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Zerando...      ");

    double sum   = 0;
    int    count = 0;
    unsigned long start = millis();

    while (millis() - start < CAL_DURATION_MS) {
        sum += readDistanceMM();
        count++;

        int secsLeft = (int)((CAL_DURATION_MS - (millis() - start)) / 1000) + 1;
        lcd.setCursor(0, 1);
        lcd.print("Tempo: ");
        lcd.print(secsLeft);
        lcd.print("s        ");

        delay(100);
    }

    referenceMM  = (float)(sum / count);
    isCalibrated = true;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ref:");
    lcd.print(referenceMM, 2);
    lcd.print(" mm     ");
    lcd.setCursor(0, 1);
    lcd.print("  Calibrado!    ");
    delay(2000);
}

// ─────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    pinMode(BTN_PIN, INPUT_PULLUP);

    Wire.begin();
    ads.setGain(GAIN_TWOTHIRDS);  // ±6.144V
    if (!ads.begin()) {
        Serial.println("ADS1115 nao encontrado!");
    }

    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print(" Motor Alignment");
    lcd.setCursor(0, 1);
    lcd.print(" Pressione BTN  ");
    delay(1500);
    lcd.clear();
}

// ─────────────────────────────────────────────────────────────────────────
void loop() {
    // Detecta botão com debounce
    static bool lastBtn = false;
    bool curBtn = buttonPressed();
    if (!lastBtn && curBtn) {
        delay(DEBOUNCE_MS);
        if (buttonPressed()) {
            runCalibration();
        }
    }
    lastBtn = curBtn;

    // Atualiza display
    if (millis() - lastDisplay >= DISPLAY_MS) {
        lastDisplay = millis();

        float dist = readDistanceMM();
        char  buf[17];

        lcd.setCursor(0, 0);
        snprintf(buf, sizeof(buf), "Dist:%5.2f mm   ", dist);
        lcd.print(buf);

        lcd.setCursor(0, 1);
        if (!isCalibrated) {
            lcd.print(" Pressione BTN  ");
        } else {
            float delta = dist - referenceMM;
            if (fabsf(delta) <= TOLERANCE_MM) {
                snprintf(buf, sizeof(buf), "d=%+.2fmm  OK  ", delta);
            } else {
                snprintf(buf, sizeof(buf), "d=%+.2fmm NOK  ", delta);
            }
            lcd.print(buf);
        }

        Serial.printf("dist=%.3f mm | ref=%.3f mm | cal=%d\n",
                      dist, referenceMM, isCalibrated);
    }
}
