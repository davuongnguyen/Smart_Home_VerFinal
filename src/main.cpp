#pragma region Khai báo thư viện

#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include "DHT.h"
#include "Adafruit_Sensor.h"
#include "DHT_U.h"

#pragma endregion Khai báo các thư viện cần thiết cho dự án nhà thông minh.

#pragma region Định nghĩa từ khóa

// Các từ khóa cấu hình wifi
#define WLAN_SSID "MYUI"
#define WLAN_PASS "11111111"

// Các từ khóa cấu hình MQTT
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883

#define IO_USERNAME "smarthome_doan"
#define IO_KEY "aio_RhZL07rLovLNuXJL5m1FNe1OOgi1"

// Từ khóa cấu hình cảm biến DHT
#define DHTTYPE DHT11

// Định nghĩa chân các thiết bị
#define DHTPIN 0
#define LIGHT_PIN 4
#define LIGHT_BEDROOM 5
#define FAN_PIN 12
#define TV_PIN 13

// Định nghĩa các chân nút bấm
#define BUTTON_LIGHT 14
#define BUTTON_BEDROOM 16
#define BUTTON_FAN 2
// #define BUTTON_TV           15

#pragma endregion

#pragma region Khởi tạo

// Khởi tạo thông số kết nối tới MQTT
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, IO_USERNAME, IO_KEY);

// Khởi tạo liên kết tới nguồn cấp dữ liệu
Adafruit_MQTT_Subscribe light = Adafruit_MQTT_Subscribe(&mqtt, IO_USERNAME "/feeds/light");
Adafruit_MQTT_Subscribe lighBedroom = Adafruit_MQTT_Subscribe(&mqtt, IO_USERNAME "/feeds/light-bedroom");
Adafruit_MQTT_Subscribe fan = Adafruit_MQTT_Subscribe(&mqtt, IO_USERNAME "/feeds/fan");
Adafruit_MQTT_Subscribe tv = Adafruit_MQTT_Subscribe(&mqtt, IO_USERNAME "/feeds/tv");
Adafruit_MQTT_Subscribe fanAuto = Adafruit_MQTT_Subscribe(&mqtt, IO_USERNAME "/feeds/fan-auto");

Adafruit_MQTT_Publish Temperature1 = Adafruit_MQTT_Publish(&mqtt, IO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish Humidity1 = Adafruit_MQTT_Publish(&mqtt, IO_USERNAME "/feeds/humidity");
Adafruit_MQTT_Publish HeatIndex = Adafruit_MQTT_Publish(&mqtt, IO_USERNAME "/feeds/heat-index");

Adafruit_MQTT_Publish light_Publish = Adafruit_MQTT_Publish(&mqtt, IO_USERNAME "/feeds/light");
Adafruit_MQTT_Publish lighBedroom_Publish = Adafruit_MQTT_Publish(&mqtt, IO_USERNAME "/feeds/light-bedroom");
Adafruit_MQTT_Publish fan_Publish = Adafruit_MQTT_Publish(&mqtt, IO_USERNAME "/feeds/fan");
// Adafruit_MQTT_Publish tv_Publish = Adafruit_MQTT_Publish(&mqtt, IO_USERNAME "/feeds/tv");

Adafruit_MQTT_Subscribe *subscription;

// Khởi tạo cảm biến DHT
DHT dht(DHTPIN, DHTTYPE);

#pragma endregion

#pragma region Các biến toàn cục

unsigned long time_Reconnect = 0, time_sensor = 0, time_button = 0;

// Khởi tạo biến lưu trữ nhiệt độ, độ ẩm, chỉ số nóng bức.
float hum = 0, temp = 0, hic = 0;

const byte INPUT_PIN[] = {BUTTON_LIGHT, BUTTON_BEDROOM, BUTTON_FAN}; // BUTTON_TV, (trước button fan)
const byte OUTPUT_PIN[] = {LIGHT_PIN, LIGHT_BEDROOM, TV_PIN};

// Có sự thay đổi nút nhấn: đèn, đèn ngủ, ti vi tốc độ quạt
byte change_button[] = {0, 0, 0, 0};

// Lưu trạng thái đầu ra: đèn, đèn ngủ, ti vi
byte state[] = {0, 0, 0};

int speed = 0; //Lưu tốc độ quạt
byte fanMode = 0;
int speed_auto = 0;

// Trạng thái online / offline
byte mode = 0;

// Biến lưu trạng thái đã tải lên thành công
byte val = 0;

#pragma endregion

#pragma region Nguyên mẫu các hàm

void wifi_connect();

void mqtt_connect();

void read_sensor();

void read_button();
void readBUTTON_LIGHT();
void readBUTTON_BEDROOM();
void readBUTTON_FAN();

void send_button();
void send_temperature();
void send_humidity();
void send_heat_index();

void receive_light();
void receive_light_bedroom();
void receive_fan_speed();
void receive_tv();
void receive_fan_auto();

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

  for (byte i = 0; i < 3; i++)
  {
    pinMode(INPUT_PIN[i], INPUT);
  }
  for (byte k = 0; k < 3; k++)
  {
    pinMode(OUTPUT_PIN[k], OUTPUT);
    digitalWrite(OUTPUT_PIN[k], HIGH);
  }
  analogWrite(FAN_PIN, 0);

  // Bắt đầu thuật toán giải mã dữ liệu từ cảm biến dth
  dht.begin();

  // Thiết lập đăng ký nguồn cấp dữ liệu trên MQTT theo thời gian
  mqtt.subscribe(&light);
  mqtt.subscribe(&lighBedroom);
  mqtt.subscribe(&fan);
  mqtt.subscribe(&tv);
  mqtt.subscribe(&fanAuto);

}

void loop()
{
  wifi_connect();

  read_sensor();

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
        receive_light();
        receive_light_bedroom();
        receive_fan_speed();
        receive_tv();
        receive_fan_auto();
      }
    }

    mqtt_ping();
  }

  fan_auto(fanMode);
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
      mqtt_connect();
      return;
    }
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
    return;
  }

  // Chuyển chế độ về offline/
  mode = 0;

  if ((ret = mqtt.connect()) != 0)
  {
    Serial.println(mqtt.connectErrorString(ret));
    mqtt.disconnect();
  }
}

void read_sensor()
{
  // Đợi một khoảng thời gian giữa các lần đọc dữ liệu
  if ((unsigned long)(millis() - time_sensor) > 20000)
  {
    time_sensor = millis();

    // Đọc dữ liệu độ ẩm và lưu vào biến hum
    hum = dht.readHumidity();
    // Đọc dữ liệu độ ẩm và lưu vào biến hum
    temp = dht.readTemperature();
    // Tính toán chỉ số nóng bức
    hic = dht.computeHeatIndex(temp, hum, false);

    // Nếu nhiệt độ hoặc độ ẩm là nan (Not a number)
    if (isnan(hum) || isnan(temp))
    {
      Serial.println(F("Đọc dữ liệu từ cảm biến thất bại!"));
      return;
    }

    // In ra giá trị và gửi dữ liệu lên máy chủ
    Serial.print(temp);
    Serial.print(" *C, ");
    Serial.print(hum);
    Serial.println(" H");
    Serial.print(hic);
    Serial.println(" - Heat Index");

    if (mode == 1)
    {
      send_temperature();
      send_humidity();
      send_heat_index();
    }
  }
}

void read_button()
{
  for (int i = 0; i < 2; i++)
  {
    if (digitalRead(INPUT_PIN[i]) == LOW)
    {
      Serial.println("Đã nhấn ...");
      digitalWrite(OUTPUT_PIN[i], state[i]);
      state[i] = !state[i];
      change_button[i] = 1;
      delay(500);
    }
  }
  if (digitalRead(BUTTON_FAN) == LOW)
  {
    change_button[2] = 1;
    if (speed >= 255)
      speed = 0;
    else
      speed += 51;
    analogWrite(FAN_PIN, speed);
    Serial.print("Tốc độ quạt: ");
    Serial.println(speed);
    delay(500);
  }
}

void send_button()
{
  if ((unsigned long)(millis() - time_button) > 100)
  {
    time_button = millis();
    for (byte i = 0; i < 3; i++)
    {
      if (change_button[i] == 1)
      {
        switch (i)
        {
        case 0:
          if (!light_Publish.publish(state[i]))
          {
            Serial.println(F("Tải lên nút bấm đèn chiếu sáng thất bại!"));
          }
          else
          {
            Serial.println(F("Tải lên nút bấm đèn chiếu sáng thành công!"));
            change_button[i] = 0;
          }
          break;

        case 1:
          if (!lighBedroom_Publish.publish(state[i]))
          {
            Serial.println(F("Tải lên nút bấm đèn ngủ thất bại!"));
          }
          else
          {
            Serial.println(F("Tải lên nút bấm đèn ngủ thành công!"));
            change_button[i] = 0;
          }
          break;

          // case 2:
          //   if (!tv_Publish.publish(state[i]))
          //   {
          //     Serial.println(F("Failed to upload button tv!"));
          //     val++;
          //   }
          //   else
          //   {
          //     Serial.println(F("tv uploaded successfully!"));
          //     change_button[i] = 0;
          //   }
          //   break;

        default:
          break;
        }

        if (i == 2)
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
      }
    }
  }
  val = change_button[0] + change_button[1] + change_button[2] + change_button[3];
}

void send_temperature()
{
  // Công bố nhiệt độ đến server
  if (!Temperature1.publish(temp))
    Serial.println(F("Tải lên nhiệt độ thất bại!"));
  else
    Serial.println(F("Tải lên nhiệt độ thành công!"));
}

void send_humidity()
{
  if (!Humidity1.publish(hum))
    Serial.println(F("Tải lên độ ẩm thất bại!"));
  else
    Serial.println(F("Tải lên độ ẩm thành công!"));
}

void send_heat_index()
{
  if (!HeatIndex.publish(hic))
    Serial.println(F("Tải lên thất bại chỉ số nóng bức!"));
  else
    Serial.println(F("Tải lên thành công chỉ số nóng bức!"));
}

void receive_light()
{
  // Kiểm tra nếu là feed của đèn chiếu sáng
  if (subscription == &light)
  {
    Serial.print("Đèn chiếu sáng: ");

    if (strcmp((char *)light.lastread, "1") == 0)
    {
      digitalWrite(LIGHT_PIN, LOW);
      Serial.println(" Bật.");
    }
    else
    {
      digitalWrite(LIGHT_PIN, HIGH);
      Serial.println("Tắt.");
    }
  }
}

void receive_light_bedroom()
{
  // Kiểm tra nếu là feed của đèn ngủ
  if (subscription == &lighBedroom)
  {
    Serial.print(F("Đèn ngủ: "));

    if (strcmp((char *)lighBedroom.lastread, "1") == 0)
    {
      digitalWrite(LIGHT_BEDROOM, LOW);
      Serial.println(" Bật.");
    }
    else
    {
      digitalWrite(LIGHT_BEDROOM, HIGH);
      Serial.println("Tắt.");
    }
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
    speed = (int) sliderval;
  }
}

void receive_fan_auto()
{
  // Kiểm tra nếu là feed của tự động quạt
  if (subscription == &fanAuto)
  {
    if (strcmp((char *)fanAuto.lastread, "1") == 0)
    {
      Serial.println("Quạt tự động : Bật.");
      fanMode = 1;
    }
    else
    {
      Serial.println("Quạt tự động : Tắt.");
      fanMode = 0;
    }
  }
}

void receive_tv()
{
  // Kiểm tra nếu là feed của ti vi
  if (subscription == &tv)
  {
    Serial.print(F("Trạng thái ti vi: "));

    if (strcmp((char *)tv.lastread, "1") == 0)
    {
      digitalWrite(TV_PIN, LOW);
      Serial.println(" Bật.");
    }
    else
    {
      digitalWrite(TV_PIN, HIGH);
      Serial.println("Tắt.");
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
  }
}

void fan_auto(byte a)
{
  if (a == 1)
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
      change_button[2] = 1;
      speed = speed_auto;
      analogWrite(FAN_PIN, speed);
    }
  }
}