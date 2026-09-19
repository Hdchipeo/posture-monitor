# SƠ ĐỒ PHẦN CỨNG VÀ KẾT NỐI MẠCH (HARDWARE SCHEMATIC & WIRING GUIDE)

> **Dự án**: ESP32-C3 Posture Monitor & Alert System  
> **Phiên bản Phần cứng**: `Rev 1.2`  
> **Vi điều khiển chính**: ESP32-C3-WROOM-02 / ESP32-C3 SuperMini (RISC-V 160MHz)  
> **Cảm biến IMU**: InvenSense MPU6050 (6-DOF)  
> **Ngày cập nhật**: 2026-09-20  

---

## 1. SƠ ĐỒ KHỐI TỔNG THỂ (SYSTEM BLOCK DIAGRAM)

```
                       +-------------------------------+
                       |    Nguồn Pin LiPo 3.7V-4.2V   |
                       +---------------+---------------+
                                       │
                    ┌──────────────────┴──────────────────┐
                    ▼                                     ▼
        +-----------------------+             +-----------------------+
        |  TP4056 Sạc Type-C   |             | Mạch phân áp đo Pin   |
        |  & LDO 3.3V (ME6211)  |             | R1=100kΩ, R2=100kΩ    |
        +-----------+-----------+             +-----------+-----------+
                    │ VCC 3.3V                            │ V_DIV
                    ▼                                     ▼
+-----------------------------------------------------------------------------------+
|                        VI ĐIỀU KHIỂN ESP32-C3 (RISC-V)                            |
|                                                                                   |
|  [GPIO 8] I2C SDA  ◄───────────────────────────► [SDA] Cảm biến MPU6050         |
|  [GPIO 9] I2C SCL  ────────────────────────────► [SCL] (I2C Bus 400kHz)           |
|                                                                                   |
|  [GPIO 6] PWM Gate ────────────────────────────► MOSFET AO3400 -> Động cơ rung    |
|  [GPIO 7] Buzzer   ────────────────────────────► BJT S8050    -> Còi báo động     |
|  [GPIO 5] Status   ────────────────────────────► Điện trở 330Ω -> Đèn LED Đa sắc  |
|  [GPIO 1] ADC1_CH1 ◄───────────────────────────┘ Điện áp pin chia đôi             |
|  [GPIO 0] Boot Key ◄─────────────────────────── Nút bấm vật lý (Tare / Snooze)   |
+-----------------------------------------------------------------------------------+
```

---

## 2. BẢNG PHÂN BỔ CHÂN GPIO (PINOUT MAPPING)

Hệ thống hỗ trợ cả dòng vi điều khiển **ESP32-C3 (RISC-V)** và **ESP32 Classic (Xtensa Dual-Core)** với cơ chế kiểm tra an toàn biên dịch ngăn chặn xung đột chân SPI Flash:

| Tên tín hiệu | Chân ESP32-C3 | Chân ESP32 Classic | Hướng I/O | Chức năng chi tiết & Yêu cầu phần cứng |
| :--- | :--- | :--- | :--- | :--- |
| **`I2C_SDA`** | **GPIO 8** | GPIO 21 | Bi-directional | Dữ liệu nối tiếp I2C. Điện trở kéo ngoài $4.7\,\text{k}\Omega$ lên $3.3\,\text{V}$. |
| **`I2C_SCL`** | **GPIO 9** | GPIO 22 | Output | Xung nhịp I2C (400kHz). Điện trở kéo ngoài $4.7\,\text{k}\Omega$ lên $3.3\,\text{V}$. |
| **`VIB_MOTOR`**| **GPIO 6** | GPIO 18 | Output | Điều khiển Gate N-MOSFET (AO3400) lái motor rung mini ERM. |
| **`BUZZER`**   | **GPIO 7** | GPIO 19 | Output | Điều khiển Base BJT (S8050) lái còi chip tích cực (Active Buzzer). |
| **`STATUS_LED`**| **GPIO 5**| GPIO 2  | Output | Đèn LED báo trạng thái chẩn đoán hệ thống (nối tiếp $330\,\Omega$). |
| **`BATTERY_ADC`**|**GPIO 1** | GPIO 34 | Analog Input | Kênh ADC1_CH1 đọc điện áp pin qua cầu chia $100\,\text{k}\Omega / 100\,\text{k}\Omega$. |
| **`USER_BUTTON`**|**GPIO 0**| GPIO 0  | Digital Input | Nút bấm vật lý đa năng (Tích cực mức THẤP - Active LOW, có tụ $100\,\text{nF}$ chống nảy). |

> [!WARNING]
> **Quy Tắc An Toàn Phần Cứng ESP32 Classic**:
> Trên dòng ESP32 Classic, tuyệt đối **không được dùng GPIO 6 đến 11**. Các chân này kết nối trực tiếp với bộ nhớ SPI Flash nội bên trong chip; việc kéo thả tín hiệu vào các chân này sẽ gây sập chip lập tức (`Core Panic / Cache Error`).

---

## 3. SƠ ĐỒ NGUYÊN LÝ CHI TIẾT (SCHEMATIC DETAILS)

### A. Mạch Giao Tiếp Cảm Biến MPU6050 (I2C Bus)
```
          +3.3V
            │
      ┌─────┴─────┐
     [4.7k]      [4.7k]  (Pull-up Resistors)
      │           │
      ├───────────┼─────────── GPIO 8 (ESP32-C3 SDA)
      │           │
      │   ┌───────┼─────────── GPIO 9 (ESP32-C3 SCL)
      │   │       │
  ┌───┴───┴───────┴───┐
  │  VCC SCL SDA GND  │
  │     MPU-6050      │
  │  AD0 INT  NC  NC  │
  └───┬───────────────┘
      │
     GND (AD0 nối GND để cố định địa chỉ I2C = 0x68)
```
* **Tụ Lọc Nhiễu**: Đặt tụ gốm $0.1\,\mu\text{F}$ (104) song song tụ hóa $10\,\mu\text{F}$ sát chân VCC của module MPU6050 để triệt tiêu nhiễu tần số cao từ nguồn switching.

---

### B. Mạch Điều Khiển Động Cơ Rung Xúc Giác (Haptic ERM Motor)
Động cơ rung là tải cảm ứng (inductive load), khi đóng cắt đột ngột sinh ra điện áp ngược rất lớn (Back-EMF spike). Mạch bắt buộc phải có **Diode dập xung (Flyback Diode)** để bảo vệ transistor lái.

```
       VCC_BAT (3.7V - 4.2V)
            │
            ├─────────────────────┐
            │                     │
        [ + Motor - ]           [1N4148]  (Diode dập xung ngược Flyback)
            │      Cathode (vạch) │
            ├─────────────────────┘
            │ Drain
       ┌────┴────┐
       │ AO3400  │  (N-Channel MOSFET: Vds=30V, Id=5.7A, Rds(on)<30mΩ)
GPIO 6 ┤Gate     │
       └────┬────┘
            │ Source
           GND
            │
          [10kΩ] (Điện trở xả Gate xuống GND, chống motor tự kích khi khởi động)
```

---

### C. Mạch Lái Còi Báo Động (Active Buzzer)
Sử dụng Transistor NPN S8050 để khuếch đại dòng điện lái còi:

```
       VCC (3.3V hoặc 5V)
            │
       [ + Buzzer - ]
            │
         Collector
       ┌────┴────┐
GPIO 7 ┤ S8050   │  (BJT NPN: Ic=500mA, hFE=120-300)
       └────┬────┘
 [1kΩ]   Emitter
            │
           GND
```

---

### D. Mạch Cầu Phân Áp Đo Điện Áp Pin LiPo (Battery Monitor ADC)
Điện áp của cell pin Li-ion/LiPo nằm trong dải $3.0\,\text{V}$ (cạn) đến $4.2\,\text{V}$ (đầy). Bộ chuyển đổi ADC của vi điều khiển ESP32-C3 chỉ đo an toàn trong dải $0 - 2.5\,\text{V}$. Cầu phân áp đối xứng $100\,\text{k}\Omega / 100\,\text{k}\Omega$ chia đôi điện áp: $V_{\text{ADC}} = V_{\text{BAT}} / 2 \in [1.5\,\text{V}, 2.1\,\text{V}]$.

```
     VCC_BAT (Pin LiPo 3.7V - 4.2V)
            │
         [100kΩ]  (Điện trở chính xác 1% R1)
            │
            ├────────────── GPIO 1 (ADC1_CH1 ESP32-C3)
            │
         [100kΩ]  (Điện trở chính xác 1% R2)
            │
           GND
```
* **Dòng Rò Tĩnh (Quiescent Current)**: $I_{\text{leak}} = \frac{4.2\,\text{V}}{200\,\text{k}\Omega} = 21\,\mu\text{A}$ (đủ nhỏ để duy trì thời gian chờ của pin hàng tháng mà không cạn kiệt).

---

### E. Mạch Đèn LED Trạng Thái & Nút Nhấn Vật Lý
```
  GPIO 5 ──[ 330Ω ]──( >| LED Đa sắc )── GND

  GPIO 0 ──┬──[ Nút nhấn Taktil ]── GND
           │
         [100nF] (Tụ chống rung nảy phần cứng Hardware Debounce)
           │
          GND
```

---

## 4. BỐ TRÍ VẬT LÝ VÀ ĐỊNH HƯỚNG CƠ KHÍ KHI ĐEO (MECHANICAL PLACEMENT)

Để thuật toán cơ sinh học phản ánh chính xác tư thế cột sống, việc định vị module cảm biến trên lưng người dùng phải tuân thủ nghiêm ngặt các chỉ dẫn giải phẫu học:

```
                [ Hộp sọ (Head) ]
                        │
                  ( Đốt cổ C1-C7 )
                        │
                ┌───────┴───────┐
             [ Vai Trái ]   [ Vai Phải ]
                        │
           ═════════════════════════════
          [ VỊ TRÍ GẮN THIẾT BỊ T3 - T5 ]
           ═════════════════════════════
           ▲     [ Module MPU6050 ]    ▲
           │                           │
           │  +X: Dọc sống lưng lên cổ │
           │  +Y: Sang ngang vai trái  │
           │  +Z: Vuông góc ra sau lưng│
                        │
                 ( Đốt ngực T6-T12 )
                        │
                ( Đốt thắt lưng L1-L5 )
                        │
                  [ Xương cùng ]
```

### Quy Ước Chiều Trục Không Gian Của Cảm Biến:
1. **Trục $+X$**: Định hướng chạy **dọc theo cột sống ngực hướng lên cổ**. Khi người dùng ngồi thẳng đứng, véc-tơ trọng trường mặt đất hướng ngược trục $X$ ($a_x \approx +1.0g$).
2. **Trục $+Y$**: Định hướng **chạy ngang theo chiều vai** (vuông góc cột sống). Khi người dùng nghiêng vai sang trái/phải, thành phần gia tốc $a_y$ xuất hiện để đo góc **Pitch**.
3. **Trục $+Z$**: Định hướng **vuông góc với mặt phẳng lưng, hướng thẳng ra sau lưng**. Khi cúi gập người về phía trước, lưng nghiêng xuống khiến trục $Z$ hướng lên trời, thu nhận thành phần trọng trường $a_z > 0$ để tính góc **Roll**.
