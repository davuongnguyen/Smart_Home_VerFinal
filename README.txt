Mô tả:
	Dự án này mô phỏng một cách đơn giản hoạt động của một Smart home, với các thiết bị như: đèn chiếu sáng, đèn ngủ, ti vi, quạt ...
	Vi xử lý chính của dự án là ESP 8266, các thiết bị được thay thế bằng led và động cơ.
	
	Dự án sử dụng dịch vụ MQTT Adafruit IO: https://io.adafruit.com/

	Về vấn đề điều khiển và theo dõi dữ liệu, có thể điều khiển qua các nút nhấn, qua giọng nói (Google assistants - trung gian bởi IFTTT) hoặc trên trang web của Adafruit IO.

Phần cứng:
	- ESP8266 NodeMCU
	- cảm biến DHT 11 
	- Led, điện trở, nút bấm, L298N

Phần mềm:
	Mã nguồn viết bằng Visual Studio Code kết hợp với tiện ích PlatformIO, biên dịch trên Framework Arduino.
	Link download:
		https://code.visualstudio.com/download
		https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide&ssr=false#review-details
		https://www.arduino.cc/en/software

	Thư viện cần cài thêm:
	- Adafruit MQTT Library  by Adafruit:	https://github.com/adafruit/Adafruit_MQTT_Library.git
	- DHT sensor library by Adafruit:	https://github.com/adafruit/DHT-sensor-library.git
	- Adafruit Unified Sensor by Adafruit:	https://github.com/adafruit/Adafruit_Sensor.git
	

	
Lưu ý: 
	- Nối led ở vị trí tích cực mức thấp.
	- Về google assistants, có thể cài đặt trực tiếp trên hệ điều hành Windows hoặc MacOS: https://github.com/Melvin-Abraham/Google-Assistant-Unofficial-Desktop-Client

Các lỗi còn tồn đọng:
	....
