#include <WiFi.h>
#include <HTTPClient.h>
#include "SSD1306Wire.h"
#include <ArduinoJson.h>
#include <qrcode.h>

#define OLED_SDA 21
#define OLED_SCL 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define BUTTON_PIN 38

SSD1306Wire display(0x3c, OLED_SDA, OLED_SCL);

const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASS";

// Function declarations
String getHackerNewsPost(int index, const DynamicJsonDocument& topPostIDsJson);
void renderScreen(const char* title, const char* url);
void displayLoadingScreen(String message);

// Global variables
int currentPostIndex = 0;
volatile bool buttonPressedFlag = false;
DynamicJsonDocument topPostIDsJson(5128);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize OLED
  display.init();
  display.setContrast(255);
  display.flipScreenVertically();

  // Display loading message
  displayLoadingScreen("Connecting to wifi...");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  // Fetch the list of new stories
  HTTPClient http;
  http.begin("https://hacker-news.firebaseio.com/v0/newstories.json");
  int httpResponseCode = http.GET();

  if (httpResponseCode == HTTP_CODE_OK) {
    String topPostIDs = http.getString();
    DynamicJsonDocument doc(5128);
    deserializeJson(doc, topPostIDs);
    topPostIDsJson = doc;
  }

  http.end();

  // Get the first post
  displayLoadingScreen("Loading post...");

  String post = getHackerNewsPost(currentPostIndex, topPostIDsJson);

  // Parse the JSON response
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, post);
  const char* title = doc["title"];
  const char* url = doc["url"];

  // Render the screen
  renderScreen(title, url);

  // Set up button interrupt
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), [](){
    buttonPressedFlag = true;
  }, FALLING);
}

void loop() {
  if (buttonPressedFlag) {
    currentPostIndex = (currentPostIndex + 1) % 500;

    // Display loading message
    displayLoadingScreen("Loading post...");

    // Fetch the next post
    String post = getHackerNewsPost(currentPostIndex, topPostIDsJson);

    // Parse the JSON response
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, post);
    const char* title = doc["title"];
    const char* url = doc["url"];

    // Render the screen
    renderScreen(title, url);

    buttonPressedFlag = false;
  }
}

String getHackerNewsPost(int index, const DynamicJsonDocument& topPostIDsJson) {
  HTTPClient http;
  String response = "";

  String postID = topPostIDsJson[index].as<String>();
  String apiUrl = "https://hacker-news.firebaseio.com/v0/item/" + postID + ".json";

  http.begin(apiUrl);
  int httpResponseCode = http.GET();

  if (httpResponseCode == HTTP_CODE_OK) {
    response = http.getString();
  }

  http.end();
  // Print response to serial for debug purposes
  Serial.println(response);
  return response;
}

void renderScreen(const char* title, const char* url) {
  // Display the post on the OLED screen
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  //display.drawString(0, 0, "Top HackerNews Post:");
  display.drawStringMaxWidth(0, 0, 64, title);

  // generate QR code
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, 0, (const char*)url);
  // Draw the qr code with 2x2 pixels using setPixelColor
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        display.setPixelColor((x*2)+64, y*2, WHITE);
        display.setPixelColor((x*2)+64+1, y*2, WHITE);
        display.setPixelColor((x*2)+64, y*2+1, WHITE);
        display.setPixelColor((x*2)+64+1, y*2+1, WHITE);
      }
    }
  }
  display.display();
}

void displayLoadingScreen(String message) {
  // Display loading message with custom message
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, message);
  display.display();
}