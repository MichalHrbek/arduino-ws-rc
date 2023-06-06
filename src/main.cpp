#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <secrets.h>

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
Adafruit_MPU6050 mpu;

#define SLEEP_PIN 5
#define ERROR_PIN 5
#define IN1_PIN 1
#define IN2_PIN 2
#define IN3_PIN 3
#define IN4_PIN 4

bool checkDriverError() {
	return !digitalRead(ERROR_PIN);
}

void motorWrite(byte pin1, byte pin2, int speed) {
	if (speed > 0) {
		analogWrite(pin1, speed);
		analogWrite(pin2, LOW);
	}
	else if (speed < 0) {
		analogWrite(pin1, LOW);
		analogWrite(pin2, abs(speed));
	}
	else {
		analogWrite(pin1, LOW);
		analogWrite(pin2, LOW);
	}
}

void page404(AsyncWebServerRequest *request) {
	String message = R"raw(
		404 not found
	)raw";
	request->send(404, "text/plain", message);
}

String processor(const String &var) {
	if (var == "TEMP") return String(temperatureRead());
	return String();
}

void getRoot(AsyncWebServerRequest *request) {
	request->send(SPIFFS, "/index.html", "text/html", false, processor);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, uint32_t id) {
	AwsFrameInfo *info = (AwsFrameInfo*)arg;
	if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
		const uint8_t size = JSON_OBJECT_SIZE(3);
		StaticJsonDocument<size> json;
		DeserializationError err = deserializeJson(json, data);
		
		if (err) {
			ws.text(id, "failed to serialize");
			Serial.print(F("deserializeJson() failed with code "));
			Serial.println(err.c_str());
			return;
		}

		// Usage: {"action": "setspeed", "motor": 0, "speed": [from -255 to 255]}
		const char *action = json["action"];
		if (!strcmp(action, "setspeed")) {
			int motor = json["motor"];
			int speed = json["speed"];
			if (motor == 0) motorWrite(IN1_PIN, IN2_PIN, speed);
			else if (motor == 1) motorWrite(IN3_PIN, IN4_PIN, speed);
		}
	}
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
	switch (type) {
		case WS_EVT_CONNECT:
			Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
			break;
		case WS_EVT_DISCONNECT:
			Serial.printf("WebSocket client #%u disconnected\n", client->id());
			break;
		case WS_EVT_DATA:
			handleWebSocketMessage(arg, data, len, client->id());
			break;
		case WS_EVT_PONG:
			break;
		case WS_EVT_ERROR:
			break;
	}
}

void notifyClients() {
	const uint8_t size = JSON_OBJECT_SIZE(11);
	StaticJsonDocument<size> json;
	json["temp"] = temperatureRead();
	json["ic_error"] = checkDriverError();
	
	sensors_event_t a, g, temp;
	mpu.getEvent(&a, &g, &temp);
	json["acceleration"]["X"] = a.acceleration.x;
	json["acceleration"]["Y"] = a.acceleration.y;
	json["acceleration"]["Z"] = a.acceleration.z;
	
	json["gyro"]["X"] = a.gyro.x;
	json["gyro"]["Y"] = a.gyro.y;
	json["gyro"]["Z"] = a.gyro.z;
	
	json["mpu_temp"] = temp.temperature;
	
	String data;
	size_t len = serializeJson(json, data);
	ws.textAll(data);
	Serial.println(data);
}

void initMPU() {
	if (!mpu.begin()) {
		while (1) {
			delay(10);
			Serial.println("Failed to find MPU6050 chip");
		}
	}
	Serial.println("MPU6050 Found!");
	mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
	mpu.setGyroRange(MPU6050_RANGE_500_DEG);
	mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
}

void initSPIFFS() {
	if (!SPIFFS.begin()) {
		Serial.println("Cannot mount SPIFFS volume...");
		while(1) digitalWrite(LED_BUILTIN, millis() % 200 < 50 ? HIGH : LOW);
	}
}

void initWiFi() {
	Serial.print("Connecting to ");
	Serial.println(ssid);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.println("WiFi connected.");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
}

void initWebServer() {
	server.on("/", getRoot);
	server.onNotFound(page404);
	server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
	server.begin();
}

void initWebSocket() {
	ws.onEvent(onWsEvent);
	server.addHandler(&ws);
}

void setup() {
	pinMode(IN1_PIN, OUTPUT);
	pinMode(IN2_PIN, OUTPUT);
	pinMode(IN3_PIN, OUTPUT);
	pinMode(IN4_PIN, OUTPUT);
	pinMode(SLEEP_PIN, OUTPUT);
	pinMode(ERROR_PIN, OUTPUT);

	digitalWrite(SLEEP_PIN, HIGH);
	digitalWrite(IN1_PIN, LOW);
	digitalWrite(IN2_PIN, LOW);
	digitalWrite(IN3_PIN, LOW);
	digitalWrite(IN4_PIN, LOW);

	Serial.begin(115200);
	initMPU();
	initSPIFFS();
	initWiFi();
	initWebSocket();
	initWebServer();
}

void loop() {
	ws.cleanupClients(16);
	notifyClients();
	delay(2500);
}