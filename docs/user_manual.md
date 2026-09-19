# HƯỚNG DẪN SỬ DỤNG VÀ VẬN HÀNH HỆ THỐNG (USER & OPERATION MANUAL)

> **Dự án**: ESP32-C3 Posture Monitor & Alert System  
> **Phiên bản Hướng dẫn**: `v1.1.2`  
> **Áp dụng cho**: Firmware ESP-IDF & Arduino ESP32  
> **Ngày cập nhật**: 2026-09-20  

---

## 1. HƯỚNG DẪN BẮT ĐẦU NHANH (QUICK START GUIDE)

### Bước 1: Đeo Thiết Bị Lên Cơ Thể
1. Gắn thiết bị vào dây đeo ngực hoặc dán/kẹp vào mặt sau của áo lót/áo thun.
2. **Vị trí chuẩn y khoa**: Nằm dọc theo chính giữa cột sống ngực, khoảng giữa hai bả vai (đốt sống $T3 - T5$).
3. **Chiều cảm biến**: Cổng sạc Type-C hướng xuống dưới; mặt có linh kiện và đèn LED hướng ra ngoài; trục cảm biến chạy dọc sống lưng hướng lên gáy.

```
       [ Cổ ]
         │
    ┌────┴────┐
 [Bả vai]  [Bả vai]
    └────┬────┘
     [THIẾT BỊ]  <-- Vị trí T3-T5 giữa 2 bả vai
         │
      [ Lưng ]
```

### Bước 2: Bật Nguồn & Tự Kiểm Tra (Power-On Self-Test)
* Bật công tắc nguồn pin hoặc cắm nguồn $5\,\text{V}$ qua cổng Type-C.
* **Đèn Status LED (GPIO 5)** sẽ nhấp nháy nhanh **3 lần** (chu kỳ $50\,\text{ms}$) để thông báo bộ vi điều khiển, bộ nhớ flash NVS và cảm biến MPU6050 đã vượt qua bài kiểm tra POST thành công.
* Motor rung sẽ khẽ giật nhẹ 1 nhịp $100\,\text{ms}$ báo hiệu thiết bị sẵn sàng làm việc.

---

## 2. THAO TÁC VỚI NÚT BẤM VẬT LÝ (PHYSICAL BUTTON GESTURES)

Thiết bị trang bị một nút nhấn duy nhất (gắn tại GPIO 0) hỗ trợ các thao tác trực quan:

```
[ Nút Bấm Vật Lý ] ──┬── Bấm 1 lần (Single-click)   ---> Tắt chuông/rung cảnh báo tức thì
                     ├── Bấm 2 lần (Double-click)   ---> Tạm hoãn (Snooze) trong 10 phút
                     └── Giữ 2 giây (Long-press)     ---> Hiệu chuẩn tư thế chuẩn (Tare Zero)
```

| Thao tác | Thời gian bấm | Phản hồi xúc giác / âm thanh | Ý nghĩa chức năng |
| :--- | :--- | :--- | :--- |
| **Bấm 1 lần** (Single-click) | Nhấn nhả $< 0.5\,\text{s}$ | Motor ngừng rung ngay | Tắt âm cảnh báo hiện tại nếu đang bị nhắc nhở mà chưa thể ngồi thẳng ngay. |
| **Bấm đúp** (Double-click) | 2 lần nhấn trong $350\,\text{ms}$ | Rung 2 nhịp ngắn | **Tạm hoãn 10 phút (Snooze)**: Tắt toàn bộ cảnh báo rung/còi khi người dùng đang di chuyển, đi vệ sinh hoặc tập thể dục. |
| **Nhấn giữ** (Long-press) | Nhấn giữ $\ge 1.8\,\text{s}$ | Rung 1 nhịp dài $100\,\text{ms}$ | **Hiệu chuẩn tư thế gốc (Tare Zero)**: Đặt lại mốc $0.0^\circ$ cho dáng ngồi hiện tại của người dùng. |

---

## 3. BẢNG MÃ ĐÈN LED TRẠNG THÁI (DIAGNOSTIC STATUS LED CODES)

Đèn LED gắn tại **GPIO 5** phát tín hiệu chẩn đoán trực quan giúp bạn nắm bắt trạng thái hoạt động của thiết bị ngay cả khi không mở màn hình điện thoại:

| Kiểu chớp đèn (Cadence) | Chu kỳ thời gian | Trạng thái hệ thống tương ứng |
| :--- | :--- | :--- |
| **3 nhịp chớp nhanh** | Chớp $50\,\text{ms}$ ON / $50\,\text{ms}$ OFF | Vừa khởi động: Tự kiểm tra phần cứng (POST) thành công. |
| **Nhịp tim (Heartbeat)** | Chớp sáng nhẹ $60\,\text{ms}$ mỗi $2\,\text{giây}$ | **Tư thế TỐT (Good)**: Bạn đang ngồi thẳng lưng chuẩn xác. |
| **Nhấp nháy vừa** | Chớp đều $2\,\text{Hz}$ ($250\,\text{ms}$ ON / $250\,\text{ms}$ OFF) | **Cảnh báo (Slouch Warning / Level 1)**: Đang phát hiện gù lưng. |
| **Chớp dồn dập (Strobe)**| Chớp nhanh $5\,\text{Hz}$ ($100\,\text{ms}$ ON / $100\,\text{ms}$ OFF) | **Báo động Mức 2 (Alarm Level 2)** hoặc **Đang nạp OTA**. |
| **Chớp thở chậm** | Chớp sáng $800\,\text{ms}$ ON / $800\,\text{ms}$ OFF | **Tạm hoãn (Snoozed)**: Đang trong thời gian tạm dừng theo dõi. |
| **Chớp siêu tốc** | Chớp $50\,\text{ms}$ liên tục ($10\,\text{Hz}$) | **Đang hiệu chuẩn (Calibrating)**: Đang lấy 100 mẫu tư thế gốc. |
| **Sáng mờ liên tục / Đỏ**| Sáng liên tục hoặc chớp ngắn | **Pin yếu (Low Battery $< 15\%$)** hoặc **Mất kết nối cảm biến**. |

---

## 4. KẾT NỐI VÀ SỬ DỤNG TRANG WEB MONITOR (100% OFFLINE)

Thiết bị tự phát Wi-Fi độc lập, cho phép xem trực tiếp tư thế trên điện thoại iPhone, Android hoặc máy tính bảng mà không cần kết nối internet hay cài đặt ứng dụng phức tạp.

```
+-------------------------------------------------------------------------------+
| BƯỚC 1: Bật Wi-Fi trên điện thoại, tìm và kết nối mạng:                      |
|         Tên Wi-Fi (SSID): Posture-Monitor-AP                                  |
|         Mật khẩu:         12345678                                            |
+-------------------------------------------------------------------------------+
                                         │
                                         ▼
+-------------------------------------------------------------------------------+
| BƯỚC 2: Mở trình duyệt web (Safari, Chrome) và truy cập địa chỉ:              |
|         http://192.168.4.1                                                    |
+-------------------------------------------------------------------------------+
```

### Các Tính Năng Nổi Bật Trên Web Monitor:
1. **Khung xương 3D Isometric Mini (Three.js Lite)**:
   * Hiển thị mô phỏng động học cột sống 3D theo thời gian thực (10Hz).
   * **Xoay góc nhìn 3D**: Dùng ngón tay vuốt trên màn hình để xoay quanh cơ thể $360^\circ$ hoặc nhìn từ trên xuống ($15^\circ - 85^\circ$).
   * **Thu phóng**: Chụm/mở 2 ngón tay (Pinch to Zoom) hoặc lăn chuột.
   * **Nút bấm `3D`**: Nhấn để đưa camera về góc nhìn xiên chuẩn (Isometric $45^\circ$).
   * **Cục cảm biến & Đèn LED động**: Phát sáng màu Xanh lá (Tốt), Cam (Nghi vấn gù), Đỏ nhấp nháy (Báo động) hoặc Xanh dương (Hiệu chuẩn).
2. **Bảng đo góc trực tiếp (Live Orientation)**:
   * **Roll (Cúi/Ngửa)**: Thể hiện độ gập người về phía trước ($0.0^\circ$ khi ngồi thẳng, tăng dương khi cúi).
   * **Pitch (Nghiêng)**: Thể hiện độ nghiêng vai sang trái/phải.
   * **Yaw (Xoay)**: Thể hiện góc vặn trục người quanh cột sống.
   * **Tổng độ lệch (Total Deviation)**: Độ lệch tổng hợp $\sqrt{\Delta r^2 + \Delta p^2}$ so với ngưỡng cho phép.
3. **Biểu đồ thời gian thực (Real-time Canvas Chart)**:
   * Vẽ liên tục đường cong hoạt động cột sống trong 30 giây gần nhất kèm đường ranh giới ngưỡng cảnh báo màu đỏ.
4. **Tab Phân tích & Lịch sử (Analytics & History)**:
   * Bảng nhiệt (Heatmap) 24 giờ chất lượng tư thế trong ngày.
   * Tỷ lệ phần trăm thời gian ngồi chuẩn, thời gian gù lưng và chuỗi kỷ lục ngồi thẳng tốt nhất.
   * Dòng sự kiện (Timeline) ghi nhận chính xác thời điểm bắt đầu sai tư thế.
   * Nút bấm **Xuất CSV (Export CSV)** lưu báo cáo trực tiếp về máy.

---

## 5. QUY TRÌNH HIỆU CHUẨN TƯ THẾ GỐC (TARE ZERO-CALIBRATION)

Hiệu chuẩn tư thế chuẩn y khoa là bước quan trọng nhất để hệ thống nhận diện đúng dáng vóc của từng cá nhân:

```
          TƯ THẾ HIỆU CHUẨN CHUẨN Y KHOA
          
    1. Lưng tựa thẳng nhẹ vào ghế, cột sống vươn tự nhiên.
    2. Hai vai thả lỏng, cân bằng đều 2 bên (không gồng cứng).
    3. Cằm song song với mặt đất, mắt nhìn thẳng phía trước.
    4. Hai bàn chân đặt phẳng trên mặt sàn nhà.
```

### Các Bước Thực Hiện:
1. Ngồi vào tư thế chuẩn nêu trên.
2. **Cách 1 (Qua nút bấm)**: Nhấn và giữ nút bấm trên thiết bị trong **2 giây** cho đến khi thiết bị rung nhẹ 1 tiếng bíp ngắn.
   **Cách 2 (Qua Web)**: Bấm nút **"Hiệu chuẩn 0" (Tare Calibrate)** trên giao diện web.
3. **Giữ bất động hoàn toàn trong 3 giây**: Đèn LED sẽ chớp cực nhanh ($10\,\text{Hz}$). Thiết bị lấy trung bình 100 mẫu đo IMU để triệt tiêu dao động cơ học.
4. Khi hoàn thành, motor sẽ rung 2 nhịp ngắn xác nhận: Mốc tọa độ $0.0^\circ$ mới đã được ghi bền vững vào bộ nhớ Flash NVS.

---

## 6. HƯỚNG DẪN CẬP NHẬT FIRMWARE KHÔNG DÂY (OTA UPDATE)

Từ phiên bản `v1.1.0` trở đi, bạn hoàn toàn có thể cập nhật các bản nâng cấp phần mềm mới trực tiếp qua giao diện web mà không cần cắm cáp nạp USB:

```
[ Máy tính / Điện thoại ]
        │
        │ Tải file 'posture-monitor.bin' mới
        ▼
   Web Monitor (http://192.168.4.1 -> Tab "Thiết bị")
        │
        │ Kéo thả file .bin vào khung "Cập nhật Firmware"
        │ Bấm nút "Tải lên & Nạp Firmware"
        ▼
[ ESP32-C3 Flash Memory ]
        │
        │ Ghi trực tiếp vào phân vùng thụ động 'ota_1'
        │ Xác thực SHA-256 & Kiểm tra Magic Byte 0xE9
        ▼
[ Tự động khởi động lại sau 1.5 giây sang phiên bản mới ]
```

> [!IMPORTANT]
> **Lưu Ý Khi Nạp OTA**:
> 1. Mức pin phải còn trên **20%** (hệ thống sẽ từ chối nạp nếu pin yếu dưới 20% để bảo vệ an toàn).
> 2. Tuyệt đối không tắt nguồn hoặc ngắt Wi-Fi trong khoảng 15 giây thiết bị đang nạp.
> 3. Sau khi màn hình đếm ngược hoàn tất, thiết bị tự khởi động lại và kết nối tự động phục hồi với số hiệu phiên bản mới (ví dụ `v1.1.2`).

---

## 7. XỬ LÝ SỰ CỐ THƯỜNG GẶP (TROUBLESHOOTING GUIDE)

| Hiện tượng | Nguyên nhân có thể | Cách khắc phục |
| :--- | :--- | :--- |
| **Đèn LED sáng đứng yên hoặc báo "SENSOR OFFLINE" trên Web** | Dây nối I2C giữa ESP32 và MPU6050 bị lỏng hoặc chập đường SDA/SCL | Kiểm tra lại 4 dây nối: VCC ($3.3\,\text{V}$), GND, SDA (GPIO 8), SCL (GPIO 9). Đảm bảo có điện trở kéo $4.7\,\text{k}\Omega$ lên $3.3\,\text{V}$. |
| **Không tìm thấy Wi-Fi `Posture-Monitor-AP`** | Thiết bị đang tắt nguồn hoặc pin đã cạn kiệt | Cắm sạc Type-C vào thiết bị, kiểm tra đèn báo sạc đỏ trên mạch sạc TP4056. |
| **Góc Roll bị lệch hoặc mô hình 3D đứng không thẳng** | Chưa hiệu chuẩn tư thế gốc cho vóc dáng của bạn | Ngồi thẳng lưng chuẩn y khoa và nhấn giữ nút bấm 2 giây để thực hiện Tare Calibrate. |
| **Motor không rung khi có cảnh báo** | Tùy chọn "Động cơ rung" đang bị tắt trong cài đặt hoặc mối hàn motor bị đứt | Mở Tab "Thiết bị" trên web, kiểm tra công tắc "Động cơ rung" đã bật màu xanh hay chưa. Kiểm tra diode 1N4148 và MOSFET AO3400. |
