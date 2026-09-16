// Air Aware Nature UI v4 PORTRAIT | ESP32-C3 SuperMini + ST7789 240x320 + PMS5003
// Libraries: Adafruit GFX Library; Adafruit ST7735 and ST7789 Library.
// Board: ESP32C3 Dev Module. USB CDC On Boot: Enabled.
// Working sensor connection: PMS TX -> GPIO21 (ESP receive).
// GPIO20 stays INPUT; this sketch never transmits to the sensor.
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <math.h>
#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"

constexpr int LCD_CS = 10, LCD_DC = 4, LCD_MOSI = 7;
constexpr int LCD_CLK = 6, LCD_RST = 5, LCD_BL = 1, PMS_RX = 21;
constexpr uint32_t WARMUP_MS = 30000UL, STALE_MS = 5000UL;
// Example CLASSROOM comparison bands, NOT health limits or AQI categories.
// Count is particles >0.3 micrometers per 0.1 liter, reported by the sensor.
constexpr uint16_t COUNT_YELLOW_AT = 300, COUNT_RED_AT = 1000;

Adafruit_ST7789 screen(&SPI, LCD_CS, LCD_DC, LCD_RST);
constexpr int SCREEN_W = 240, SCREEN_H = 320;
constexpr uint8_t DISPLAY_ROTATION = 0;
GFXcanvas16 canvas(SCREEN_W, SCREEN_H);
constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 248) << 8) | ((g & 252) << 3) | (b >> 3);
}
constexpr uint16_t BG = rgb(15, 40, 35), PANEL = rgb(29, 61, 51);
constexpr uint16_t PAPER = rgb(244, 244, 226), INK = rgb(25, 56, 45);
constexpr uint16_t MUTED = rgb(97, 119, 98), SAGE = rgb(167, 196, 162);
constexpr uint16_t MINT = rgb(179, 224, 160), GREEN = rgb(121, 204, 149);
constexpr uint16_t YELLOW = rgb(242, 202, 107), RED = rgb(235, 143, 124);
constexpr uint16_t GRAY = rgb(183, 192, 178);
constexpr float PI_F = 3.14159265f;

uint16_t pm25 = 0, particleCount03 = 0;
uint32_t lastReading = 0, wakeTime = 0, lastDraw = 0;
uint32_t rxBytes = 0, validFrames = 0;
bool haveReading = false;
uint8_t frame[32], used = 0;

uint16_t wordAt(int index) {
  return (uint16_t(frame[index]) << 8) | frame[index + 1];
}
void acceptByte(uint8_t value) {
  ++rxBytes;
  frame[used++] = value;
  if (used < sizeof(frame)) return;
  uint16_t sum = 0;
  for (int i = 0; i < 30; ++i) sum += frame[i];
  if (frame[0] == 0x42 && frame[1] == 0x4D && wordAt(2) == 28 &&
      sum == wordAt(30)) {
    pm25 = wordAt(12);             // Atmospheric PM2.5, ug/m3 (not CF=1)
    particleCount03 = wordAt(16);  // >0.3 um particles per 0.1 L
    lastReading = millis();
    haveReading = true;
    ++validFrames;
    used = 0;
  } else {
    memmove(frame, frame + 1, sizeof(frame) - 1);
    used = sizeof(frame) - 1;
  }
}
int countBand(uint16_t count) {
  return count >= COUNT_RED_AT ? 2 : (count >= COUNT_YELLOW_AT ? 1 : 0);
}
bool readingFresh(uint32_t now) {
  return haveReading && uint32_t(now - lastReading) < STALE_MS;
}
void textAt(const char *s, int x, int baseline, const GFXfont *font,
            uint16_t color) {
  canvas.setFont(font);
  canvas.setTextSize(1);
  canvas.setTextColor(color);
  canvas.setCursor(x, baseline);
  canvas.print(s);
}
void centered(const char *s, int x, int baseline, const GFXfont *font,
              uint16_t color) {
  canvas.setFont(font);
  canvas.setTextSize(1);
  int16_t bx, by;
  uint16_t w, h;
  canvas.getTextBounds(s, 0, baseline, &bx, &by, &w, &h);
  textAt(s, x - int(w) / 2 - bx, baseline, font, color);
}
void tiny(const char *s, int x, int y, uint16_t color) {
  textAt(s, x, y, nullptr, color);
}
void tinyCentered(const char *s, int x, int y, uint16_t color) {
  tiny(s, x - int(strlen(s)) * 3, y, color);
}

void leaf(int x, int y, float angle, float length, float width, uint16_t color) {
  const float dx = cosf(angle), dy = sinf(angle);
  int px = x, py = y, qx = x, qy = y;
  for (int i = 1; i <= 12; ++i) {
    const float t = i / 12.0f, spread = sinf(t * PI_F) * width;
    int ax = lroundf(x + dx * length * t - dy * spread);
    int ay = lroundf(y + dy * length * t + dx * spread);
    int bx = lroundf(x + dx * length * t + dy * spread);
    int by = lroundf(y + dy * length * t - dx * spread);
    canvas.fillTriangle(px, py, qx, qy, ax, ay, color);
    canvas.fillTriangle(qx, qy, ax, ay, bx, by, color);
    px = ax; py = ay; qx = bx; qy = by;
  }
}
void header(bool live) {
  canvas.fillScreen(BG);
  leaf(24, 32, -1.25f, 22, 6, MINT);
  leaf(25, 31, -2.6f, 13, 4, SAGE);
  canvas.drawLine(23, 35, 29, 19, BG);
  textAt("air aware", 44, 30, &FreeSansBold9pt7b, PAPER);
  if (live) {
    canvas.fillCircle(186, 23, 3, MINT);
    tiny("LIVE", 198, 20, SAGE);
  }
}
void drawFace(int band) {
  const int cx = 120, cy = 203;
  const uint16_t colors[] = {GREEN, YELLOW, RED, GRAY};
  canvas.fillCircle(cx, cy, 28, colors[band]);
  canvas.fillCircle(cx - 10, cy - 5, 2, INK);
  canvas.fillCircle(cx + 10, cy - 5, 2, INK);
  // Upturned corners = smile; straight = neutral; downturned = sad.
  for (int x = -10; x <= 10; ++x) {
    float bend = 5.0f * (1.0f - x * x / 100.0f);
    int y = cy + 10 + (band == 0 ? lroundf(bend) :
                       (band == 2 ? -lroundf(bend) : 0));
    canvas.fillCircle(cx + x, y, 1, INK);
  }
}
void drawWarmup(uint32_t elapsed) {
  header(false);
  // Subtle foliage behind the countdown.
  leaf(0, 242, -0.9f, 85, 20, PANEL);
  leaf(240, 198, 2.2f, 76, 19, PANEL);
  const int cx = 120, cy = 137, radius = 55;
  for (int d = 0; d < 360; d += 3) {
    float a = (d - 90) * PI_F / 180.0f;
    canvas.fillCircle(cx + lroundf(radius * cosf(a)),
                      cy + lroundf(radius * sinf(a)), 3, PANEL);
  }
  const int degrees = int((elapsed * 360UL) / WARMUP_MS);
  for (int d = 0; d <= degrees; d += 3) {
    float a = (d - 90) * PI_F / 180.0f;
    canvas.fillCircle(cx + lroundf(radius * cosf(a)),
                      cy + lroundf(radius * sinf(a)), 3, MINT);
  }
  const float tip = (degrees - 90) * PI_F / 180.0f;
  canvas.fillCircle(cx + lroundf(radius * cosf(tip)),
                    cy + lroundf(radius * sinf(tip)), 5, PAPER);
  char seconds[8];
  snprintf(seconds, sizeof(seconds), "%lu", (unsigned long)
           ((WARMUP_MS - elapsed + 999UL) / 1000UL));
  centered(seconds, cx, 148, &FreeSansBold24pt7b, PAPER);
  tinyCentered("SECONDS", cx, 162, SAGE);
  centered("A little time to settle", 120, 240, &FreeSansBold9pt7b, PAPER);
  tinyCentered("WAKING UP THE PARTICLE SENSOR", 120, 261, SAGE);
  canvas.fillRoundRect(93, 297, 54, 3, 1, PANEL);
  canvas.fillRoundRect(93, 297, 1 + int(elapsed * 53UL / WARMUP_MS), 3, 1, MINT);
}
void drawLive(uint32_t now) {
  const bool fresh = readingFresh(now);
  header(fresh);
  canvas.fillRoundRect(12, 48, 216, 212, 18, PAPER);
  tinyCentered("PARTICLE COUNT", 120, 66, MUTED);
  char value[8];
  if (fresh) snprintf(value, sizeof(value), "%u", unsigned(particleCount03));
  else strcpy(value, "--");
  centered(value, 120, 122, &FreeSansBold24pt7b, INK);
  centered("per 0.1 L", 120, 145, &FreeSans9pt7b, MUTED);
  tinyCentered("PARTICLES >0.3 um", 120, 158, MUTED);
  int band = fresh ? countBand(particleCount03) : 3;
  drawFace(band);
  const char *labels[] = {"LOW COUNT", "MID COUNT", "HIGH COUNT", "WAITING"};
  tinyCentered(labels[band], 120, 243, MUTED);
  canvas.fillRoundRect(12, 269, 216, 33, 11, PANEL);
  textAt("PM2.5", 24, 291, &FreeSans9pt7b, PAPER);
  if (fresh) snprintf(value, sizeof(value), "%u", unsigned(pm25));
  else strcpy(value, "--");
  canvas.setFont(&FreeSansBold9pt7b);
  int16_t bx, by; uint16_t w, h;
  canvas.getTextBounds(value, 0, 291, &bx, &by, &w, &h);
  textAt(value, 176 - int(w) - bx, 291, &FreeSansBold9pt7b, PAPER);
  tiny("ug/m3", 186, 284, SAGE);
  tinyCentered(fresh ? "CLASSROOM COUNT SCALE / NOT AQI" :
               (haveReading ? "SENSOR DATA LOST" : "WAITING FOR SENSOR DATA"),
               120, 311, SAGE);
}
void drawUI() {
  const uint32_t now = millis(), elapsed = now - wakeTime;
  if (elapsed < WARMUP_MS) drawWarmup(elapsed);
  else drawLive(now);
  screen.drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_W, SCREEN_H);
}
void fatalError(const char *message) {
  screen.fillScreen(BG);
  screen.setFont(nullptr);
  screen.setTextSize(1);
  screen.setTextColor(PAPER);
  screen.setCursor(12, 30);
  screen.print(message);
  while (true) delay(1000);
}
void requireUart(esp_err_t result) {
  if (result != ESP_OK) fatalError(esp_err_to_name(result));
}
void startReceiver() {
  requireUart(gpio_reset_pin(GPIO_NUM_20));
  requireUart(gpio_reset_pin(GPIO_NUM_21));
  requireUart(gpio_set_direction(GPIO_NUM_20, GPIO_MODE_INPUT));
  requireUart(gpio_set_direction(GPIO_NUM_21, GPIO_MODE_INPUT));
  requireUart(gpio_set_pull_mode(GPIO_NUM_20, GPIO_PULLUP_ONLY));
  requireUart(gpio_set_pull_mode(GPIO_NUM_21, GPIO_PULLUP_ONLY));
  uart_config_t config = {};
  config.baud_rate = 9600;
  config.data_bits = UART_DATA_8_BITS;
  config.parity = UART_PARITY_DISABLE;
  config.stop_bits = UART_STOP_BITS_1;
  config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  config.source_clk = UART_SCLK_DEFAULT;
  requireUart(uart_param_config(UART_NUM_1, &config));
  requireUart(uart_driver_install(UART_NUM_1, 4096, 0, 0, nullptr, 0));
  requireUart(uart_set_pin(UART_NUM_1, UART_PIN_NO_CHANGE, PMS_RX,
                         UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  requireUart(uart_flush_input(UART_NUM_1));
}
void setup() {
  gpio_reset_pin(GPIO_NUM_20);
  gpio_reset_pin(GPIO_NUM_21);
  gpio_set_direction(GPIO_NUM_20, GPIO_MODE_INPUT);
  gpio_set_direction(GPIO_NUM_21, GPIO_MODE_INPUT);
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);
  delay(100);
  SPI.begin(LCD_CLK, -1, LCD_MOSI, LCD_CS);
  screen.init(240, 320, SPI_MODE0);
  screen.setRotation(DISPLAY_ROTATION);
  screen.setSPISpeed(10000000);
  screen.setTextWrap(false);
  canvas.setTextWrap(false);
  if (!canvas.getBuffer()) fatalError("Display buffer allocation failed");
  startReceiver();
  wakeTime = millis();
  drawUI();
  lastDraw = millis();
}
void loop() {
  // Continue receiving during warmup and animations; no 30-second delay().
  uint8_t data[256];
  const int count = uart_read_bytes(UART_NUM_1, data, sizeof(data), 0);
  for (int i = 0; i < count; ++i) acceptByte(data[i]);
  const uint32_t now = millis();
  const uint32_t interval = now - wakeTime < WARMUP_MS ? 200UL : 1000UL;
  if (now - lastDraw >= interval) {
    drawUI();
    lastDraw = now;
  }
  delay(1);
}
