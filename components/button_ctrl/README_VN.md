# Thành Phần Button Control (Xử Lý Tương Tác Nút Bấm Đa Năng)

## 1. Tổng Quan
Thành phần `button_ctrl` đóng gói thư viện chính thức `espressif/button` từ ESP Component Registry, loại bỏ hiện tượng dội phím cơ học và giải mã các cử chỉ bấm của người dùng để điều khiển thiết bị giám sát tư thế.

## 2. Các Cử Chỉ Hỗ Trợ
- **Nhấn Giữ Lâu (> 2.0 giây)**: Phát sinh sự kiện `BUTTON_EVENT_TARE_CALIBRATE` để tiến hành đo mẫu và lưu vị trí ngồi chuẩn mới vào NVS (Tare mode).
- **Nhấn Đúp (Double Click)**: Phát sinh sự kiện `BUTTON_EVENT_SNOOZE_10MIN` để tạm dừng cảnh báo trong 10 phút khi người dùng muốn nằm nghỉ hoặc vươn vai.
- **Nhấn Đơn (Single Click)**: Tắt nhanh cảnh báo đang kích hoạt tức thời.

## 3. Cấu Hình Kconfig
- `CONFIG_POSTURE_BUTTON_GPIO`: Chân GPIO kết nối nút nhấn (mặc định: GPIO 9, tích cực mức THẤP kèm điện trở kéo lên nội).

## 4. Giao Diện Lập Trình (API)
- `button_ctrl_init()`: Khởi tạo driver nút nhấn và đăng ký hàm callback nhận sự kiện.
