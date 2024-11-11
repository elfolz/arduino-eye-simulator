#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <SPI.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define EYE_SIZE 40
#define EYE_COLOR 0x07ED

#define DC_PIN 16
#define CS_PIN 17
#define RST_PIN 5

#define BLE_SERVER_NAME "Cyberpunk Eye"
#define BLE_SERVICE_UUID "098f6372-826e-43c8-a45f-d29382e382d4"
#define BLE_CHARACTERISTIC_UUID "9e423521-9ca6-4ed2-b0f4-079653fdc879"

Adafruit_SSD1351 oled = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, CS_PIN, DC_PIN, RST_PIN);
GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;

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
unsigned long eyeLastUpdate = millis() + random(2500, 5001);

class HandleServerCallbacks : public BLEServerCallbacks {
	void onConnect(BLEServer *pServer) {
		Serial.println("\n[BLE] Device connected...");
		bleConnected = true;
	};
	void onDisconnect(BLEServer *pServer) {
		bleConnected = false;
		delay(500);
		Serial.println("\n[BLE] Start Advertising...");
		pServer->getAdvertising()->start();
	}
};

class HandleClientCallbacks : public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic *pCharacteristic) {
		String value = pCharacteristic->getValue();
		Serial.println(value.c_str());
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

void loop() { updateEye(); }

void updateEye() {
	if (millis() > eyeLastUpdate) {
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
		eyeLastUpdate = millis() + random(2500, 5001);
	}
}

void refresh() {
	canvas.fillScreen(0x0000);
	canvas.fillCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, EYE_SIZE, EYE_COLOR);
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
		canvas.fillCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, EYE_SIZE, EYE_COLOR);
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
	pServer = BLEDevice::createServer();
	pServer->setCallbacks(new HandleServerCallbacks());
	BLEService *pService = pServer->createService(BLE_SERVICE_UUID);
	log("[BLE] Creating Characteristic...");
	pCharacteristic = pService->createCharacteristic(BLE_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
	pCharacteristic->addDescriptor(new BLE2902());
	pCharacteristic->setCallbacks(new HandleClientCallbacks());
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
		pCharacteristic->setValue(str);
		pCharacteristic->notify();
	}
}