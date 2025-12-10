# 🎙️ AIOT Smart Light Voice Control

Hệ thống điều khiển đèn thông minh bằng giọng nói sử dụng ESP32, INMP441 Microphone và Edge Impulse AI.

## 📋 Tổng quan

Dự án này cho phép điều khiển đèn LED thông qua **3 phương thức**:
- 🗣️ **Giọng nói** - Nhận dạng lệnh tiếng Việt bằng AI
- 📱 **Ứng dụng điện thoại** - Điều khiển từ xa qua SinricPro (tương thích Alexa/Google Home)
- 🔘 **Nút nhấn vật lý** - Điều khiển trực tiếp

## 🛠️ Phần cứng

### Linh kiện cần thiết:
- **ESP32 DevKit** (bất kỳ model nào)
- **INMP441 I2S Microphone**
- **LED** (hoặc relay module)
- **Nút nhấn**
- **Breadboard và dây jumper**

### Sơ đồ kết nối:

```
ESP32          INMP441
-----          -------
GPIO 25   -->  WS (Word Select)
GPIO 22   -->  SD (Serial Data)
GPIO 26   -->  SCK (Serial Clock)
GND       -->  GND
3.3V      -->  VDD
GND       -->  L/R (để chọn kênh Left)

ESP32          LED/Relay
-----          ---------
GPIO 5    -->  IN 
GND       -->  GND

ESP32          Button
-----          ------
GPIO 18   -->  Button (pull-up internal)
GND       -->  Button (chân còn lại)
```

## 📦 Cài đặt

### Bước 1: Cài đặt Arduino IDE

1. Tải Arduino IDE từ [arduino.cc](https://www.arduino.cc/en/software)
2. Thêm ESP32 Board Manager:
   - Mở **File → Preferences**
   - Thêm URL vào **Additional Board Manager URLs**:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Vào **Tools → Board → Boards Manager**
   - Tìm "ESP32" và cài đặt **esp32 by Espressif Systems**

### Bước 2: Cài đặt thư viện

**Thêm các file .zip vào Arduino IDE**
1. Mở Arduino IDE
2. **Sketch → Include Library → Add .ZIP Library..**
3. Thêm các thư viện vào Arduino IDE



### Bước 3: Cấu hình WiFi và SinricPro

Mở file `esp32_microphone.ino` và chỉnh sửa:

```cpp
// Thông tin WiFi của bạn
const char* WIFI_SSID = "TenWiFi";      // Thay bằng tên WiFi
const char* WIFI_PASS = "MatKhauWiFi";  // Thay bằng mật khẩu WiFi

// Thông tin SinricPro (đăng ký miễn phí tại sinric.pro)
const char* APP_KEY    = "your-app-key";
const char* APP_SECRET = "your-app-secret";
const char* DEVICE_ID  = "your-device-id";
```

#### Cách lấy SinricPro credentials:
1. Đăng ký tài khoản tại [sinric.pro](https://sinric.pro)
2. Tạo device mới (chọn loại **Switch**)
3. Copy **App Key**, **App Secret**, và **Device ID**
4. Paste vào code

### Bước 4: Upload code

1. Kết nối ESP32 với máy tính qua USB
2. Chọn board: **Tools → Board → ESP32 Arduino → ESP32 Dev Module**
3. Chọn port: **Tools → Port → COMx** (port của ESP32)
4. Cấu hình:
   - **Upload Speed**: 921600
   - **Flash Frequency**: 80MHz
5. Click **Upload** (hoặc Ctrl+U)

## 🎤 Lệnh giọng nói

Model AI đã được train với các lệnh tiếng Việt:

| Lệnh | Chức năng |
|------|-----------|
| **"Hello đèn"** | Wake word - Đánh thức hệ thống |
| **"Bật đèn"** | Bật đèn LED |
| **"Tắt đèn"** | Tắt đèn LED |
| **"Im lặng"** | Reset về chế độ chờ |

### Quy trình sử dụng:
1. Nói **"Hello đèn"** → LED nhấp nháy xác nhận
2. Nói **"Bật đèn"** hoặc **"Tắt đèn"**
3. Hệ thống tự động quay về chế độ chờ sau 10 lần phát hiện "im lặng"

## 📱 Điều khiển qua App

### Cách sử dụng SinricPro App:
1. Tải app **SinricPro** (iOS/Android)
2. Đăng nhập tài khoản
3. Device sẽ tự động xuất hiện
4. Bật/tắt đèn từ bất kỳ đâu có internet

### Tích hợp với Alexa/Google Home:
1. Trong app SinricPro, vào **Settings → Smart Home**
2. Link account với Alexa/Google Home
3. Phát hiện thiết bị mới
4. Điều khiển 

## 🚀 Tính năng nâng cao

### Có thể mở rộng:
- ✨ Điều khiển nhiều thiết bị (quạt, TV, rèm cửa)
- 🌡️ Thêm cảm biến nhiệt độ/độ ẩm
- 🔔 Thông báo qua Telegram/Discord
- 📊 Dashboard web real-time
- 🤖 Automation rules (bật đèn khi tối)

