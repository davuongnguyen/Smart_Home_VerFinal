#pragma region Khai báo thư viện

#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#pragma endregion Khai báo các thư viện cần thiết cho dự án nhà thông minh.

#pragma region Định nghĩa từ khóa

// Các từ khóa cấu hình wifi
#define WLAN_SSID "MYUI"
#define WLAN_PASS "11111111"

// Các từ khóa cấu hình MQTT
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883

#define IO_USERNAME "smarthome_doan"
#define IO_KEY "aio_KvhG23VXUzzdFzFeXTchD0EX6fqH"

// Định nghĩa chân các thiết bị
#define PW_PIN 4          //Led này tích cực mức cao
#define WIFI_PIN 5
#define MQTT_PIN 12
#define FAN_AUTO 0
#define FAN_PIN 13

// Định nghĩa các chân nút bấm
#define BUTTON_FAN 14
#define BUTTON_FAN_AUTO 16

#pragma endregion

#pragma region Khởi tạo

// Khởi tạo thông số kết nối tới MQTT
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, IO_USERNAME, IO_KEY);

// Khởi tạo liên kết tới nguồn cấp dữ liệu
Adafruit_MQTT_Subscribe fan = Adafruit_MQTT_Subscribe(&mqtt, IO_USERNAME "/feeds/fan");
Adafruit_MQTT_Subscribe fanAuto = Adafruit_MQTT_Subscribe(&mqtt, IO_USERNAME "/feeds/fan-auto");
Adafruit_MQTT_Subscribe heatIndex = Adafruit_MQTT_Subscribe(&mqtt, IO_USERNAME "/feeds/heat-index");

Adafruit_MQTT_Publish fan_Publish = Adafruit_MQTT_Publish(&mqtt, IO_USERNAME "/feeds/fan");
Adafruit_MQTT_Publish fanAuto_Publish = Adafruit_MQTT_Publish(&mqtt, IO_USERNAME "/feeds/fan-auto");

Adafruit_MQTT_Subscribe *subscription;

#pragma endregion

#pragma region Các biến toàn cục

unsigned long time_Reconnect = 0;

// Nút bấm quạt, nút bấm quạt tự động
const byte INPUT_PIN[] = {BUTTON_FAN, BUTTON_FAN_AUTO};

// Đầu ra led nguồn, led wifi, led MQTT
const byte OUTPUT_PIN[] = {PW_PIN, WIFI_PIN, MQTT_PIN, FAN_AUTO};

// Có sự thay đổi nút nhấn: quạt, chế độ quạt tự động
byte change_button[] = {0, 0};

// Biến lưu chỉ số nóng bức
int hic;

// Chế độ quạt: 
// 0 - tự động
// 1 - tùy chỉnh
byte fanMode = 1;

//Lưu tốc độ quạt
int speed = 0; //Lưu tốc độ quạt

// Tốc độ quạt tự động
int speed_auto = 0;

// Trạng thái online / offline
byte mode = 0;

// Biến lưu trạng thái đã tải lên thành công
byte val = 0;

#pragma endregion

#pragma region Nguyên mẫu các hàm

// Kiểm tra trạng thái kết nối Wifi
// In ra tên Wifi và địa chỉ IP cục bộ
// Mỗi 10s tối đa kiểm tra 1 lần
// Nếu có kết nối sẽ sáng đèn Wifi
void wifi_connect();

// Kiểm tra kết nối tới MQTT, nếu có kết nối sẽ sáng đèn MQTT
void mqtt_connect();

// Đọc dữ liệu từ nút bấm và thực thi lệnh từ nút bấm, sau đó chuyển trạng thái nút bấm thành có thay đổi
void read_button();

// Gửi dữ liệu từ nút bấm lên server nếu có thay đổi từ nút bấm
void send_button();

// Nhận dữ liệu về chỉ số nóng bức từ MQTT
void receive_heat_index();

// Nhận tốc độ quạt từ MQTT
void receive_fan_speed();

// Nhận trạng thái của chế độ quạt từ MQTT
void receive_fan_auto();

// Ping tới server để giữ kết nối tồn tại
// Không bắt buộc nếu gửi dữ liệu lên server mỗi giây
void mqtt_ping();

void fan_auto(byte a);

#pragma endregion

void setup()
{
  //Thiết lập tốc độ baud rate cho serial monitor
  Serial.begin(115200);

  // Thiết lập kết nối tới Wifi
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  Serial.println("Đang kết nối tới wifi MYUI ...");

  for (byte i = 0; i < 2; i++)
  {
    pinMode(INPUT_PIN[i], INPUT);
  }
  for (byte k = 0; k < 4; k++)
  {
    pinMode(OUTPUT_PIN[k], OUTPUT);
    digitalWrite(OUTPUT_PIN[k], HIGH);
  }
  analogWrite(FAN_PIN, 0);

  // Thiết lập đăng ký nguồn cấp dữ liệu trên MQTT theo thời gian
  mqtt.subscribe(&heatIndex);
  mqtt.subscribe(&fan);
  mqtt.subscribe(&fanAuto);
}

void loop()
{
  wifi_connect();

  read_button();

  // Nếu có kết nối (Chế độ online)
  if (mode == 1)
  {
    send_button();

    // Nếu dữ liệu từ nút bấm đã được tải lên thành công
    if (val == 0)
    {
      // Vòng lặp để nhận đầy đủ dữ liệu
      while ((subscription = mqtt.readSubscription(1000)))
      {
        receive_heat_index();
        receive_fan_speed();
        receive_fan_auto();
      }
    }

    mqtt_ping();

    fan_auto(fanMode);
  }
}

void wifi_connect()
{
  if ((unsigned long)(millis() - time_Reconnect) > 10000)
  {
    time_Reconnect = millis();
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("Đã kết nối wifi!");
      Serial.print("Tên Wifi: MYUI. Địa chỉ IP cục bộ: ");
      Serial.println(WiFi.localIP());
      digitalWrite(WIFI_PIN, LOW);
      mqtt_connect();
      return;
    }
    digitalWrite(WIFI_PIN, HIGH);
    digitalWrite(MQTT_PIN, HIGH);
    Serial.println("Đang kết nối tới wifi MYUI ...");
    mode = 0;
  }
}

void mqtt_connect()
{
  // Chức năng kết nối và kết nối lại khi cần thiết với máy chủ MQTT.
  int8_t ret;

  // Dừng lại nếu đã được kết nối.
  if (mqtt.connected())
  {
    // Chuyển chế độ sang online.
    mode = 1;
    Serial.println("Đã kết nối tới MQTT!");
    digitalWrite(MQTT_PIN, LOW);
    return;
  }

  // Chuyển chế độ về offline/
  digitalWrite(MQTT_PIN, HIGH);
  mode = 0;

  if ((ret = mqtt.connect()) != 0)
  {
    Serial.println(mqtt.connectErrorString(ret));
    mqtt.disconnect();
  }
}

void read_button()
{
  if (digitalRead(BUTTON_FAN) == LOW)
  {
    change_button[0] = 1;
    if (speed >= 255)
      speed = 0;
    else
      speed += 51;
    analogWrite(FAN_PIN, speed);
    Serial.print("Tốc độ quạt: ");
    Serial.println(speed);
    delay(500);
  }
  if (digitalRead(BUTTON_FAN_AUTO) == LOW)
  {
    Serial.println("Đã nhấn thay đổi chế độ quạt.");
    fanMode = !fanMode;
    digitalWrite(FAN_AUTO, fanMode);
    change_button[1] = 1;
    delay(500);
  }
}

void send_button()
{
  for (byte i = 0; i < 2; i++)
  {
    if (change_button[i] == 1)
    {
      if (i == 0)
      {
        if (!fan_Publish.publish(speed))
        {
          Serial.println(F("Tải lên tốc độ quạt thất bại!"));
        }
        else
        {
          Serial.println(F("Tải lên tốc độ quạt thành công!"));
          change_button[i] = 0;
        }
      }
      if (i == 1)
      {
        if (!fanAuto_Publish.publish(fanMode))
        {
          Serial.println(F("Tải lên chế độ quạt thất bại!"));
        }
        else
        {
          Serial.println(F("Tải lên chế độ quạt thành công!"));
          change_button[i] = 0;
        }
      }
    }
  }
  val = change_button[0] + change_button[1];
}

void receive_heat_index()
{
  if (subscription == &heatIndex)
  {
    Serial.print(F("Chỉ số nóng bức: "));
    Serial.println((char *)heatIndex.lastread);
    uint16_t sliderval = atoi((char *)heatIndex.lastread); // chuyển thành 1 số
    hic = (int)sliderval;
  }
}

void receive_fan_speed()
{
  // Kiểm tra nếu là feed của quạt
  if (subscription == &fan)
  {
    Serial.print(F("Tốc độ quạt: "));
    Serial.println((char *)fan.lastread);
    uint16_t sliderval = atoi((char *)fan.lastread); // chuyển thành 1 số
    analogWrite(FAN_PIN, sliderval);
    speed = (int)sliderval;
  }
}

void receive_fan_auto()
{
  // Kiểm tra nếu là feed của tự động quạt
  if (subscription == &fanAuto)
  {
    if (strcmp((char *)fanAuto.lastread, "0") == 0)
    {
      Serial.println("Quạt tự động : Bật.");
      fanMode = 0;
    }
    else
    {
      Serial.println("Quạt tự động : Tắt.");
      fanMode = 1;
    }
  }
}

void mqtt_ping()
{
  // ping máy chủ để giữ cho kết nối mqtt hoạt động
  if (!mqtt.ping())
  {
    Serial.println("MQTT không phản hồi ...");
    mqtt.disconnect();
    mode = 0;
  }
}

void fan_auto(byte a)
{
  if (a == 0)
  {
    // Chọn tốc độ theo chỉ số nóng bức
    if (hic > 22 && hic < 27)
    {
      speed_auto = 51;
    }
    else if (hic >= 27 && hic < 30)
    {
      speed_auto = 153;
    }
    else if (hic >= 32 && hic < 35)
    {
      speed_auto = 204;
    }
    else if (hic >= 35)
    {
      speed_auto = 255;
    }

    // Nếu tốc độ thay đổi thì tải lên tốc độ mới
    if (speed_auto != speed)
    {
      change_button[0] = 1;
      speed = speed_auto;
      analogWrite(FAN_PIN, speed);
    }
  }
}