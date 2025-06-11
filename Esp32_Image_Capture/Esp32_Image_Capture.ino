#include <WebServer.h>
#include <WiFi.h>
#include <esp32cam.h>
#include <ESP32Servo.h>

const char* WIFI_SSID = "esp32-ap";
const char* WIFI_PASS = "12345678";

WebServer server(80);

static auto medRes = esp32cam::Resolution::find(640, 480); 


#define LED_PIN 4 
#define SERVO_PIN 15

Servo lid_servo;

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



void handleJpgMed() {  
  if (!esp32cam::Camera.changeResolution(medRes)) {
    Serial.println("Medium-res image failed to set up");
  }
  serveJpg();
}


void blink() {
  server.send(200, "text/plain", "blinked");
  lid_servo.write(90);
  digitalWrite(LED_PIN, HIGH);
  delay(2000);
  digitalWrite(LED_PIN, LOW);
  lid_servo.write(0);
}

void noBlink() {
  digitalWrite(LED_PIN, LOW);
  server.send(200, "text/plain", "stopped blinking");
  lid_servo.write(0);
}



void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  using namespace esp32cam;
  Config cfg;
  cfg.setPins(pins::AiThinker);
  cfg.setResolution(medRes);  
  cfg.setBufferCount(2);
  cfg.setJpeg(80);
  bool ok = Camera.begin(cfg);
  Serial.println(ok ? "CAMERA OK" : "CAMERA FAIL");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASS);

  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());


  Serial.println("  /cam-med.jpg (Medium Resolution)");


  server.on("/cam-med.jpg", handleJpgMed);
  server.on("/blink", blink);
  server.on("/no_blink", noBlink);

  server.begin();

  lid_servo.attach(SERVO_PIN);

  digitalWrite(LED_PIN, HIGH);
  lid_servo.write(0);
  delay(100);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  server.handleClient();
}

