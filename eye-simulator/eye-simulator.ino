#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <BLE2901.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <SPI.h>
#include <Regexp.h>
#include <stdlib.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define EYE_SIZE 40

#define CS_PIN 17
#define DC_PIN 16
#define RST_PIN 5

#define BLE_SERVER_NAME "Cyberpunk Eye"
#define BLE_SERVICE_UUID "0888fddd-b50d-4710-aa9b-17051f1b4948"
#define BLE_CHARACTERISTIC_UUID_READ "b2d07323-8815-4a88-9bae-27c835524aee"
#define BLE_CHARACTERISTIC_UUID_WRITE "d359723f-1a25-477b-9070-e7483e231aef"

Adafruit_SSD1351 oled = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, CS_PIN, DC_PIN, RST_PIN);

GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);

BLECharacteristic *pCharacteristicRead = NULL;

uint16_t eyeColor = 0x07EA;
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
bool bleConnected = false;
bool pendingRefresh = false;
unsigned long eyeNextUpdate = millis() + random(2500, 5001);
String incomingMessage;
MatchState ms;

void readData(String str) {
  char buf[128];
  str.toCharArray(buf, 128);
  ms.Target(buf);
  char receiveColor = ms.Match("eyeColor0[xX][0-9a-fA-F]+", 0);
  if (receiveColor == REGEXP_MATCHED) {
    String color = str;
    color.replace("eyeColor", "");
    color.replace("\n", "");
    eyeColor = strtol(color.c_str(), NULL, 0);
    pendingRefresh = true;
  }
};

class HandleServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    Serial.println("[BLE] Device connected...");
    bleConnected = true;
  };
  void onDisconnect(BLEServer *pServer) {
    bleConnected = false;
    delay(500);
    Serial.println("[BLE] Start Advertising...");
    pServer->getAdvertising()->start();
  }
};

class HandleClientCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String value = pCharacteristic->getValue();
    readData(value);
  }
};

void setup(void) {
  Serial.begin(115200);
  oled.begin();
  refresh();
  delay(500);
  connectBLE();
  log("Started");
}

void loop() {
  updateReceiveData();
  updateEye();
  if (pendingRefresh) {
    refresh();
  }
}

void updateReceiveData() {
  if (Serial.available() > 0) {
    char received = Serial.read();
    incomingMessage += received;
    if (received == '\n') {
      readData(incomingMessage);
      incomingMessage = "";
    }
  }
}

void updateEye() {
  if (millis() > eyeNextUpdate) {
    randomBlink = random(2);
    randomReturnCenter = random(2);
    if (randomReturnCenter > 0 && !moving && currentx != 0 && currenty != 0) {
      returnCenter();
    } else {
      if (!moving && randomBlink > 0 && !blinking) {
        blink();
      } else if (!moving) {
        randomMove();
      }
    }
    eyeNextUpdate = millis() + random(2500, 5001);
  }
}

void refresh() {
  pendingRefresh = false;
  canvas.fillScreen(0x0000);
  canvas.fillCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, EYE_SIZE, eyeColor);
  canvas.fillCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, EYE_SIZE * 0.8, 0x0000);
  canvas.fillCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, EYE_SIZE * 0.6, 0xFFFF);
  oled.drawRGBBitmap(currentx, currenty, canvas.getBuffer(), canvas.width(), canvas.height());
  String str = "draw eye ";
  str = str + currentx + " " + currenty;
  log(str.c_str());
}

void blink() {
  log("blink");
  blinking = true;
  for (int i = 10; i > 1; i--) {
    canvas.fillScreen(0x0000);
    canvas.fillCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, EYE_SIZE, eyeColor);
    canvas.fillCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, EYE_SIZE * 0.8, 0x0000);
    canvas.fillCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, EYE_SIZE * 0.6, 0xFFFF);
    canvas.fillRect(0, 0, canvas.width(), SCREEN_HEIGHT / i, 0x0000);
    canvas.fillRect(0, SCREEN_HEIGHT - (SCREEN_HEIGHT / i), canvas.width(), SCREEN_HEIGHT / i, 0x0000);
    oled.drawRGBBitmap(currentx, currenty, canvas.getBuffer(), canvas.width(), canvas.height());
  }
  delay(random(100, 201));
  refresh();
  blinking = false;
}

void randomMove() {
  moving = true;
  randomx = random(2);
  randomy = random(2);
  newx = (SCREEN_WIDTH / 2) - EYE_SIZE - 1;
  newy = (SCREEN_HEIGHT / 2) - EYE_SIZE - 1;
  if (randomx <= 0) {
    newx *= -1;
  }
  if (randomy <= 0) {
    newy *= -1;
  }
  String str = "random move ";
  str = str + newx + " " + newy;
  log(str.c_str());
  if (newx == currentx && newy != currenty) {
    if (newy > currenty) {
      while (newy > currenty) {
        currenty++;
        refresh();
      }
    } else if (newy < currenty) {
      while (newy < currenty) {
        currenty--;
        refresh();
      }
    }
  } else if (newx != currentx && newy == currenty) {
    if (newx > currentx) {
      while (newx > currentx) {
        currentx++;
        refresh();
      }
    } else if (newx < currentx) {
      while (newx < currentx) {
        currentx--;
        refresh();
      }
    }
  } else if (newx != currentx && newy != currenty) {
    if (newx > currentx && newy > currenty) {
      while (currentx < newx && currenty < newy) {
        currentx++;
        currenty++;
        refresh();
      }
    } else if (newx < currentx && newy < currenty) {
      while (currentx > newx && currenty > newy) {
        currentx--;
        currenty--;
        refresh();
      }
    } else if (newx > currentx && newy < currenty) {
      while (currentx < newx && currenty > newy) {
        currentx++;
        currenty--;
        refresh();
      }
    } else if (newx < currentx && newy > currenty) {
      while (currentx > newx && currenty < newy) {
        currentx--;
        currenty++;
        refresh();
      }
    }
  }
  moving = false;
}

void returnCenter() {
  moving = true;
  String str = "return center ";
  str = str + currentx + " " + currenty;
  if (currentx == 0 && currenty != 0) {
    if (currenty > 0) {
      while (currenty > 0) {
        currenty--;
        refresh();
      }
    } else if (currenty < 0) {
      while (currenty < 0) {
        currenty++;
        refresh();
      }
    }
  } else if (currenty == 0 && currentx != 0) {
    if (currentx > 0) {
      while (currentx > 0) {
        currentx--;
        refresh();
      }
    } else if (currentx < 0) {
      while (currentx < 0) {
        currentx++;
        refresh();
      }
    }
  } else if (currentx != 0 && currenty != 0) {
    if (currentx > 0 && currenty > 0) {
      while (currentx > 0 && currenty > 0) {
        currentx--;
        currenty--;
        refresh();
      }
    } else if (currentx < 0 && currenty < 0) {
      while (currentx > 0 && currenty > 0) {
        currentx++;
        currenty++;
        refresh();
      }
    } else if (currentx > 0 && currenty < 0) {
      while (currentx > 0 && currenty < 0) {
        currentx--;
        currenty++;
        refresh();
      }
    } else if (currentx < 0 && currenty > 0) {
      while (currentx < 0 && currenty > 0) {
        currentx++;
        currenty--;
        refresh();
      }
    }
  }
  moving = false;
}

void connectBLE() {
  log("[BLE] Server Starting...");
  BLEDevice::init(BLE_SERVER_NAME);
  log("[BLE] Creating Server...");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new HandleServerCallbacks());
  BLEService *pService = pServer->createService(BLE_SERVICE_UUID);
  log("[BLE] Creating Characteristic...");
  pCharacteristicRead = pService->createCharacteristic(BLE_CHARACTERISTIC_UUID_READ, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristicRead->addDescriptor(new BLE2902());
  BLECharacteristic *pCharacteristicWrite = pService->createCharacteristic(BLE_CHARACTERISTIC_UUID_WRITE, BLECharacteristic::PROPERTY_WRITE);
  pCharacteristicWrite->addDescriptor(new BLE2901());
  pCharacteristicWrite->setCallbacks(new HandleClientCallbacks());
  log("[BLE] Service Starting...");
  pService->start();
  log("[BLE] Creating Advertising...");
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
  BLEDevice::startAdvertising();
  log("[BLE] Waiting for connection...");
}

void log(const char *str) {
  Serial.println(str);
  if (bleConnected) {
    pCharacteristicRead->setValue(str);
    pCharacteristicRead->notify();
  }
}