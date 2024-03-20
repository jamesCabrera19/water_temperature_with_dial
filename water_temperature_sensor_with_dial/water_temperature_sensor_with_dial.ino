#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define NEEDLE_LENGTH 25  // Visible length
#define NEEDLE_WIDTH   5  // Width of needle - make it an odd number
#define NEEDLE_RADIUS 60  // Radius at tip
#define NEEDLE_COLOR1 TFT_MAROON  // Needle periphery colour
#define NEEDLE_COLOR2 TFT_RED     // Needle centre colour
#define DIAL_CENTRE_X 120
#define DIAL_CENTRE_Y 120
#define DEGREES "F"

#include "NotoSansBold36.h"
#define AA_FONT_LARGE NotoSansBold36

// Define the temperature range in Fahrenheit
const float MIN_TEMP_F = 32.0;  // Minimum temperature in Fahrenheit
const float MAX_TEMP_F = 212.0;  // Maximum temperature in Fahrenheit

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite needle = TFT_eSprite(&tft);
TFT_eSprite spr    = TFT_eSprite(&tft);

#include "dial.h"

uint16_t* tft_buffer;
bool      buffer_loaded = false;
uint16_t  spr_width = 0;
uint16_t  bg_color = 0;

const int oneWireBus = 21;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

void setup() {
  Serial.begin(115200);

  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  TJpgDec.drawJpg(0, 0, dial, sizeof(dial));
  tft.drawCircle(DIAL_CENTRE_X, DIAL_CENTRE_Y, NEEDLE_RADIUS-NEEDLE_LENGTH, TFT_BLACK);

  spr.loadFont(AA_FONT_LARGE);
  spr_width = spr.textWidth("777");
  spr.createSprite(spr_width, spr.fontHeight());
  bg_color = tft.readPixel(120, 120);
  spr.fillSprite(bg_color);
  spr.setTextColor(TFT_WHITE, bg_color, true);
  spr.setTextDatum(MC_DATUM);
  spr.setTextPadding(spr_width);
  spr.drawNumber(0, spr_width / 2, spr.fontHeight() / 2);
  spr.pushSprite(DIAL_CENTRE_X - spr_width / 2, DIAL_CENTRE_Y - spr.fontHeight() / 2);

  tft.setTextColor(TFT_WHITE, bg_color);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1); // You can adjust the size as needed
  tft.drawString(DEGREES, DIAL_CENTRE_X, DIAL_CENTRE_Y + 48, 2);

  tft.setPivot(DIAL_CENTRE_X, DIAL_CENTRE_Y);

  createNeedle();
  plotNeedle(0, 0);

  sensors.begin(); // Initialize Dallas Temperature sensor
  delay(2000);
}

void loop() {
  float temperature_F = readTemperature_F();
  uint16_t angle = map(temperature_F, MIN_TEMP_F, MAX_TEMP_F, 0, 240);
  plotNeedle(angle, 30);
  delay(2500);
}

float readTemperature_F() {
  sensors.requestTemperatures();
  float temperature_C = sensors.getTempCByIndex(0);
  return (temperature_C * 9.0 / 5.0) + 32.0;  // Convert Celsius to Fahrenheit
}

void createNeedle() {
  needle.setColorDepth(16);
  needle.createSprite(NEEDLE_WIDTH, NEEDLE_LENGTH);

  needle.fillSprite(TFT_BLACK);
  uint16_t piv_x = NEEDLE_WIDTH / 2;
  uint16_t piv_y = NEEDLE_RADIUS;
  needle.setPivot(piv_x, piv_y);
  needle.fillRect(0, 0, NEEDLE_WIDTH, NEEDLE_LENGTH, TFT_MAROON);
  needle.fillRect(1, 1, NEEDLE_WIDTH - 2, NEEDLE_LENGTH - 2, TFT_RED);

  int16_t min_x;
  int16_t min_y;
  int16_t max_x;
  int16_t max_y;
  needle.getRotatedBounds(45, &min_x, &min_y, &max_x, &max_y);
  tft_buffer = (uint16_t*)malloc(((max_x - min_x) + 2) * ((max_y - min_y) + 2) * 2);
  
}

void plotNeedle(int16_t angle, uint16_t ms_delay) {
    static int16_t old_angle = -120;

    static int16_t min_x;
    static int16_t min_y;
    static int16_t max_x;
    static int16_t max_y;

    // Here the angle is limited to 0 and 240
    if (angle < 0) angle = 0;
    if (angle > 240) angle = 240;

    angle -= 120;

    while (angle != old_angle || !buffer_loaded) {
    if (old_angle < angle) old_angle++;
    else old_angle--;

    if ((old_angle & 1) == 0) {
      if (buffer_loaded) {
        tft.pushRect(min_x, min_y, 1 + max_x - min_x, 1 + max_y - min_y, tft_buffer);
      }

      if (needle.getRotatedBounds(old_angle, &min_x, &min_y, &max_x, &max_y)) {
        tft.readRect(min_x, min_y, 1 + max_x - min_x, 1 + max_y - min_y, tft_buffer);
        buffer_loaded = true;
      }

      needle.pushRotated(old_angle, TFT_BLACK);
      delay(ms_delay);
    }

    spr.setTextColor(TFT_WHITE, bg_color, true);
    spr.drawNumber(old_angle + 120, spr_width / 2, spr.fontHeight() / 2);
    spr.pushSprite(120 - spr_width / 2, 120 - spr.fontHeight() / 2);

    if (abs(old_angle - angle) < 10) ms_delay += ms_delay / 5;
  }
}
