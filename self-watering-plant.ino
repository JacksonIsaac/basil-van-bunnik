// Libraries
#include <ESP8266WiFi.h>
#include <TwitterWebAPI.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Pins
# define LED D4                 // Use built-in LED which connected to D4 pin or GPIO 20
# define ANALOG A0              // Analog Input pin
# define DIGITAL D5             // Digital Input pin
# define WATERPUMP D6           // Waterpump Output pin
# define MOIST_SENSOR D1        // Rewiring Moisture Sensor to prevent corrosion

// SECRETS and KEYS
# define SECRET_SSID "<replaceMe>"
# define SECRET_PWD "<replaceMe>"
# define TWI_API_KEY "<replaceMe>"
# define TWI_API_SECRET "<replaceMe>"
# define TWI_ACCESS_KEY "<replaceMe>"
# define TWI_ACCESS_SECRET "<replaceMe>"

# define TWI_TIMEOUT 2000        // Twitter Timeout in msec

// Global Variables
double analogInputValue = 0.0;  // Can also be int, to use less memory
double digitalInputValue = 0.0; // Can also be int, to use less memory
double analogThreshold = 400.0; // Set moisture sensor threshold for analog input
int readDelay = 500;            // 1 second = 1000
int pumpTime = 5000;            // Pump water for 5 seconds
int noOfReadings = 10;          // Take a range of input readings
int sleepTime = 3600e6;         // Sleep time = 1 hour Ref: https://thingpulse.com/max-deep-sleep-for-esp8266/

// WiFi configuration
const char* ssid = SECRET_SSID;
const char* password = SECRET_PWD;

// Setup timezone for NTP server
const char *ntp_server = "pool.ntp.org";  // time1.google.com, time.nist.gov, pool.ntp.org
int timezone = +1;                        // CET +01:00 HRS when DST is off
int dstTimezone = +2;                     // CET +02:00 HRS when DST is on

static char const consumer_key[]    = TWI_API_KEY;
static char const consumer_sec[]    = TWI_API_SECRET;
static char const accesstoken[]     = TWI_ACCESS_KEY;
static char const accesstoken_sec[] = TWI_ACCESS_SECRET;

// Declare clients for Twitter
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntp_server, timezone*3600, 60000);  // NTP server pool, offset (in seconds), update interval (in milliseconds)
TwitterClient tcr(timeClient, consumer_key, consumer_sec, accesstoken, accesstoken_sec);

void configureWiFi() {
  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  bool tryingToConnect = true;

  while(tryingToConnect){                 // Wait for the Wi-Fi to connect
    delay(500);
    Serial.print(".");

    if (WiFi.status() == WL_CONNECTED){   // WiFi Connected
      Serial.println('\n');
      Serial.println("Connection established!");

      Serial.print("IP address:\t");
      Serial.println(WiFi.localIP());     // Send the IP address of the ESP8266 to the computer

      tryingToConnect = false;
    } else if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL){ 
      Serial.println("Connection failed! I am now going to sleep for: "); 
      Serial.print(sleepTime / 1e6);
      Serial.print(" seconds");
      ESP.deepSleep(sleepTime);           // WiFi Connection failed, or WiFi unavailable
    }
  }

}

void pumpWater() {
  Serial.println("Turning on the water pump");
  digitalWrite(WATERPUMP, HIGH);
  digitalWrite(LED, LOW);                 // Turn the LED on while pumping water

  delay(pumpTime);

  Serial.println("Turning off the water pump");
  digitalWrite(WATERPUMP, LOW);
  digitalWrite(LED, HIGH);                // Turn the LED off
//  delay(readDelay);                       // delay between next read for water to soak in
}

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println('\n');

  // Set pin modes
  pinMode(ANALOG, INPUT);                 // Initialize the ANALOG as input
  pinMode(DIGITAL, INPUT);                // Initialize the DIGITAL as input
  pinMode(WATERPUMP, OUTPUT);             // Initialize the WATERPUMP as output
  pinMode(LED, OUTPUT);                   // Initialize LED as output
  pinMode(MOIST_SENSOR, OUTPUT);          // Initialize moidure sensor as output

  Serial.println("Let's water the plant or not!");

  digitalWrite(LED, HIGH);                // Turn the LED off because the LED is active low
  digitalWrite(MOIST_SENSOR, HIGH);       // Turn on the moisture sensor for reading in
  delay(2000);                            // Some wait time to let the moisture sensor to start up

  // Digital reading is either HIGH or LOW, hence not going to use that for now
  // Reset/Initialize analog input reading.
  analogInputValue = 0.0;

  Serial.print("Taking analog readings");
  for(int i = 0; i < noOfReadings; i++) {
    Serial.print('.');
    analogInputValue = analogInputValue + analogRead(ANALOG);
    delay(readDelay);                     // Let's give some time between the readings from sensor
  }
  Serial.print('\n');

  digitalWrite(MOIST_SENSOR, LOW);        // Turn off the moisture sensor after reading
  double analogValue = analogInputValue / noOfReadings;

  char* analogReading = "Current moisture level in soil: ";
//  Serial.print(analogReading);
//  Serial.println(analogValue);

  char analogValueString[8];
  dtostrf(analogValue, 6, 2, analogValueString);

  analogReading = strcat(analogReading, analogValueString);
  Serial.println(analogReading);

  char* soilStatus;

  if (analogValue > analogThreshold) {
    pumpWater();
    soilStatus = "Plant needs some love. Watering it <3";
  } else {
    Serial.println("Soil is sufficiently moist!");
    soilStatus = "Plant is happy and blooming!";
  }

  configureWiFi();

  tcr.startNTP();
  std::string tweetMSG = strcat(strcat(analogReading, "\n"), soilStatus);
//  Serial.println(tweetMSG.c_str());
  tcr.tweet(tweetMSG);

  Serial.println("I am now going to sleep for: ");
  Serial.print(sleepTime / 1e6);
  Serial.print(" seconds");
  ESP.deepSleep(sleepTime);               //sleeping
}

void loop() {
}
