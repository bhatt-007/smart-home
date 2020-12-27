#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>        // Include the mDNS library
#include <ESP8266HTTPClient.h>
#include <stdio.h>
#include <DHT.h>

#define DHT11_PIN D1
#define DHTTYPE DHT11   // DHT 22  (AM2302), AM2321
#define POWER_ON '1'
#define POWER_OFF '0'
DHT dht(DHT11_PIN, DHTTYPE);

const char* ssid = "Toofan";
const char* password = "NahiBataunga";
ESP8266WebServer server(80);
WiFiClient client;
int power_off_lock = 0;
int power_on_lock = 0;
int threshold_temp = 25;
float curr_temp, curr_humidity;
unsigned long startMillis, currentMillis;  //some global variables available anywhere in the program
char host[20]="";

void setup() {
	Serial.begin(115200);         // Start the Serial communication to send messages to the computer
	delay(10);
	Serial.println('\n');
	WiFi.begin(ssid, password);
	Serial.println("");
	Serial.println("Connecting ...");
	Serial.println("");
	// Wait for connection
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.print("Connected to ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	if (!MDNS.begin("wms")) {
		Serial.println("Error setting up MDNS responder!");
		return;
	}
	Serial.println("mDNS responder started");
	Serial.println("TCP server started");
	// Temperature sensor setup begins
	server.on("/get-temperature", handle_On_Get_Temp_Connect);
	server.on("/get-humidity", handle_On_Get_Humid_Connect);
	server.on("/set-temperature", HTTP_POST, handle_On_Set_Temp_Connect);
	server.onNotFound(handle_NotFound);
	MDNS.addService("http", "tcp", 80);
	server.begin();
	pinMode(DHT11_PIN, INPUT);
	dht.begin();
	startMillis = millis();  //initial start time
	int nrOfServices = MDNS.queryService("http", "tcp");
	if (nrOfServices == 0) {
		Serial.println("No services were found.");
	}
	for (int i = 0; i < nrOfServices; i=i+1) {
		if(MDNS.hostname(i)=="heater-server.local") {
			ConvertAddress(MDNS.IP(i)).toCharArray(host,20);
			Serial.println(host);
		}
		else {
			Serial.println("No heater-server found");
		}
	}
}

String ConvertAddress(IPAddress address)
{
	return String(address[0]) + "." + 
		String(address[1]) + "." + 
		String(address[2]) + "." + 
		String(address[3]);
}
void handle_NotFound() {
	server.send(404, "text/plain", "Not found");
}

void  handle_On_Get_Temp_Connect() {
	curr_temp = dht.readTemperature(); // Gets the values of the temperature
	char temperature[6] = "";
	sprintf(temperature, "%f", curr_temp);
	server.send(200, "text/html", temperature);
}
void  handle_On_Set_Temp_Connect() {
	Serial.println("handle_On_Set_Temp_Connect");
	if(server.hasArg("set-temperature")) {
		char settemp[20]="";
		server.arg("set-temperature").toCharArray(settemp, 20);
		Serial.println(settemp);
		threshold_temp = atof(settemp);
		Serial.println("Threshold Temperature");
		Serial.println(threshold_temp);
		server.send(200, "text/html");
	}
}
void  handle_On_Get_Humid_Connect() {
	curr_humidity = dht.readHumidity(); // Gets the values of the temperature
	char humidity[6] = "";
	sprintf(humidity , "%f", curr_humidity);
	server.send(200, "text/html", humidity);
}

void loop() {

	MDNS.update();
	server.handleClient();
	currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)
	const int httpPort = 80;
	if (!client.connect(host, httpPort)) {
		Serial.println("connection failed");
		delay(2000);
		return;
	}
	if (currentMillis - startMillis >= 2000)  //test whether the period has elapsed
	{ 
		curr_temp = dht.readTemperature();
		startMillis = currentMillis;
	}
	if((threshold_temp <= curr_temp)&&(power_off_lock < 10)) {
		power_off_lock++;
		power_on_lock = 0;
		// power off the heater
		Serial.println("Sending Power Off");
		String url = "/power-off";
		Serial.print("Requesting URL: ");
		Serial.println(url);
		Serial.print("Requesting GET: ");
		client.print(String("GET ") + url + " HTTP/1.1\r\n" +"Connection: close\r\n\r\n");
		unsigned long timeout = millis();
		while (client.available() == 0) {
			if (millis() - timeout > 10000) {
				Serial.println(">>> Client Timeout !");
				client.stop();
				break;
			}
		}
	delay(1000);
	}
	else if((curr_temp < threshold_temp - 2)&&(power_on_lock < 10)) {
		power_off_lock = 0;
		power_on_lock++;
		// power on the heater
		Serial.println("Sending Power On");
	// We now create a URI for the request
		String url = "/power-on";
		Serial.print("Requesting URL: ");
		Serial.println(url);
		Serial.print("Requesting GET: ");
		client.print(String("GET ") + url + " HTTP/1.1\r\n" +"Connection: close\r\n\r\n");
		unsigned long timeout = millis();
		while (client.available() == 0) {
			if (millis() - timeout > 5000) {
				Serial.println(">>> Client Timeout !");
				client.stop();
				break;
				}
			}
		client.stop();
		delay(1000);
	}
}
