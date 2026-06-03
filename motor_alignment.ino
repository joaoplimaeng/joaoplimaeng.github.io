/*
 * Motor Cap Alignment — Sensor Indutivo com ESP32
 *
 * Sensor:  METALTEX I18-8-ANV (0–10V, range 8mm)
 * ADC:     ADS1115 via I2C — após divisor 2:1 (10V → 5V)
 * Display: LCD Keypad Shield 1602 (paralelo 4-bit)
 * Botão:   SELECT do shield → pino analógico (divisor resistivo)
 * MCU:     ESP32 NodeMCU-32 (30 pinos, 3.3V)
 *
 * ── Mapeamento de pinos ────────────────────────────────────────────────────
 *  Shield → ESP32
 *  RS          → GPIO 19
 *  EN          → GPIO 23
 *  D4          → GPIO 18
 *  D5          → GPIO 17
 *  D6          → GPIO 16
 *  D7          → GPIO 15
 *  Botões (A0) → GPIO 34  (só leitura, ADC1_CH6)
 *  Backlight   → 3.3V (sempre ligado)
 *  I2C SDA     → GPIO 21  (ADS1115)
 *  I2C SCL     → GPIO 22  (ADS1115)
 *
 * ATENÇÃO: alimente o rail de botões do shield com 3.3V (não 5V),
 *          pois o GPIO34 do ESP32 suporta no máximo 3.3V.
 *
 * Libraries: Adafruit ADS1X15 | LiquidCrystal (built-in Arduino/ESP32)
 */

#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <LiquidCrystal.h>

// ── Pinos LCD (paralelo 4-bit) ─────────────────────────────────────────────
#define LCD_RS  19
#define LCD_EN  23
#define LCD_D4  18
#define LCD_D5  17
#define LCD_D6  16
#define LCD_D7  15

// ── Pino analógico dos botões do shield ───────────────────────────────────
#define BTN_PIN 34   // GPIO34 — input only, ADC1_CH6

// ── Limites ADC para o botão SELECT (3.3V rail, 12-bit = 0–4095) ──────────
// Ladder: VCC(3.3V) — 2kΩ — A0 — 3.3kΩ — GND  →  ~2.05V → ADC ≈ 2549
#define BTN_SELECT_LOW  1800
#define BTN_SELECT_HIGH 3200

// ── Constantes de conversão do sensor ────────────────────────────────────
constexpr float ADS_LSB_MV      = 0.1875f;  // GAIN_TWOTHIRDS: 1 LSB = 0.1875 mV
constexpr float DIVIDER_RATIO   = 2.0f;      // R1=R2=10kΩ → 10V → 5V
constexpr float SENSOR_RANGE_MM = 8.0f;      // range nominal I18-8-ANV
constexpr float SENSOR_MAX_V    = 10.0f;     // tensão de fundo de escala

// ── Parâmetros de operação ────────────────────────────────────────────────
constexpr unsigned long CAL_DURATION_MS = 10000UL;  // 10 s
constexpr float         TOLERANCE_MM   = 1.0f;      // ±1 mm OK/NOK
constexpr unsigned long DEBOUNCE_MS    = 60UL;
constexpr unsigned long DISPLAY_MS     = 150UL;

// ── Objetos ───────────────────────────────────────────────────────────────
Adafruit_ADS1115  ads;
LiquidCrystal     lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// ── Estado global ─────────────────────────────────────────────────────────
float        referenceMM  = 0.0f;
bool         isCalibrated = false;
unsigned long lastDisplay = 0;

// ─────────────────────────────────────────────────────────────────────────
float readDistanceMM() {
    int16_t raw       = ads.readADC_SingleEnded(0);
    float   voltADS   = (raw * ADS_LSB_MV) / 1000.0f;
    float   voltSensor = voltADS * DIVIDER_RATIO;
    return constrain((voltSensor / SENSOR_MAX_V) * SENSOR_RANGE_MM, 0.0f, SENSOR_RANGE_MM);
}

bool selectPressed() {
    int v = analogRead(BTN_PIN);
    return (v >= BTN_SELECT_LOW && v <= BTN_SELECT_HIGH);
}

// ─────────────────────────────────────────────────────────────────────────
void runCalibration() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Zerando...      ");

    double sum  = 0;
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

    Wire.begin();
    ads.setGain(GAIN_TWOTHIRDS);  // ±6.144V
    if (!ads.begin()) {
        Serial.println("ADS1115 nao encontrado!");
    }

    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print(" Motor Alignment");
    lcd.setCursor(0, 1);
    lcd.print("Pressione SELECT");
    delay(1500);
    lcd.clear();
}

// ─────────────────────────────────────────────────────────────────────────
void loop() {
    // Detecta botão SELECT com debounce
    static bool lastSel = false;
    bool curSel = selectPressed();
    if (!lastSel && curSel) {
        delay(DEBOUNCE_MS);
        if (selectPressed()) {
            runCalibration();
        }
    }
    lastSel = curSel;

    // Atualiza display
    if (millis() - lastDisplay >= DISPLAY_MS) {
        lastDisplay = millis();

        float dist = readDistanceMM();
        char buf[17];

        lcd.setCursor(0, 0);
        snprintf(buf, sizeof(buf), "Dist:%5.2f mm   ", dist);
        lcd.print(buf);

        lcd.setCursor(0, 1);
        if (!isCalibrated) {
            lcd.print("Pressione SELECT");
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
