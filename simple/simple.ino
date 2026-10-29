/* ----------------------------------------------------------------------
"Simple" Protomatter library example sketch (once you get past all
the various pin configurations at the top, and all the comments).
Shows basic use of Adafruit_Protomatter library with different devices.

This example is written for a 64x32 matrix but can be adapted to others.

Once the RGB matrix is initialized, most functions of the Adafruit_GFX
library are available for drawing -- code from other projects that use
LCDs or OLEDs can be easily adapted, or may be insightful for reference.
GFX library is documented here:
https://learn.adafruit.com/adafruit-gfx-graphics-library
------------------------------------------------------------------------- */

#include <Adafruit_Protomatter.h>
#include "shapes.h"
#include "pins.h"

enum Drawing {
  FLAG,
  HEART,
  SHIELD,
  LANTERN
};

Adafruit_Protomatter
    matrix(32,          // Width of matrix (or matrix chain) in pixels
           4,           // Bit depth, 1-6 (each step up halves the FPS)
           1, rgbPins,  // # of matrix chains, array of 6 RGB pins for each
           4, addrPins, // # of address pins (height is inferred), array of pins
           clockPin, latchPin, oePin, // Other matrix control pins
           false); // No double-buffering here (see "doublebuffer" example)

// SETUP - RUNS ONCE AT PROGRAM START --------------------------------------

ProtomatterStatus status; // Kept so loop() can keep reporting it

const int HEART_THRESHOLD = 60;
const int HEART_RUNTIME = 30;
const int LANTERN_RUNTIME = 30;
int tick; //ticks since the last reset of the loop

uint8_t brightness = 64; // 0 (off) to 255 (full), applied on the next draw

// Like matrix.color565(), but scaled by brightness
uint16_t dimColor(uint8_t r, uint8_t g, uint8_t b) {
  return matrix.color565(r * brightness / 255, g * brightness / 255,
                         b * brightness / 255);
}

// Functions rather than constants, so brightness changes are picked up
uint16_t ukraineBlue() { return dimColor(0, 87, 183); }
uint16_t ukraineYellow() { return dimColor(255, 221, 0); }

void setup(void) {
  Serial.begin(9600);

  // Initialize matrix...
  status = matrix.begin();
  Serial.print("Protomatter begin() status: ");
  Serial.println((int)status);
  if (status != PROTOMATTER_OK) {
    // DO NOT CONTINUE if matrix setup encountered an error. Keep printing
    // the status, since native USB boards (e.g. UNO R4) drop the first line.
    for (;;) {
      Serial.print("Protomatter begin() status: ");
      Serial.println((int)status);
      delay(1000);
    }
  }
  tick = 0;
}

// LOOP - RUNS REPEATEDLY AFTER SETUP --------------------------------------

void loop(void) {
  // Since there's nothing more to be drawn, this loop() function just
  // shows the approximate refresh rate of the matrix at current settings.
  // matrix.fillScreen(0); // Fill background black

  Serial.print("status ");
  Serial.print((int)status);
  Serial.print(", tick = ");
  Serial.print(tick);
  Serial.print(", Refresh FPS = ~");
  // getFrameCount() is frames since the last call, so divide by elapsed time
  static uint32_t lastMillis = millis();
  uint32_t now = millis();
  uint32_t frames = matrix.getFrameCount();
  Serial.println(frames * 1000 / max(now - lastMillis, (uint32_t)1));
  lastMillis = now;

  if (tick > HEART_THRESHOLD + HEART_RUNTIME + LANTERN_RUNTIME) {
    tick = 0;
  }

  Drawing type = FLAG;
  if (tick > HEART_THRESHOLD && tick < HEART_THRESHOLD + HEART_RUNTIME) {
      type = HEART;
  } else if (tick >= HEART_THRESHOLD + HEART_RUNTIME && tick < HEART_THRESHOLD + HEART_RUNTIME + LANTERN_RUNTIME) {
      type = LANTERN;
  }
  draw(type, tick - HEART_THRESHOLD);
  tick++;

  delay(100);
}

void draw(Drawing drawType, int stage) {
  matrix.fillScreen(0); // Reset background
  switch (drawType) {
    case HEART:
      drawHeart(stage);
      break;
    case LANTERN:
      drawJackOLantern();
      break;
    case SHIELD:
      drawShield(false);
      break;
    default:
      drawFlag();
      break;
  }
  matrix.show();
}

void drawFlag(void) {
  int padding = 3;
  int half = matrix.height() / 2;

  for (int y = padding; y < matrix.height() - padding; y++) {
    uint16_t color = y < half ? ukraineBlue() : ukraineYellow();
    for (int x = 0; x < matrix.width(); x++) {
      matrix.drawPixel(x, y, color);
    }
  }
  // drawShield(true);
}

void drawHeart(int stage) {
  // sizeof() counts bytes, so divide by the element size to get counts
  const int rows = sizeof(heart) / sizeof(heart[0]);
  const int cols = sizeof(heart[0]) / sizeof(heart[0][0]);

  // Grow from the bitmap's own size (15) to the full panel (32) as stage goes
  // 0..HEART_RUNTIME
  int size = rows + stage * (matrix.height() - rows) / HEART_RUNTIME;
  int offsetX = (matrix.width() - size) / 2; // Keep it centred
  int offsetY = (matrix.height() - size) / 2;

  // For every screen pixel in the size x size square, look up which heart
  // cell it falls in, and draw it if that cell is set
  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      int row = y * rows / size;
      int col = x * cols / size;
      if (heart[row][col] == 1) {
        matrix.drawPixel(offsetX + x, offsetY + y, dimColor(255, 0, 0));
      }
    }
  }
}

void drawShield(bool overlay) {
  if (!overlay) {matrix.fillScreen(0);} // Reset background

  const int rows = sizeof(shield) / sizeof(shield[0]);
  const int cols = sizeof(shield[0]) / sizeof(shield[0][0]);
  // The array is written row by row, so shield[row][col] goes at (col, row)
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      switch (shield[row][col]) {
      case 1:
        matrix.drawPixel(col, row, ukraineBlue());
        break;
      case 2:
        matrix.drawPixel(col, row, ukraineYellow());
        break;
      case 3:
        matrix.drawPixel(col, row, dimColor(16, 16, 16));
        break;
      default:
        continue;
      }
    }
  }
}

void drawJackOLantern(void) {
  const uint16_t palette[] = {
      0,                       // 0: off
      dimColor(255, 90, 0),    // 1: deep orange body
      dimColor(200, 40, 0),    // 2: near-red ribs
      dimColor(0, 150, 0),     // 3: green stem
      matrix.color565(255, 200, 40), // 4: glowing face, never dimmed
  };
  const int rows = sizeof(jackOLantern) / sizeof(jackOLantern[0]);
  const int cols = sizeof(jackOLantern[0]) / sizeof(jackOLantern[0][0]);
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      uint8_t v = jackOLantern[row][col];
      if (v != 0) {
        matrix.drawPixel(col, row, palette[v]);
      }
    }
  }
}
