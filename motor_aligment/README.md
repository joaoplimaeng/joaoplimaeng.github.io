# Motor Cap Alignment — Sensor Indutivo com ESP32

Ferramenta de medição de deflexão da tampa do motor usando sensor indutivo analógico, ADC ADS1115, LCD I2C 1602 e botão discreto.

---

## Componentes

| Componente | Modelo |
|---|---|
| Microcontrolador | ESP32 NodeMCU-32 (30 pinos) |
| Sensor indutivo | METALTEX I18-8-ANV (0–10V, range 8mm) |
| ADC | ADS1115 (16-bit, I2C) |
| Display | LCD 1602 I2C (módulo PCF8574, endereço 0x27) |
| Botão | Botão tátil discreto (NO — normalmente aberto) |
| Fonte | 24V + LM2596 step-down para 5V |
| Resistores | 2× 10kΩ (divisor de tensão do sensor) |

---

## Topologia de Ligação

```
┌──────────────────────────────────────────────────────────────────────┐
│                            FONTE 24V                                 │
└──────┬───────────────────────────────────────┬─────────────────────-─┘
       │                                       │
       ▼                                       ▼
┌─────────────┐                      ┌──────────────────┐
│  METALTEX   │                      │  LM2596 Step-Down│
│ I18-8-ANV   │                      │    24V → 5V      │
│ OUT (0–10V) │                      └────────┬─────────┘
└──────┬──────┘                               │ 5V
       │                     ┌────────────────┼──────────────┬──────────────┐
       ▼                     │                │              │              │
    R1=10kΩ                  ▼                ▼              ▼              ▼
       │              ┌──────────┐     ┌──────────┐  ┌────────────┐  ┌──────────┐
    [ADS1115 A0]      │  ESP32   │     │ ADS1115  │  │ LCD 1602   │  │  Botão   │
       │              │NodeMCU-32│     │  (I2C)   │  │   (I2C)    │  │Discreto  │
    R2=10kΩ           └──────────┘     └──────────┘  └────────────┘  └──────────┘
       │
      GND
```

---

## Divisor de Tensão do Sensor (obrigatório)

O sensor entrega até **10V**, o ADS1115 suporta no máximo ~6.1V. O divisor 1:2 reduz para 0–5V:

```
Sensor OUT ──── R1 (10kΩ) ──┬──── ADS1115 A0
                             │
                         R2 (10kΩ)
                             │
                            GND
```

| Tensão do sensor | Tensão no ADS1115 | Distância |
|---|---|---|
| 0V | 0V | 0.00 mm |
| 5V | 2.5V | 4.00 mm |
| 10V | 5.0V | 8.00 mm |

---

## Barramento I2C → ESP32

O LCD e o ADS1115 **compartilham o mesmo barramento I2C**:

| Sinal | ESP32 GPIO |
|---|---|
| SDA | GPIO 21 |
| SCL | GPIO 22 |

| Dispositivo | Endereço I2C |
|---|---|
| ADS1115 | 0x48 (padrão) |
| LCD PCF8574 | 0x27 (típico) — tente 0x3F se não funcionar |

---

## Conexões ADS1115

| ADS1115 | ESP32 |
|---|---|
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| VCC | 5V |
| GND | GND |
| A0 | Saída do divisor de tensão |

---

## Conexões LCD 1602 I2C

| LCD I2C (PCF8574) | ESP32 |
|---|---|
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| VCC | 5V |
| GND | GND |

> O backlight é controlado pelo próprio módulo PCF8574 via `lcd.backlight()` no código.

---

## Conexão do Botão Discreto

O botão é do tipo **NO (normalmente aberto)**. Um terminal vai ao GPIO e o outro ao GND. O pull-up interno do ESP32 mantém o pino em HIGH; pressionar o botão força LOW.

```
GPIO 32 ──── [ BTN ] ──── GND
              (NO)
```

| Sinal | ESP32 GPIO |
|---|---|
| Botão | GPIO 32 |
| GND | GND |

> Não é necessário resistor externo — o código usa `INPUT_PULLUP`.

---

## Como Compilar e Gravar

### Opção A — Arduino IDE (GUI)

#### 1. Instalar o Arduino IDE

```bash
# Baixe o AppImage em https://arduino.cc/en/software e torne executável:
chmod +x arduino-ide_*.AppImage
./arduino-ide_*.AppImage
```

#### 2. Permissão de porta serial (Linux)

```bash
sudo usermod -aG dialout $USER
# Faça logout e login novamente para aplicar
```

#### 3. Adicionar suporte ao ESP32

1. Abra *File → Preferences*
2. No campo **Additional Boards Manager URLs**, cole:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Clique em OK
4. Vá em *Tools → Board → Boards Manager*
5. Busque `esp32`, selecione o pacote da **Espressif Systems** e clique em **Install**

#### 4. Instalar as bibliotecas

Vá em *Tools → Manage Libraries* e instale:

| Biblioteca | Autor |
|---|---|
| `Adafruit ADS1X15` | Adafruit |
| `LiquidCrystal_I2C` | Frank de Brabander |

#### 5. Configurar e gravar

1. *Tools → Board → ESP32 Arduino* → `NodeMCU-32S` (ou `ESP32 Dev Module`)
2. *Tools → Port* → `/dev/ttyUSB0` ou `/dev/ttyACM0`
3. Clique em **Upload** (→)

> Se o upload travar, segure o botão **BOOT** do ESP32 enquanto o upload inicia.

---

### Opção B — arduino-cli (linha de comando)

Mais rápido para quem prefere terminal.

#### 1. Instalar o arduino-cli

```bash
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
sudo mv bin/arduino-cli /usr/local/bin/
```

#### 2. Permissão de porta serial

```bash
sudo usermod -aG dialout $USER
# Faça logout e login novamente para aplicar
```

#### 3. Configurar o arduino-cli

```bash
arduino-cli config init
arduino-cli config add board_manager.additional_urls \
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32
```

#### 4. Instalar as bibliotecas

```bash
arduino-cli lib install "Adafruit ADS1X15"
arduino-cli lib install "LiquidCrystal I2C"
```

#### 5. Compilar

```bash
arduino-cli compile \
  --fqbn esp32:esp32:nodemcu-32s \
  motor_alignment.ino
```

#### 6. Identificar a porta

```bash
arduino-cli board list
# Exemplo de saída:
# /dev/ttyUSB0   Serial Port (USB)   CP2102 ...
```

#### 7. Gravar

```bash
arduino-cli upload \
  --fqbn esp32:esp32:nodemcu-32s \
  --port /dev/ttyUSB0 \
  motor_alignment.ino
```

> Se preferir compilar e gravar em um único comando:
> ```bash
> arduino-cli compile --fqbn esp32:esp32:nodemcu-32s motor_alignment.ino && \
> arduino-cli upload  --fqbn esp32:esp32:nodemcu-32s --port /dev/ttyUSB0 motor_alignment.ino
> ```

#### 8. Monitor serial (opcional)

```bash
arduino-cli monitor --port /dev/ttyUSB0 --config baudrate=115200
```

---

## Solução de Problemas

| Sintoma | Causa provável | Solução |
|---|---|---|
| LCD não acende | Endereço I2C errado | Troque `0x27` por `0x3F` no define `LCD_I2C_ADDR` |
| LCD acende mas sem texto | Contraste do potenciômetro do módulo PCF8574 | Ajuste o trimpot na parte traseira do módulo |
| Botão não responde | Conexão GND ou GPIO errada | Verifique se um terminal vai ao GPIO 32 e o outro ao GND |
| ADS1115 não encontrado | Endereço ou fiação I2C | Confirme SDA→GPIO21, SCL→GPIO22 e alimentação 5V |

---

## Como Usar

1. Ligue o equipamento — display mostra `Pressione BTN`
2. Posicione o sensor na referência (posição zero)
3. Pressione o **botão discreto**
4. O sistema coleta amostras por **10 segundos** com countdown e calcula a média como referência
5. Display passa a mostrar a distância e o status:

```
Dist: X.XX mm
d=+0.12mm  OK        ← variação ≤ 1mm
```
```
Dist: X.XX mm
d=-1.47mm NOK        ← variação > 1mm
```
