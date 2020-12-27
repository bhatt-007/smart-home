#include <ESPmDNS.h>
#include <WiFi.h>
#include <WebServer.h>

#define POWER_ON 1
#define POWER_OFF 0

// Replace with your network credentials

const char* ssid = "Toofan";
const char* password = "NahiBataunga";
// Set web server port number to 80
WebServer server(80);
int heaterpin = 23;
int status = 0;
void setup() {
	Serial.begin(115200);
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
	if(!MDNS.begin("heater-server")) {
		Serial.println("Error starting mDNS");
		return;
	}
	MDNS.addService("http", "tcp", 80);
	server.on("/power-status", handle_get_power_status);
	server.on("/power-on", handle_post_power_on);
	server.on("/power-off", handle_post_power_off);
	server.onNotFound(handle_NotFound);
	server.begin();
	Serial.begin(115200);
	pinMode(heaterpin, OUTPUT);
}
void handle_NotFound() {
	server.send(404, "text/plain", "Not found");
}
void handle_get_power_status() {
	Serial.println("In get power status");
	char power_status[5]="";
	sprintf(power_status, "%d",status );
	server.send(200, "text/html", power_status);
}

void handle_post_power_on() {
	if(status == POWER_OFF){
	Serial.println("Powering On");
	digitalWrite(heaterpin, HIGH);   // turn the LED on (HIGH is the voltage level)
	char power_status[3]="";
	status = POWER_ON;
	sprintf(power_status, "%d",status );
	server.send(200, "text/html", power_status);
	}
	else {
		server.send(200, "text/html");
		Serial.println("Power already On");
	}
}
	void handle_post_power_off() {
		if(status == POWER_ON) {
			Serial.println("Powering OFF");
			digitalWrite(heaterpin, LOW);   // turn the LED on (HIGH is the voltage level)
			char power_status[5]="";
			status = POWER_OFF;
			sprintf(power_status, "%d",status );
			server.send(200, "text/html", power_status);
			}
		else {
		server.send(200, "text/html");
		Serial.println("Power already Off");
		}
	}

	void loop() {
		server.handleClient();
	}
