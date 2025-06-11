#include <WebServer.h>
#include <WiFi.h>
#include <esp32cam.h>

const char* WIFI_SSID = "esp32-ap";
const char* WIFI_PASS = "12345678";

WebServer server(80);

// Define available resolutions
static auto hiRes = esp32cam::Resolution::find(800, 600);  // High resolution
static auto medRes = esp32cam::Resolution::find(640, 480); // Medium resolution (fixed)
static auto lowRes = esp32cam::Resolution::find(320, 240); 

#define LED_PIN 4 // Built-in LED on ESP32-CAM

void serveJpg() {
  auto frame = esp32cam::capture();
  if (frame == nullptr) {
    Serial.println("CAPTURE FAIL");
    server.send(503, "", "");
    return;
  }
  Serial.printf("CAPTURE OK %dx%d %db\n", frame->getWidth(), frame->getHeight(),
                static_cast<int>(frame->size()));
  server.setContentLength(frame->size());
  server.send(200, "image/jpeg");
  WiFiClient client = server.client();
  frame->writeTo(client);
}

void handleJpgHi() {
  if (!esp32cam::Camera.changeResolution(hiRes)) {
    Serial.println("SET-HI-RES FAIL");
  }
  serveJpg();
}

void handleJpgMed() {  // Fixed function name and variable
  if (!esp32cam::Camera.changeResolution(medRes)) {
    Serial.println("SET-MED-RES FAIL");
  }
  serveJpg();
}

void handleJpgLow() {  // Fixed function name and variable
  if (!esp32cam::Camera.changeResolution(lowRes)) {
    Serial.println("SET-LOW-RES FAIL");
  }
  serveJpg();
}

void blink() {
  server.send(200, "text/plain", "blinked");
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
}

void noBlink() {
  digitalWrite(LED_PIN, LOW);
  server.send(200, "text/plain", "stopped blinking");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  using namespace esp32cam;
  Config cfg;
  cfg.setPins(pins::AiThinker);
  cfg.setResolution(hiRes);  // Start with high resolution
  cfg.setBufferCount(2);
  cfg.setJpeg(80);
  bool ok = Camera.begin(cfg);
  Serial.println(ok ? "CAMERA OK" : "CAMERA FAIL");

  // Set up ESP32-CAM as an Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASS);

  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  Serial.println("  /cam-hi.jpg (High Resolution)");
  Serial.println("  /cam-med.jpg (Medium Resolution)");
  Serial.println("  /cam-low.jpg (Low Resolution)");
  Serial.println("  /blink");

  server.on("/cam-hi.jpg", handleJpgHi);
  server.on("/cam-med.jpg", handleJpgMed);
  server.on("/cam-low.jpg", handleJpgLow);
  server.on("/blink", blink);
  server.on("/no_blink", noBlink);

  server.begin();

  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  server.handleClient();
}

