/*
  IoT Wifi Boat Monitor v0.1

  31/01/2017
  for Arduino MKR1000
  by Tom Kuehn

*/

#include <WiFi101.h>
#include <TinyGPS++.h>

static const uint32_t GPSBAUD = 9600;   // Define GPS baud. Connected to UART Rx

static const char BILGE_SWITCH      = 8;
static const char MKR1000_LED       = 6;
static const char BATT_MEAS         = A1;

static const unsigned int CHECK_INTERVAL = 500;           // Regular checks [milliseconds]

//================ USER SETTINGS. PLEASE UPDATE =================
char ssid[] = "WIFI_SSID";            // your network SSID (name)
char pass[] = "WIFI_PASSWORD";        // your network password
String APIKey1 = "xxxxxxxxxxxxxxxx";  // your channel's Write API Key
//===============================================================

int status = WL_IDLE_STATUS;
char thingSpeakAddress[] = "api.thingspeak.com";

const int dataUpdateRate = 60 * 1000;      //  interval at which to update ThingSpeak [milliseconds]
unsigned long lastUpdate = 0;                   // Time of last succesful connection to ThingSpeak

WiFiClient client;
TinyGPSPlus gps;

void setup()
{
  pinMode(MKR1000_LED, OUTPUT);
  digitalWrite(MKR1000_LED, HIGH);
  
  pinMode(BILGE_SWITCH, INPUT_PULLUP);

  // Start Serial for debugging on the Serial Monitor
  Serial.begin(9600);
  Serial1.begin(GPSBAUD);

  connectWifi();

  printWifiStatus();

  Serial.println("Setup Complete");
}

void connectWifi(void)
{ 
  WiFi.disconnect();

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) 
  {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) 
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
}

void loop()
{
  static unsigned long bilge_timer = millis();
  static unsigned long bilgeTimeHigh = 0;       //[seconds]
  static unsigned long dataConnectionTime = 0;
  static int battValue = 0;
  String batt = "";

  // Print Update Response to Serial Monitor
  if (client.available())
  {
    char c = client.read();
    Serial.print(c);
  }

  //client.flush();

  // Update ThingSpeak
  if (!client.connected() && (millis() - dataConnectionTime > dataUpdateRate))
  {
    digitalWrite(MKR1000_LED, HIGH);

    status = WiFi.status();
    if (status != WL_CONNECTED) 
    {
      connectWifi();
    }

    // Read battery voltage. Mapped to an int in milliVolts
    battValue = analogRead(BATT_MEAS)*57*3300/10/1024; 
    if (battValue > 1000) 
    {
      batt = String((float)battValue / 1000.0, 2);
    }

    updateThingSpeak(APIKey1, "field1=" + batt + "&field2=" + String(bilgeTimeHigh) + "&field3=" + String(gps.location.lat(), 6) + "&field4=" + String(gps.location.lng(), 6) );
    bilgeTimeHigh = 0;

    dataConnectionTime = millis();

    digitalWrite(MKR1000_LED, LOW);
  }

  if (digitalRead(BILGE_SWITCH) == LOW)   // Bilge switch activated by high water
  {
    if (millis() - bilge_timer > 1000)  // Count every second water is high
    {
      bilge_timer = millis();
      
      bilgeTimeHigh++;
    }
  }
  else
  {
    bilge_timer = millis();
  }

  // Parse GPS data
  while (Serial1.available())
  {
    char c = Serial1.read();
    gps.encode(c);
  }
}

void updateThingSpeak(String APIKey_, String tsData)
{
  // close any connection before send a new request.
  client.stop();

  if (client.connect(thingSpeakAddress, 80))
  {
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + APIKey_ + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(tsData.length());
    client.print("\n\n");
    client.print(tsData);

    if (client.connected())
    {
      Serial.println("Connecting to ThingSpeak...");
      Serial.println(tsData);
      lastUpdate = millis();
    }
  }
  else
  {
    Serial.println("Connection failed");
  }
}

// ------------------------------------------------------
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
