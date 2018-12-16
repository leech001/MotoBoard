#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <MapleFreeRTOS900.h>
#include <EEPROM.h>

const int led = PC13;
const int lubePomp = PB12;			//Цифровой пин электроклапана
const int hall_pin = PB9;
const float lenghtWheel = 1.96;		//Длинна окружности колеса
const int ble_delay = 10;

uint16 Status;
uint16 Data;

int LubeCount = 0;						//Количество произведенных циклов смазки
int LubeDelay = 500;				//Время смазки в мс
int WheelRotateCount = 0;
int WheelRotateCountPrev = 0 ;
int WheelRotateLimitBase = 300;
int WheelRotateLimit;
float WheelSpeed;
int SpeedDelay = 5;

String ble_string;;
char ble_char[32];

//StaticJsonBuffer<200> jsonBuffer;
DynamicJsonBuffer jsonBuffer(32);

JsonObject& root = jsonBuffer.createObject();
JsonObject& json_WheelRotateLimit = jsonBuffer.createObject();
JsonObject& json_WheelRotateCount = jsonBuffer.createObject();
JsonObject& json_LubeDelay = jsonBuffer.createObject();
JsonObject& json_LubeCount = jsonBuffer.createObject();
JsonObject& json_Speed = jsonBuffer.createObject();


void setup() {
	Serial.begin();
	Serial1.begin(115200);

	pinMode(led, OUTPUT);
	pinMode(lubePomp, OUTPUT);
	pinMode(hall_pin, INPUT_PULLDOWN);
	attachInterrupt(digitalPinToInterrupt(hall_pin), HallInterrupt, RISING);

//	EEPROM.PageBase0 = 0x801F000;
//	EEPROM.PageBase1 = 0x801F800;
//	EEPROM.PageSize = 0x800;

	Status = EEPROM.init();
	Serial.print("EEPROM.init() : ");
	Serial.println(Status, HEX);
	Serial.println();

	EEPROM.read(0x00, &Data);
	Serial.println("Memory: " + String(Data) + " Status: " + String(Status));
	if (Data != 1) {
//		EEPROM.PageBase0 = 0x801F000;
//		EEPROM.PageBase1 = 0x801F800;
//		EEPROM.PageSize = 0x800;
//		EEPROM.format();
		EEPROM.write(0x00, 1);
		EEPROM.write(0x01, LubeCount);
		EEPROM.write(0x02, LubeDelay);
		EEPROM.write(0x03, WheelRotateLimitBase);
	}
	
	EEPROM.read(0x01, &Data);
	LubeCount = Data;

	EEPROM.read(0x02, &Data);
	LubeDelay = Data;

	EEPROM.read(0x03, &Data);
	WheelRotateLimitBase = Data;

	xTaskCreate(vDefaultTask,
		"DefaultTask",
		configMINIMAL_STACK_SIZE,
		NULL,
		tskIDLE_PRIORITY + 2,
		NULL);

	xTaskCreate(vLubeLimitCorrect,
		"LubeLimitCorrect",
		configMINIMAL_STACK_SIZE,
		NULL,
		tskIDLE_PRIORITY + 2,
		NULL);

	xTaskCreate(vBLETransmit,
		"BLETransmit",
		configMINIMAL_STACK_SIZE,
		NULL,
		tskIDLE_PRIORITY + 2,
		NULL);

	vTaskStartScheduler();
}


void loop() {
	// put your main code here, to run repeatedly:

}


void HallInterrupt() {
	WheelRotateCount++;
}


static void vDefaultTask(void *pvParameters)
{
	for (;;) {
		ble_string = Serial1.readString();
		ble_string.toCharArray(ble_char, 32);

		JsonObject& json_inc = jsonBuffer.parseObject(ble_char);
		
		int wrl_inc = json_inc["WRL"];
		if (wrl_inc > 0) {
			WheelRotateLimitBase = wrl_inc;
			EEPROM.write(0x03, WheelRotateLimitBase);
		}

		int ld_inc = json_inc["LD"];
		if (ld_inc > 0) {
			LubeDelay = ld_inc;
			EEPROM.write(0x02, LubeDelay);
		}

		if (WheelRotateCount > WheelRotateLimit) {
			WheelRotateCount = 0;
			WheelRotateCountPrev = 0;
			LubeChain();
		}
		vTaskDelay(10);
	}
}


static void vLubeLimitCorrect(void *pvParameters)
{
	for (;;) {
		WheelSpeed = ((WheelRotateCount - WheelRotateCountPrev) * lenghtWheel / SpeedDelay) * 3600 / 1000;
		WheelRotateLimit = WheelRotateLimitBase;
		if (WheelSpeed > 100) WheelRotateLimit = WheelRotateLimitBase * 0.95;
		if (WheelSpeed > 140) WheelRotateLimit = WheelRotateLimitBase * 0.9;
		if (WheelSpeed > 180) WheelRotateLimit = WheelRotateLimitBase * 0.85;
		if (WheelSpeed > 210) WheelRotateLimit = WheelRotateLimitBase * 0.8;
		if (WheelSpeed > 240) WheelRotateLimit = WheelRotateLimitBase * 0.75;
		WheelRotateCountPrev = WheelRotateCount;

		vTaskDelay(SpeedDelay*1000);
	}
}


static void vBLETransmit(void *pvParameters)
{
	for (;;) {
		root["WheelRotateLimit"] = WheelRotateLimit;
		root["WheelRotateCount"] = WheelRotateCount;
		root["LubeDelay"] = LubeDelay;
		root["LubeCount"] = LubeCount;
		root["Speed"] = WheelSpeed;

//		root.prettyPrintTo(Serial);
		root.printTo(Serial);
		Serial.println();

		json_WheelRotateLimit["WRL"] = WheelRotateLimit;
		json_WheelRotateCount["WRC"] = WheelRotateCount;
		json_LubeDelay["LD"] = LubeDelay;
		json_LubeCount["LC"] = LubeCount;
		json_Speed["SP"] = WheelSpeed;

		json_WheelRotateLimit.printTo(Serial1);
		Serial1.println();
		vTaskDelay(ble_delay);

		json_WheelRotateCount.printTo(Serial1);
		Serial1.println();
		vTaskDelay(ble_delay);

		json_LubeDelay.printTo(Serial1);
		Serial1.println();
		vTaskDelay(ble_delay);

		json_LubeCount.printTo(Serial1);
		Serial1.println();
		vTaskDelay(ble_delay);

		json_Speed.printTo(Serial1);
		Serial1.println();
		vTaskDelay(ble_delay);
	}
}


void LubeChain(void) {
	digitalWrite(led, HIGH);
	delay(LubeDelay);
	digitalWrite(led, LOW);
	LubeCount++;
	Status = EEPROM.write(0x01, LubeCount);
}
