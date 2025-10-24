#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// ---------------- Cấu hình Wi-Fi và MQTT ----------------
const char* ssid = "Xiaomi";          // Tên Wi-Fi
const char* password = "34567890";  // Mật khẩu Wi-Fi
const char* mqtt_server = "mqttserver.ddns.net";    // Địa chỉ IP của Raspberry Pi (Mosquitto)
const int mqtt_port = 1883;                   // Cổng mặc định không mã hóa và không xác thực
const char* mqtt_client_id = "ESP8266_VL53L0X";
const char* mqtt_topic = "esp/vl53l0x/distance"; // Chủ đề để gửi dữ liệu khoảng cách

// ---------------- Khởi tạo ----------------
WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

long lastMsg = 0;
char msg[50];
int value = 0;

// ---------------- Kết nối Wi-Fi ----------------
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// ---------------- Kết nối lại MQTT ----------------
void reconnect() {
  // Loop cho đến khi kết nối được
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Kết nối lại *KHÔNG* sử dụng username/password
    if (client.connect(mqtt_client_id)) { 
      Serial.println("connected");
      // Có thể đăng ký (subscribe) một chủ đề nếu cần
      // client.subscribe("test/input"); 
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  Wire.begin(); // Khởi tạo I2C

  setup_wifi();
  
  // Thiết lập MQTT broker
  client.setServer(mqtt_server, mqtt_port);
  // Không cần client.setCallback(callback); nếu chỉ publish

  // Khởi tạo cảm biến VL53L0X
  Serial.println("VL53L0X Test");
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }
}

// ---------------- Loop ----------------
void loop() {
  // Đảm bảo kết nối MQTT vẫn còn
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  // Gửi dữ liệu mỗi 2 giây (hoặc theo nhu cầu)
  if (now - lastMsg > 500) { 
    lastMsg = now;

    // Đọc khoảng cách từ VL53L0X
    VL53L0X_RangingMeasurementData_t measure;
    lox.rangingTest(&measure, false); // Đo và không in ra Serial

    if (measure.RangeStatus != 4) { // 4 = out of range, error
      int distance_mm = measure.RangeMilliMeter;
      
      // Chuyển đổi khoảng cách sang chuỗi
      String messageString = String(distance_mm);
      messageString.toCharArray(msg, 50);

      Serial.print("Publishing distance (mm) [");
      Serial.print(mqtt_topic);
      Serial.print("]: ");
      Serial.println(msg);

      // Gửi dữ liệu qua MQTT
      client.publish(mqtt_topic, msg);
    } else {
      Serial.println("VL53L0X out of range / error");
    }
  }
}