#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ---------------- Cấu hình Kết nối ----------------
const char* ssid = "Xiaomi";
const char* password = "34567890";
const char* mqtt_server = "mqttserver.ddns.net"; // Hoặc Public IP/Domain Name của Pi 3
const int mqtt_port = 1883;

const char* CLIENT_ID = "ESP8266_DHT11"; // Client ID phải là duy nhất

// ---------------- Cấu hình Cảm biến DHT11 ----------------
#define DHTPIN D2        // Chân D2 trên NodeMCU/ESP8266
#define DHTTYPE DHT11    // Chọn loại cảm biến

DHT dht(DHTPIN, DHTTYPE);

// ---------------- Cấu hình MQTT Topics ----------------
// Topics sẽ chứa DEVICE_ID để phân biệt nguồn
const char* TEMP_TOPIC = "esp/dht11/temperature";
const char* HUMID_TOPIC = "esp/dht11/humidity";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
const int PUBLISH_INTERVAL = 500; // Gửi dữ liệu mỗi 5 giây

// ---------------- Hàm Setup/Loop ----------------

void setup_wifi() {
  delay(10);
  Serial.begin(115200);
  Serial.println("\nConnecting to WiFi..");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Thử kết nối với Client ID đã định danh
    if (client.connect(CLIENT_ID)) {
      Serial.println("connected!");
      // Không cần subscribe nếu chỉ gửi dữ liệu
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  dht.begin();
  Serial.println("DHT Sensor Started.");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > PUBLISH_INTERVAL) {
    lastMsg = now;

    // Đọc dữ liệu từ DHT11
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    // Kiểm tra đọc thành công
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Chuyển đổi giá trị sang chuỗi
    char temp_str[8];
    char humid_str[8];
    dtostrf(t, 4, 2, temp_str); // Định dạng 4 ký tự, 2 số thập phân
    dtostrf(h, 4, 2, humid_str);

    // Gửi Nhiệt độ
    Serial.print("Publishing [");
    Serial.print(TEMP_TOPIC);
    Serial.print("]: ");
    Serial.println(temp_str);
    client.publish(TEMP_TOPIC, temp_str);

    // Gửi Độ ẩm
    Serial.print("Publishing [");
    Serial.print(HUMID_TOPIC);
    Serial.print("]: ");
    Serial.println(humid_str);
    client.publish(HUMID_TOPIC, humid_str);
  }
}