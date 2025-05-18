#include <WiFi.h>
#include <WebServer.h>
#include <HX711.h>
#include <ESP32Servo.h>


const char* ssid = "Name";
const char* password = "Password";


const int motorPin1 = 12;
const int motorPin2 = 14;
const int stepsPerRevolution = 200;
const int feedSteps = 100;


// Датчик веса HX711
const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 4;
HX711 scale;


// Веб-сервер
WebServer server(80);


// Переменные состояния
float currentWeight = 0;
bool feedingInProgress = false;
String lastFeedingTime = "Never";
int portionSize = 1; // Размер порции (1-5)


void setup() {
  Serial.begin(115200);
  
  // Инициализация пинов
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  
  // Инициализация датчика веса
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(2280.f);
  scale.tare();
  
  // Подключение к Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());
  
  // Настройка веб-сервера
  server.on("/", handleRoot);
  server.on("/feed", handleFeed);
  server.on("/status", handleStatus);
  server.on("/calibrate", handleCalibrate);
  server.on("/setportion", handleSetPortion);
  server.on("/style.css", handleCSS);
  server.begin();
}


void loop() {
  server.handleClient();
  static unsigned long lastWeightUpdate = 0;
  if (millis() - lastWeightUpdate > 5000) {
    updateWeight();
    lastWeightUpdate = millis();
  }
}


// Функция кормления
void feedPet(int portions = 1) {
  if (feedingInProgress) return;
  
  feedingInProgress = true;
  Serial.println("Feeding started");
  
  for (int i = 0; i < portions; i++) {
    for (int j = 0; j < feedSteps; j++) {
      digitalWrite(motorPin1, HIGH);
      digitalWrite(motorPin2, LOW);
      delay(5);
      digitalWrite(motorPin1, LOW);
      digitalWrite(motorPin2, HIGH);
      delay(5);
    }
    delay(500);
  }
  
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  
  lastFeedingTime = getTimeString();
  updateWeight();
  feedingInProgress = false;
  Serial.println("Feeding completed");
}


void updateWeight() {
  if (scale.is_ready()) {
    currentWeight = scale.get_units(5);
  }
}


String getTimeString() {
  unsigned long seconds = millis() / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  return String(hours) + "h " + String(minutes % 60) + "m " + String(seconds % 60) + "s ago";
}


// Обработчики веб-запросов
void handleRoot() {
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Smart Pet Feeder</title>
  <link rel="stylesheet" href="/style.css">
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <div class="container">
    <h1>Smart Pet Feeder</h1>
    
    <div class="status-card">
      <h2>Current Status</h2>
      <div class="status-item">
        <span class="label">Food Level:</span>
        <span class="value">)=====" + String(currentWeight, 2) + R"=====( kg</span>
      </div>
      <div class="status-item">
        <span class="label">Last Feeding:</span>
        <span class="value">)=====" + lastFeedingTime + R"=====(</span>
      </div>
      <div class="status-item">
        <span class="label">Portion Size:</span>
        <span class="value">)=====" + String(portionSize) + R"=====(</span>
      </div>
    </div>
    
    <div class="control-panel">
      <h2>Controls</h2>
      <div class="button-group">
        <a href="/feed" class="btn feed-btn">Feed Now</a>
      </div>
      
      <form action="/setportion" method="get" class="portion-form">
        <label for="portion">Set Portion Size (1-5):</label>
        <input type="number" id="portion" name="size" min="1" max="5" value=")=====" + String(portionSize) + R"=====(">
        <button type="submit" class="btn">Set</button>
      </form>
      
      <form action="/calibrate" method="get" class="calibrate-form">
        <label>Calibrate Scale:</label>
        <input type="text" name="weight" placeholder="Known weight (kg)">
        <button type="submit" class="btn calibrate-btn">Calibrate</button>
      </form>
    </div>
  </div>
</body>
</html>
)=====";
  
  server.send(200, "text/html", html);
}


void handleCSS() {
  String css = R"=====(
body {
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  background-color: #f5f5f5;
  margin: 0;
  padding: 20px;
  color: #333;
}


.container {
  max-width: 800px;
  margin: 0 auto;
  background-color: white;
  border-radius: 10px;
  box-shadow: 0 2px 10px rgba(0,0,0,0.1);
  padding: 20px;
}


h1 {
  color: #2c3e50;
  text-align: center;
  margin-bottom: 30px;
}


.status-card {
  background-color: #f8f9fa;
  border-radius: 8px;
  padding: 20px;
  margin-bottom: 20px;
}


.status-item {
  display: flex;
  justify-content: space-between;
  margin-bottom: 10px;
  padding: 8px 0;
  border-bottom: 1px solid #eee;
}


.label {
  font-weight: bold;
  color: #555;
}


.value {
  color: #2c3e50;
}


.control-panel {
  background-color: #f8f9fa;
  border-radius: 8px;
  padding: 20px;
}


.button-group {
  margin-bottom: 20px;
  text-align: center;
}


.btn {
  display: inline-block;
  background-color: #3498db;
  color: white;
  padding: 10px 20px;
  border-radius: 5px;
  text-decoration: none;
  font-weight: bold;
  border: none;
  cursor: pointer;
  transition: background-color 0.3s;
}


.btn:hover {
  background-color: #2980b9;
}


.feed-btn {
  background-color: #2ecc71;
  font-size: 18px;
  padding: 15px 30px;
}


.feed-btn:hover {
  background-color: #27ae60;
}


.calibrate-btn {
  background-color: #e74c3c;
}


.calibrate-btn:hover {
  background-color: #c0392b;
}


form {
  margin-bottom: 20px;
}


label {
  display: block;
  margin-bottom: 5px;
  font-weight: bold;
}


input[type="number"],
input[type="text"] {
  width: 100%;
  padding: 10px;
  border: 1px solid #ddd;
  border-radius: 5px;
  margin-bottom: 10px;
  box-sizing: border-box;
}


@media (max-width: 600px) {
  .container {
    padding: 10px;
  }
  
  .btn {
    width: 100%;
    box-sizing: border-box;
  }
}
)=====";
  
  server.send(200, "text/css", css);
}


void handleFeed() {
  feedPet(portionSize);
  server.sendHeader("Location", "/");
  server.send(303);
}


void handleStatus() {
  String json = "{";
  json += "\"weight\":" + String(currentWeight, 2) + ",";
  json += "\"lastFeeding\":\"" + lastFeedingTime + "\",";
  json += "\"portionSize\":" + String(portionSize) + ",";
  json += "\"status\":\"" + (feedingInProgress ? "feeding" : "idle") + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}


void handleCalibrate() {
  if (server.hasArg("weight")) {
    float knownWeight = server.arg("weight").toFloat();
    if (knownWeight > 0) {
      calibrateScale(knownWeight);
    }
  }
  server.sendHeader("Location", "/");
  server.send(303);
}


void handleSetPortion() {
  if (server.hasArg("size")) {
    int newPortion = server.arg("size").toInt();
    if (newPortion >= 1 && newPortion <= 5) {
      portionSize = newPortion;
    }
  }
  server.sendHeader("Location", "/");
  server.send(303);
}


void calibrateScale(float knownWeight) {
  scale.set_scale();
  scale.tare();
  delay(5000);
  
  float reading = scale.get_units(10);
  float newScale = reading / knownWeight;
  scale.set_scale(newScale);
}