# Hướng Dẫn Sử Dụng Phiên Bản Arduino Cho Posture Monitor (ESP32 & ESP32-C3)

## 1. Tổng Quan
Thư mục này chứa toàn bộ mã nguồn phiên bản Arduino cho hệ thống Giám sát & Cảnh báo tư thế ngồi. Mã nguồn được thiết kế chạy trực tiếp trên **Arduino IDE** hoặc **PlatformIO** mà **không cần cài đặt bất kỳ thư viện ngoài nào** (toàn bộ giao tiếp I2C MPU6050 được xử lý trực tiếp qua `Wire.h`).

## 2. Hỗ Trợ Phần Cứng & Tự Động Gán Chân
Hệ thống tự động nhận diện loại chip khi chọn board trong Arduino IDE:

| Tín Hiệu | ESP32 Thường (Xtensa Dual-Core) | ESP32-C3 (RISC-V) | Ghi Chú |
| :--- | :--- | :--- | :--- |
| **I2C SDA** | GPIO 21 | GPIO 8 (hoặc 4) | Nối chân SDA của MPU6050 |
| **I2C SCL** | GPIO 22 | GPIO 9 (hoặc 5) | Nối chân SCL của MPU6050 |
| **Motor Rung** | GPIO 18 | GPIO 6 | Kích cổng G MOSFET kênh N (AO3400) |
| **Còi Chip** | GPIO 19 | GPIO 7 | Kích transistor NPN (S8050) |
| **Nút Nhấn** | GPIO 0 (BOOT) | GPIO 0 (hoặc 9) | Tích cực mức THẤP (Active LOW) |

> [!WARNING]
> Trên ESP32 thường, các chân GPIO 6 đến 11 được gắn cứng với bộ nhớ SPI Flash nội. Mã nguồn đã có sẵn lệnh kiểm tra khóa an toàn tầng biên dịch để ngăn ngừa chạm chân Flash gây sập chip.

## 3. Các Bước Nạp Bằng Arduino IDE

### 3.1 Cài Đặt Gói Bo Mạch ESP32
1. Mở Arduino IDE (khuyến nghị phiên bản 2.x).
2. Vào **File -> Preferences**, dán đường dẫn sau vào ô **Additional Boards Manager URLs**:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Vào **Tools -> Board -> Boards Manager**, tìm kiếm `esp32` và bấm **Install**.

### 3.2 Mở Và Nạp Chương Trình
1. Mở tệp `arduino/posture_monitor/posture_monitor.ino` trong Arduino IDE.
2. Vào mục **Tools -> Board** để chọn board phù hợp:
   - Nếu dùng ESP32 2 nhân thông thường: Chọn **ESP32 Dev Module**.
   - Nếu dùng ESP32-C3: Chọn **ESP32C3 Dev Module**.
3. Chọn cổng COM/Serial tại **Tools -> Port**.
4. Nhấn nút **Upload** (Ctrl+U / Cmd+U) để nạp.
5. Mở **Serial Monitor** ở tốc độ **115200 baud** để xem dữ liệu góc nghiêng, độ lệch Euclidean, điểm tư thế (0-100) và trạng thái.

## 4. Hướng Dẫn Thao Tác Nút Bấm
- **Nhấn giữ > 2 giây**: Thiết bị rung 1 nhịp bắt đầu cân chỉnh tư thế ngồi chuẩn (Tare). Ngồi thẳng lưng trong 2 giây, thiết bị rung 2 nhịp xác nhận hoàn tất và lưu vào Flash NVS.
- **Nhấn đúp (2 lần liên tiếp)**: Tạm dừng cảnh báo trong 10 phút (Snooze) khi cần đứng dậy hoặc tập thể dục.
- **Nhấn đơn (1 lần)**: Tắt nhanh cảnh báo đang rung/kêu.

## 5. Tính Năng Chống Lỗi Tự Động (Fault Tolerance)
- **Tự động un-wedge I2C bus**: Khi dây SDA/SCL bị nhiễu hoặc ngắt kết nối, driver phát 9 xung xung nhịp SCL để mở khóa bus I2C.
- **Khởi động không chặn (Non-blocking Boot)**: Nếu cảm biến chưa cắm lúc bật nguồn, hệ thống không bị treo mà liên tục thử kết nối lại ngầm mỗi 2 giây.
- **Tự động ngắt cơ cấu chấp hành**: Khi cảm biến ngoại tuyến, motor rung và còi chip được tự động ngắt để bảo vệ an toàn.
