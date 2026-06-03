# Motor Cap Alignment — Sensor Indutivo com ESP32

Ferramenta de medição de deflexão da tampa do motor usando sensor indutivo analógico, ADC ADS1115 e LCD Keypad Shield 1602.

---

## Componentes

| Componente | Modelo |
|---|---|
| Microcontrolador | ESP32 NodeMCU-32 (30 pinos) |
| Sensor indutivo | METALTEX I18-8-ANV (0–10V, range 8mm) |
| ADC | ADS1115 (16-bit, I2C) |
| Display + botões | LCD Keypad Shield 1602 (paralelo 4-bit + ladder resistivo) |
| Fonte | 24V + LM2596 step-down para 5V |
| Resistores | 2× 10kΩ (divisor de tensão do sensor) |

---

## Topologia de Ligação

```
┌──────────────────────────────────────────────────────────────────┐
│                          FONTE 24V                               │
└──────┬───────────────────────────────────────┬───────────────────┘
       │                                       │
       ▼                                       ▼
┌─────────────┐                      ┌──────────────────┐
│  METALTEX   │                      │  LM2596 Step-Down│
│ I18-8-ANV   │                      │    24V → 5V      │
│ OUT (0–10V) │                      └────────┬─────────┘
└──────┬──────┘                               │ 5V
       │                          ┌───────────┼──────────────┐
       ▼                          │           │              │
    R1=10kΩ                       ▼           ▼              ▼
       │                   ┌──────────┐ ┌──────────┐ ┌──────────────┐
    [ADS1115 A0]           │  ESP32   │ │ ADS1115  │ │ LCD Keypad   │
       │                   │NodeMCU-32│ │  I2C     │ │   Shield     │
    R2=10kΩ                └──────────┘ └──────────┘ └──────────────┘
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

## Conexões ADS1115 → ESP32 (I2C)

| ADS1115 | ESP32 GPIO |
|---|---|
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| VCC | 5V |
| GND | GND |
| A0 | Saída do divisor de tensão |

---

## LCD Keypad Shield → ESP32

O shield usa LCD paralelo 4-bit e botões por divisor resistivo (pino analógico).
Como o ESP32 opera em **3.3V**, o rail dos botões deve ser alimentado com **3.3V** (não 5V) para não danificar o GPIO34.

### Conexão dos pinos do LCD

| Sinal no shield | ESP32 GPIO |
|---|---|
| RS | GPIO 19 |
| EN | GPIO 23 |
| D4 | GPIO 18 |
| D5 | GPIO 17 |
| D6 | GPIO 16 |
| D7 | GPIO 15 |
| Backlight | 3.3V (sempre ligado) |
| GND | GND |
| VCC (LCD) | 5V |

### Conexão dos botões

| Sinal no shield | ESP32 GPIO |
|---|---|
| A0 (botões) | GPIO 34 |
| VCC do ladder resistivo | **3.3V** ⚠️ |

> ⚠️ **Importante:** o pino VCC que alimenta o divisor resistivo dos botões (normalmente ligado ao 5V do Arduino) deve ser conectado ao **3.3V do ESP32**. O GPIO34 é somente leitura e suporta no máximo 3.3V.

### Como o botão SELECT é detectado

O shield possui um ladder resistivo que entrega tensões diferentes conforme o botão pressionado. Com VCC = 3.3V:

| Botão | Tensão aprox. | ADC (12-bit) |
|---|---|---|
| RIGHT | 0.0V | ~0 |
| UP | 0.47V | ~583 |
| DOWN | 0.78V | ~968 |
| LEFT | 1.10V | ~1365 |
| **SELECT** | **2.05V** | **~2549** |
| Nenhum | 3.3V | ~4095 |

O código detecta SELECT quando o ADC lê entre **1800 e 3200**.

---

## Como Compilar e Gravar (Arduino IDE)

### 1. Instalar o Arduino IDE

Baixe e instale em: https://arduino.cc/en/software

### 2. Adicionar suporte ao ESP32

1. Abra *File → Preferences*
2. No campo **Additional Boards Manager URLs**, cole:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Clique em OK
4. Vá em *Tools → Board → Boards Manager*
5. Busque `esp32`, selecione o pacote da **Espressif Systems** e clique em **Install**

### 3. Instalar as bibliotecas

Vá em *Tools → Manage Libraries* e instale:

| Biblioteca | Autor | Observação |
|---|---|---|
| `Adafruit ADS1X15` | Adafruit | ADC |
| `LiquidCrystal` | Arduino | já inclusa na IDE |

### 4. Configurar a placa

Vá em *Tools → Board → ESP32 Arduino* e selecione:
```
NodeMCU-32S
```
ou, se não aparecer:
```
ESP32 Dev Module
```

### 5. Selecionar a porta

Conecte o ESP32 via USB e vá em *Tools → Port → COMx*.

> Se a porta não aparecer, instale o driver CP210x: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers

### 6. Gravar

Abra `motor_alignment.ino` e clique em **Upload** (→).

> Se o upload travar, segure o botão **BOOT** do ESP32 enquanto o upload inicia.

---

## Como Usar

1. Ligue o equipamento — display mostra `Pressione SELECT`
2. Posicione o sensor na referência (posição zero)
3. Pressione o botão **SELECT** do shield
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
