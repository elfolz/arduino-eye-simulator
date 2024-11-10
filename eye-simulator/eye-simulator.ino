#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>

#define SW 128
#define SH 128

#define DC_PIN 16
#define CS_PIN 17
#define RST_PIN 5

#define EYE_COLOR 0x07ED

Adafruit_SSD1351 tft = Adafruit_SSD1351(SW, SH, &SPI, CS_PIN, DC_PIN, RST_PIN);
GFXcanvas16 canvas(SW, SH);

int newx = 0;
int newy = 0;
int currentx = 0;
int currenty = 0;
int randomx = 0;
int randomy = 0;
int randomReturnCenter = 0;
int randomBlink = 0;
bool blinking = false;
bool moving = false;

void setup(void) {
  Serial.begin(115200);
  tft.begin();
  drawEye(0, 0);
}

void loop() {
  delay(random(5000, 10001));
  randomBlink = random(2);
  if (randomBlink > 0 && !blinking) {
    blink();
  } else if (!moving) {
    if (currentx == 0 && currenty == 0) {
      randomMove();
    } else {
      randomReturnCenter = random(2);
      if (randomReturnCenter > 0) {
        returnCenter();
      } else {
        randomMove();
      }
    }
  }
}

void drawEye(int x, int y) {
  Serial.print("draw eye ");
  Serial.print(currentx);
  Serial.print(" ");
  Serial.print(currenty);
  Serial.println("");

  canvas.fillScreen(0x0000);
  canvas.fillCircle(SW/2, SH/2, SW/2*0.8, EYE_COLOR);
  canvas.fillCircle(SW/2, SH/2, SW/2*0.6, 0x0000);
  canvas.fillCircle(SW/2, SH/2, SW/2*0.4, 0xFFFF);
  tft.drawRGBBitmap(x, y, canvas.getBuffer(), canvas.width(), canvas.height());
  currentx = x;
  currenty = y;
}

void blink() {
  Serial.println("blink");
  blinking = true;
  for (int i=10; i>1; i--) {
    canvas.fillScreen(0x0000);
    canvas.fillCircle(SW/2, SH/2, SW/2*0.8, EYE_COLOR);
    canvas.fillCircle(SW/2, SH/2, SW/2*0.6, 0x0000);
    canvas.fillCircle(SW/2, SH/2, SW/2*0.4, 0xFFFF);
    canvas.fillRect(0, 0, canvas.width(), SH/i, 0x0000);
    canvas.fillRect(0, SH-(SH/i), canvas.width(), SH/i, 0x0000);
    tft.drawRGBBitmap(currentx, currenty, canvas.getBuffer(), canvas.width(), canvas.height());
  }
  delay(random(100, 201));
  drawEye(currentx, currenty);
  blinking = false;
}

void randomMove() {
  moving = true;
  randomx = random(2);
  randomy = random(2);

  newx = (SW-(SW*0.8))/2;
  newy = (SH-(SH*0.8))/2;
  if (randomx <= 0) {
    newx *= -1;
  }
  if (randomy <= 0) {
    newy *= -1;
  }
  
  Serial.print("random move ");
  Serial.print(newx);
  Serial.print(" ");
  Serial.print(newy);
  Serial.println("");

  if (newx == currentx && newy != currenty) {
    if (newy > currenty) {
      for (int i=currenty; i<=newy; i++) {
        drawEye(currentx, i);
      }
    } else if (newy < currenty) {
      for (int i=currenty; i>=newy; i--) {
        drawEye(currentx, i);
      }
    }
  } else if (newx != currentx && newy == currenty) {
    if (newx > currentx) {
      for (int i=currentx; i>=newx; i++) {
        drawEye(i, currenty);
      }
    } else if (newx < currentx) {
      for (int i=currentx; i<=newx; i--) {
        drawEye(i, currenty);
      }
    }
  } else if (newx != currentx && newy != currenty) {
    if  (newx > currentx && newy > currenty) {
      for (int i=currentx; i<=newx; i++) {
        for (int j=currenty; j<newy; j++) {
          drawEye(i, j);
        }
      }
    } else if  (newx < currentx && newy < currenty) {
      for (int i=currentx; i>=newx; i--) {
        for (int j=currenty; j>=newy; j--) {
          drawEye(i, j);
        }
      }
    } else if  (newx > currentx && newy < currenty) {
      for (int i=currentx; i<=newx; i++) {
        for (int j=currenty; j>=newy; j--) {
          drawEye(i, j);
        }
      }
    } else if  (newx < currentx && newy > currenty) {
      for (int i=currentx; i>=newx; i--) {
        for (int j=currenty; j<=newy; j++) {
          drawEye(i, j);
        }
      }
    }
  }
  moving = false;
}

void returnCenter() {
  moving = true;

  Serial.print("return center ");
  Serial.print(currentx);
  Serial.print(" ");
  Serial.print(currenty);
  Serial.println("");
  
  if (currentx == 0 && currenty != 0) {
    if (currentx == 0 && currenty > 0) {
      for (int i=currenty; i>=0; i--) {
        drawEye(0, i);
      }
    } else if (currentx == 0 && currenty < 0) {
      for (int i=currenty; i<=0; i++) {
        drawEye(0, i);
      }
    }
  } else if (currenty == 0 && currentx != 0) {
    if (currentx > 0 && currenty == 0) {
      for (int i=currentx; i>=0; i--) {
        drawEye(i, 0);
      }
    } else if (currentx < 0 && currenty == 0) {
      for (int i=currentx; i<=0; i++) {
        drawEye(i, 0);
      }
    }
  } else if (currentx != 0 && currenty != 0) {
    if (currentx > 0 && currenty > 0) {
      for (int i=currentx; i>=0; i--) {
        for (int j=currenty; j>=0; j--) {
          drawEye(i, j);
        }
      }
    } else if (currentx < 0 && currenty < 0) {
      for (int i=currentx; i<=0; i++) {
        for (int j=currenty; j<=0; j++) {
          drawEye(i, j);
        }
      }
    } else if (currentx > 0 && currenty < 0) {
      for (int i=currentx; i>=0; i--) {
        for (int j=currenty; j<=0; j++) {
          drawEye(i, j);
        }
      }
    } else if (currentx < 0 && currenty > 0) {
      for (int i=currentx; i<=0; i++) {
        for (int j=currenty; j>=0; j--) {
          drawEye(i, j);
        }
      }
    }
  }
  moving = false;
}