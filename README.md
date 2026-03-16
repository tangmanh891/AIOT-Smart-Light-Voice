# AIOT Smart Light Voice Control

Hệ thống điều khiển đèn bằng giọng nói tiếng Việt với ESP32 + INMP441 + Edge Impulse + SinricPro.

## Demo video

<video src="./demo.mp4" controls width="720">
  Trình duyệt không hỗ trợ video. Mở trực tiếp tại <a href="./demo.mp4">demo.mp4</a>.
</video>

Nếu GitHub không hiển thị player, mở link: [Xem demo](./demo.mp4).

## Cấu trúc dự án

```text
.
├── esp32_microphone.ino
├── src/
│   ├── audio_capture.*
│   ├── inference_controller.*
│   ├── iot_controller.*
│   ├── button_controller.*
│   ├── light_controller.*
│   ├── config.h
│   └── credentials.h
├── lib/
│   └── ESP32_INMP441_inferencing/   # Edge Impulse exported library (project specific)
├── platformio.ini
├── secrets.example.h
├── ROADMAP.md
└── .github/
    ├── workflows/ci.yml
    └── ISSUE_TEMPLATE/
```

## Dependency management

Không commit thư viện dạng `.zip` vào repo.

### Phiên bản thư viện đang dùng
- ArduinoJson `7.4.2` - [Release](https://github.com/bblanchon/ArduinoJson/releases)
- SinricPro `3.5.2` - [Release](https://github.com/sinricpro/esp8266-esp32-sdk/releases)
- WebSockets `2.7.1` - [Release](https://github.com/Links2004/arduinoWebSockets/releases)
- ESP32_INMP441_inferencing `1.0.10` - export từ Edge Impulse cho project này

### Cách cài bằng PlatformIO (khuyến nghị)
1. Cài PlatformIO extension hoặc CLI.
2. Mở project và build theo `platformio.ini`.
3. Thư viện public sẽ tự cài theo `lib_deps`.
4. Model Edge Impulse dùng từ thư mục `lib/ESP32_INMP441_inferencing`.

## Bảo mật credentials

Không sửa trực tiếp SSID/password/API key trong source.

1. Copy file mẫu:
   ```bash
   cp secrets.example.h secrets.h
   ```
   Trên Windows PowerShell:
   ```powershell
   Copy-Item secrets.example.h secrets.h
   ```
2. Điền giá trị thật vào `secrets.h`.
3. `secrets.h` đã nằm trong `.gitignore`, không commit.

## Build nhanh

### PlatformIO
```bash
pio run
```

### Arduino IDE
1. Cài board `esp32 by Espressif Systems`.
2. Cài library đúng version ở trên bằng Library Manager.
3. Mở `esp32_microphone.ino` và upload.

## CI/CD và quy trình

Repo đã có GitHub Actions tại `.github/workflows/ci.yml` gồm:
- Arduino lint
- Compile check cho ESP32

Repo đã thêm issue templates:
- Bug report
- Feature request
- Chore / maintenance

## Roadmap

Xem chi tiết tại [ROADMAP.md](ROADMAP.md).

