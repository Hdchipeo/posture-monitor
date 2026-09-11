# Thành Phần MPU6050 Sensor (Thu Nhận & Lọc Tín Hiệu Góc Cột Sống)

## 1. Tổng Quan
Thành phần `mpu6050_sensor` đóng gói và mở rộng thư viện chính thức `espressif/mpu6050` từ ESP Component Registry, cung cấp tầng giao tiếp phần cứng I2C tin cậy và bộ lọc tư thế tối ưu cho các thiết bị đeo trên người.

## 2. Điểm Nhấn Kỹ Thuật
- **Khôi Phục Treo Bus I2C (I2C Bus Recovery)**: Tự động phát hiện đường SDA bị kéo giữ mức LOW (do mất điện giữa chu kỳ truyền I2C trước đó) và phát 9 xung clock trên SCL kèm tín hiệu STOP để giải phóng bus trước khi gắn driver.
- **Khử Nhiễu Cơ Khí Khi Motor Rung (Actuator Decoupling)**: Khi motor rung cảnh báo hoạt động, thành phần tạm khóa cập nhật vector gia tốc trọng trường (`freeze_accel_bias = true`) và chỉ tích phân vận tốc góc từ con quay hồi chuyển, loại bỏ 100% hiện tượng rung phản hồi làm sai lệch góc đo.
- **Bộ Lọc Bù (Complementary Filter)**: Triệt tiêu hiện tượng trôi góc (drift) của Gyroscope dài hạn và triệt tiêu rung chấn ngắn hạn của Accelerometer với hệ số $\alpha = 0.96$ cấu hình được trong menuconfig.

## 3. Công Thức Tính Góc
$$\text{Pitch}_{\text{acc}} = \text{atan2}\left(a_y, \sqrt{a_x^2 + a_z^2}\right) \times \frac{180}{\pi}$$

$$\text{Roll}_{\text{acc}} = \text{atan2}\left(-a_x, a_z\right) \times \frac{180}{\pi}$$

## 4. Tham Số Cấu Hình Menuconfig
- `CONFIG_POSTURE_I2C_SDA_GPIO`: Chân SDA (mặc định: 4).
- `CONFIG_POSTURE_I2C_SCL_GPIO`: Chân SCL (mặc định: 5).
- `CONFIG_POSTURE_I2C_FREQ_HZ`: Tần số xung clock I2C (mặc định: 400kHz).
- `CONFIG_POSTURE_FILTER_ALPHA_X100`: Trọng số tích phân Gyro $\alpha \times 100$ (mặc định: 96 $\rightarrow$ 0.96).

## 5. Giao Diện Lập Trình (API)
- `mpu6050_sensor_init()`: Khôi phục bus, đánh thức cảm biến, cấu hình thang đo $\pm 2g$ và $\pm 250^\circ/\text{s}$.
- `mpu6050_sensor_update()`: Đọc dữ liệu thô và cập nhật góc Pitch/Roll theo thời gian thực.
- `mpu6050_sensor_sleep()`: Đưa cảm biến về trạng thái ngủ tiết kiệm pin.
