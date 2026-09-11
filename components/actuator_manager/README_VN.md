# Thành Phần Actuator Manager (Điều Khiển Cơ Cấu Chấp Hành Phi Nghẽn)

## 1. Tổng Quan
Thành phần `actuator_manager` phụ trách điều phối nhịp xung rung của Mini Vibration Motor và tín hiệu âm thanh của Buzzer trên ESP32-C3. Module được thiết kế hoàn toàn bất đồng bộ (non-blocking) dựa trên `esp_timer`, đảm bảo không gây trễ hoặc rung lắc chu kỳ lấy mẫu góc của cảm biến IMU.

## 2. Tính Năng Kỹ Thuật
- **Bộ Định Thời Độ Chính Xác Cao (100ms Base Tick)**: Sử dụng `esp_timer` phần cứng chạy nền.
- **Mẫu Cảnh Báo Lũy Tiến**:
  - `ALERT_PATTERN_IDLE`: Tắt toàn bộ actuator, bảo tồn năng lượng pin.
  - `ALERT_PATTERN_CALIB_START`: 1 nhịp rung 200ms báo bắt đầu cân chỉnh.
  - `ALERT_PATTERN_CALIB_DONE`: 2 nhịp rung 150ms báo cân chỉnh thành công.
  - `ALERT_PATTERN_LEVEL1_HAPTIC`: Rung nhẹ ngắt quãng 200ms mỗi 1.2 giây (không còi, phù hợp văn phòng).
  - `ALERT_PATTERN_LEVEL2_ALARM`: Rung mạnh kết hợp còi kêu đồng nhịp chu kỳ 500ms khi người dùng phớt lờ cảnh báo.
- **Tín Hiệu Khóa Nhiễu Cơ Khí**: Cung cấp hàm `actuator_manager_is_vibrating()` giúp bộ lọc IMU nhận diện motor đang rung để tạm ngừng cập nhật gia tốc, tránh báo động giả do rung lắc nội bộ.

## 3. Yêu Cầu Phần Cứng & Mạch Điện
- **Mini Vibration Motor**: Tuyệt đối **không nối trực tiếp GPIO**. Phải dùng mạch đệm MOSFET kênh N (AO3400 / 2N7002), lắp kèm diode Schottky (1N5819) song song ngược cực motor để dập xung điện cảm Back-EMF và tụ lọc $10\mu\text{F} \parallel 100\text{nF}$.
- **Buzzer**: Kích qua transistor NPN (S8050) với điện trở hạn dòng chân Base $1\text{k}\Omega$.

## 4. Giao Diện Lập Trình (API)
- `actuator_manager_init()`: Khởi tạo chân GPIO và start timer định thời.
- `actuator_manager_set_pattern()`: Chuyển đổi trạng thái cảnh báo phi nghẽn.
- `actuator_manager_is_vibrating()`: Truy vấn trạng thái thực tế của motor.
