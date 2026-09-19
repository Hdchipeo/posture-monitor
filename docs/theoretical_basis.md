# CƠ SỞ LÝ THUYẾT VÀ PHƯƠNG PHÁP TÍNH TOÁN CƠ SINH HỌC (THEORETICAL BASIS & MATHEMATICAL DERIVATION)

> **Dự án**: ESP32-C3 Posture Monitor & Alert System  
> **Chuyên ngành**: Kỹ thuật Cơ Sinh Học (Biomechanics) & Ước Lượng Định Hướng Đa Trục (Multi-Axis Sensor Fusion)  
> **Ngày cập nhật**: 2026-09-20  

---

## 1. MÔ HÌNH CƠ SINH HỌC CỘT SỐNG CON NGƯỜI

Cột sống con người bao gồm 3 đoạn cong tự nhiên trong mặt phẳng đứng dọc (Sagittal Plane):
1. **Đoạn cột sống cổ (Cervical C1 - C7)**: Uốn cong ưỡn ra trước (Lordosis).
2. **Đoạn cột sống ngực (Thoracic T1 - T12)**: Uốn cong gù nhẹ ra sau sinh lý (Kyphosis, thông thường khoảng $2^\circ - 5^\circ$ ở tư thế ngồi nghỉ tự nhiên).
3. **Đoạn thắt lưng (Lumbar L1 - L5)**: Uốn cong ưỡn ra trước (Lordosis).

```
        MẶT PHẲNG ĐỨNG DỌC (SAGITTAL PLANE)
        
            Đốt cổ (C1-C7)  --> ưỡn ra trước
                  \
                   \ 
                   /
                  /   Đốt ngực (T1-T12)  --> gù sinh lý ~3°
                 |    [Vị trí đeo cảm biến T3-T5]
                  \
                   \
                   /  Đốt thắt lưng (L1-L5) --> ưỡn ra trước
                  /
                 [Xương cùng Pelvis]
```

Khi người dùng làm việc trước màn hình máy tính hoặc điện thoại di động:
* **Tật gù lưng (Slouching / Forward Head Posture)**: Đoạn ngực gập về phía trước, gia tăng góc gù (Thoracic Kyphosis) từ mức sinh lý $\sim 3^\circ$ lên tới $20^\circ - 35^\circ$. Điều này làm tăng áp lực tải trọng lên các đĩa đệm $L4/L5$ và các cơ vùng thang gấp 3 đến 5 lần bình thường.
* **Tật vẹo cột sống (Lateral Tilt)**: Nghiêng một bên vai sang trái hoặc phải khi ngồi chống cằm hoặc tì tay không đều.
* **Tật vặn người (Axial Torso Twist)**: Xoay vặn trục cơ thể sang bên khi đặt màn hình máy tính lệch một bên so với bàn phím.

Hệ thống Posture Monitor theo dõi đồng thời cả 3 biến dạng không gian này thông qua hệ góc Euler:
* **Roll**: Góc Cúi / Ngửa (Gập duỗi trong mặt phẳng Sagittal).
* **Pitch**: Góc Nghiêng vai (Nghiêng bên trong mặt phẳng Coronal).
* **Yaw**: Góc Xoay vặn trục thân người (Xoay trong mặt phẳng Transverse).

---

## 2. HỆ QUY CHIẾU KHÔNG GIAN: HORIZONTAL IMU VS. VERTICAL SPINE IMU

### A. Vấn đề của công thức hàng không truyền thống (Horizontal Drone/Flat Mount)
Trong các ứng dụng máy bay không người lái (UAV) hoặc robot tự hành, cảm biến IMU luôn được gắn nằm ngang trên khung máy bay:
* Trục $Z$ hướng thẳng lên trời hoặc xuống đất (đo trọng lực $a_z \approx 1.0g$).
* Trục $X$ hướng về phía trước ($a_x \approx 0$).
* Trục $Y$ hướng sang cánh trái ($a_y \approx 0$).

Công thức cổ điển ước tính góc từ gia tốc kế:
$$\text{pitch}_{\text{flat}} = \text{atan2f}(a_y, \sqrt{a_x^2 + a_z^2})$$
$$\text{roll}_{\text{flat}} = \text{atan2f}(-a_x, a_z)$$

### B. Nghịch lý khi áp dụng vào thiết bị đeo cột sống (Vertical Spine Mounting)
Khi gắn cảm biến lên lưng:
* Thiết bị được treo **dọc theo sống lưng**: Trục $X$ chạy dọc sống lưng hướng lên cổ, thu nhận toàn bộ véc-tơ phản lực trọng trường Trái Đất $\Rightarrow \mathbf{a_x \approx +1.0g}$.
* Trục $Z$ vuông góc với mặt lưng hướng ra sau $\Rightarrow \mathbf{a_z \approx 0.0g}$ (trong thực tế xương ngực cong sinh lý $\Rightarrow a_z \approx +0.05g$).
* Nếu áp dụng công thức cũ $\text{atan2f}(-a_x, a_z)$:
  $$\text{roll} = \text{atan2f}(-1.0, 0.05) \approx \mathbf{-87.13^\circ \approx -90^\circ}$$
  Hệ quả: Góc Roll khi người dùng ngồi thẳng lưng bị lệch một khoảng hằng số đúng **$-90^\circ$**, khiến mô hình 3D ngã gập ngược $90^\circ$ ra sau sàn.

### C. Công thức chuẩn hóa cho trục cột sống đứng (Vertical Spine Formulation)
Hệ quy chiếu đứng dọc lưng lấy phương của véc-tơ trọng lực làm trục gốc ($X = a_x \approx 1.0g$):
* **Góc Cúi / Ngửa (Roll)**:
  $$\mathbf{\theta_{\text{Roll}} = \text{atan2f}(a_z, a_x) \cdot \frac{180}{\pi}}$$
* **Góc Nghiêng vai (Pitch)**:
  $$\mathbf{\phi_{\text{Pitch}} = \text{atan2f}(a_y, \sqrt{a_x^2 + a_z^2}) \cdot \frac{180}{\pi}}$$

#### Kiểm chứng giá trị biên:
1. **Ngồi thẳng tự nhiên**: $a_x \approx 1.0g, a_z \approx 0.05g \Rightarrow \theta_{\text{Roll}} \approx +2.88^\circ$ (khớp hoàn hảo với độ cong sinh lý của người).
2. **Cúi gập người $20^\circ$ (Forward Slouch)**: Cột sống gập trước $\Rightarrow a_x = \cos(20^\circ) = 0.94g, a_z = \sin(20^\circ) = 0.34g \Rightarrow \theta_{\text{Roll}} = \mathbf{+20.0^\circ}$.
3. **Ưỡn ngửa người $15^\circ$ (Backward Extension)**: Cột sống ngửa sau $\Rightarrow a_x = \cos(-15^\circ) = 0.96g, a_z = \sin(-15^\circ) = -0.26g \Rightarrow \theta_{\text{Roll}} = \mathbf{-15.0^\circ}$.

---

## 3. BỘ LỌC BÙ KHỬ NHIỄU ĐỒNG PHA (COMPLEMENTARY FILTER CO-PHASE PROOF)

Gia tốc kế (Accelerometer) có độ chính xác cao về mặt tĩnh nhưng rất nhạy cảm với rung động cơ học (bước chân, rung motor). Ngược lại, con quay hồi chuyển (Gyroscope) đáp ứng tức thời cực nhanh nhưng bị trôi dạt tích phân (Drift) theo thời gian.

Bộ lọc bù kết hợp hai nguồn tín hiệu:
$$\theta_k = \alpha \cdot (\theta_{k-1} + \omega \cdot \Delta t) + (1 - \alpha) \cdot \theta_{\text{acc}}$$

Với:
* $\Delta t = 0.02\,\text{s}$ (tần số lấy mẫu 50Hz).
* $\alpha = 0.96$ (trọng số con quay hồi chuyển 96%).
* Thời hằng của bộ lọc:
  $$\tau = \frac{\alpha \cdot \Delta t}{1 - \alpha} = \frac{0.96 \cdot 0.02}{0.04} = 0.48\,\text{giây}$$
  Nghĩa là mọi rung chấn ngắn hạn $< 0.48\,\text{s}$ đều được con quay hồi chuyển hấp thụ và làm phẳng hoàn toàn.

### Chứng Minh Vi Phân Đồng Pha (Co-phase Derivative Proof)
Để đảm bảo bộ lọc không bị dao động hoặc lệch pha, thành phần đạo hàm góc nghiêng $\frac{d\theta}{dt}$ từ gia tốc phải bằng chính xác vận tốc góc $g_y$ từ con quay.

Góc nghiêng dọc cột sống: $\theta = \text{atan2}(a_z, a_x)$.
Theo giải tích vi phân:
$$\frac{d\theta}{dt} = \frac{a_x \frac{da_z}{dt} - a_z \frac{da_x}{dt}}{a_x^2 + a_z^2}$$

Theo phương trình chuyển động quay của vật rắn đối với véc-tơ gia tốc $\vec{a}$ quay với vận tốc góc $\vec{\omega} = (g_x, g_y, g_z)$:
$$\frac{d\vec{a}}{dt} = -\vec{\omega} \times \vec{a}$$
Tính các thành phần tích có hướng:
$$\frac{da_x}{dt} = - (g_y a_z - g_z a_y) \approx -g_y a_z \quad (\text{khi } a_y \approx 0)$$
$$\frac{da_z}{dt} = - (g_x a_y - g_y a_x) \approx +g_y a_x \quad (\text{khi } a_y \approx 0)$$

Thay vào biểu thức vi phân:
$$\frac{d\theta}{dt} = \frac{a_x (+g_y a_x) - a_z (-g_y a_z)}{a_x^2 + a_z^2} = \frac{g_y (a_x^2 + a_z^2)}{a_x^2 + a_z^2} = \mathbf{+g_y}$$

$$\mathbf{\frac{d\theta}{dt} \equiv +g_y}$$

**Kết luận**: Biểu thức vi phân của $\text{atan2}(a_z, a_x)$ hoàn toàn trùng khớp $100\%$ về dấu và độ lớn với vận tốc góc $+g_y$. Thuật toán bộ lọc bù đạt trạng thái cân bằng tuyệt đối.

---

## 4. THUẬT TOÁN TRÍCH XUẤT GÓC XOAY THÂN NGƯỜI (RELATIVE YAW EXTRACTION)

MPU6050 là cảm biến 6 bậc tự do (không tích hợp la bàn từ trường Magnetometer), do đó không có điểm tựa tuyệt đối để đo hướng cực Bắc Trái Đất. Tuy nhiên, trong cơ sinh học cột sống, người dùng chỉ quan tâm đến **góc xoay vặn thân người tương đối quanh chính trục cơ thể**.

### Phép Chiếu Véc-Tơ Lên Hướng Trọng Lực Thực:
Khi người dùng cúi hoặc nghiêng, trục xoay thân người không trùng với trục $Z$ cố định của cảm biến. Thuật toán chiếu véc-tơ vận tốc góc của 3 trục con quay $\vec{\omega} = (g_x, g_y, g_z)$ lên hướng của véc-tơ gia tốc trọng trường hiện thời $\vec{a} = (a_x, a_y, a_z)$:

$$\omega_{\text{yaw}} = \frac{\vec{a} \cdot \vec{\omega}}{\|\vec{a}\|} = \frac{a_x g_x + a_y g_y + a_z g_z}{\sqrt{a_x^2 + a_y^2 + a_z^2}}$$

### Bộ Lọc Vùng Chết (Deadband Filtering) & Chuẩn Hóa:
Để loại bỏ trôi dạt tĩnh của con quay khi người dùng ngồi yên:
$$\omega_{\text{filtered}} = \begin{cases} 
\omega_{\text{yaw}} & \text{khi } |\omega_{\text{yaw}}| > 0.35^\circ/\text{s} \\ 
0 & \text{khi } |\omega_{\text{yaw}}| \le 0.35^\circ/\text{s} 
\end{cases}$$

Tích phân góc xoay:
$$\text{Yaw}_k = \text{Yaw}_{k-1} + \omega_{\text{filtered}} \cdot \Delta t$$
Sau đó chuẩn hóa góc trong khoảng đối xứng:
$$\text{Yaw} \in [-180.0^\circ, +180.0^\circ]$$
Góc Yaw được tự động đặt lại về $0.0^\circ$ khi người dùng thực hiện Tare Zero-Calibration.

---

## 5. MÔ HÌNH ĐỘNG HỌC THUẬN KHUNG XƯƠNG 3D (FORWARD KINEMATICS)

Trên giao diện Web Monitor, mô hình 3D [web/skeleton3d.js](file:///Users/dangminhtam/Documents/Posture%20monitor/web/skeleton3d.js) tái hiện cột sống bằng chuỗi liên kết động học gồm $N = 10$ đốt sống:

```
Sacrum (Gốc = 0,0,0) --> Đốt 1 --> Đốt 2 --> ... --> Đốt 6 (Sensor T4) --> Đốt 10 (Hộp sọ C1)
```

Mỗi đốt $i \in [0, 10]$ có tham số chuẩn hóa $t = \frac{i}{N} \in [0, 1]$.
Thực nghiệm y sinh cho thấy đoạn cột sống ngực và cổ uốn cong phi tuyến mạnh hơn phần thắt lưng dưới. Độ uốn cong được điều chế bằng hàm lũy thừa:
$$F(t) = t^{1.3}$$

Tọa độ 3D của từng đốt sống $(x_i, y_i, z_i)$ trước khi xoay trục:
$$dx_i = \sin(\phi_{\text{Pitch}} \cdot F(t)) \cdot L \cdot i$$
$$dz_i = -\sin(\theta_{\text{Roll}} \cdot F(t)) \cdot L \cdot i$$
$$dy_i = \cos(\theta_{\text{Roll}} \cdot F(t)) \cdot \cos(\phi_{\text{Pitch}} \cdot F(t)) \cdot L \cdot i$$

Xoay vặn theo góc xoay thân người $\text{Yaw} \cdot t$:
$$X_i = dx_i \cdot \cos(\text{Yaw} \cdot t) - dz_i \cdot \sin(\text{Yaw} \cdot t)$$
$$Z_i = dx_i \cdot \sin(\text{Yaw} \cdot t) + dz_i \cdot \cos(\text{Yaw} \cdot t)$$
$$Y_i = dy_i$$

Sau đó các điểm không gian $P_i = (X_i, Y_i, Z_i)$ được chiếu lên mặt phẳng 2D Isometric bằng ma trận quay Azimuth $45^\circ$ và Elevation $25^\circ$ với hệ số thu phóng Perspective FoV 340, tạo nên mô hình 3D chuyển động trực quan, mượt mà 60 FPS hoàn toàn không cần thư viện bên ngoài.
