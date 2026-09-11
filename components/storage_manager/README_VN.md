# Thành Phần Storage Manager (Quản Lý Lưu Trữ NVS & Toàn Vẹn CRC32)

## 1. Tổng Quan
Thành phần `storage_manager` chịu trách nhiệm lưu trữ và phục hồi bền vững các tham số hiệu chuẩn tư thế (góc lệch chuẩn `pitch_offset`, `roll_offset`) và các ngưỡng cảnh báo vào bộ nhớ Flash NVS của ESP32-C3.

## 2. Tính Năng Kỹ Thuật
- **Kiểm Tra Tính Toàn Vẹn Bằng CRC32**: Đảm bảo phát hiện và loại bỏ các dữ liệu hỏng do sập nguồn hoặc cạn pin đột ngột trong lúc ghi flash.
- **Xác Thực Magic Word (`0x504F5354` - "POST")**: Tránh đọc phải các vùng nhớ chưa được khởi tạo.
- **Cơ Chế Phục Hồi Tự Động (Fallback)**: Khi phát hiện CRC sai hoặc dữ liệu lỗi, hệ thống tự động ghi đè bộ tham số mặc định từ Kconfig, tránh làm crash hệ thống.

## 3. Cấu Trúc Dữ Liệu Lưu Trữ
```c
typedef struct {
    uint32_t magic;               // Định danh cấu trúc ("POST")
    float    pitch_offset;        // Góc lệch Pitch tư thế chuẩn (độ)
    float    roll_offset;         // Góc lệch Roll tư thế chuẩn (độ)
    float    angle_threshold;     // Ngưỡng góc lệch cho phép (độ)
    uint32_t slouch_delay_s;      // Thời gian trễ trước khi rung Level 1
    uint32_t escalation_delay_s;  // Thời gian trễ trước khi còi Level 2
    uint32_t crc32;               // Mã kiểm dư tuần hoàn CRC32
} __attribute__((packed)) posture_calib_data_t;
```

## 4. Giao Diện Lập Trình (API)
- `storage_manager_init()`: Khởi tạo phân vùng NVS.
- `storage_manager_load_calibration()`: Tải và xác thực dữ liệu từ NVS.
- `storage_manager_save_calibration()`: Tính toán CRC32 và ghi dữ liệu xuống NVS.
- `storage_manager_reset_to_default()`: Khôi phục cấu hình về mặc định nhà máy.

## 5. Đánh Giá Tài Nguyên & Bộ Nhớ
- **Heap Usage**: Hoàn toàn không cấp phát động (`malloc`), mọi thao tác thực thi trên Stack (~64 bytes).
- **Flash Overhead**: Chiếm 1 blob duy nhất trong namespace `posture_cfg`.
