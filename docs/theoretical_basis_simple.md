# CƠ SỞ LÝ THUYẾT VÀ PHƯƠNG PHÁP TÍNH TOÁN CƠ SINH HỌC
*(Phiên bản giản lược, trực quan, dễ hiểu)*

> **Thiết bị**: Máy Giám Sát Và Cảnh Báo Tư Thế Ngồi  
> **Chuyên đề**: Cơ Sinh Học Cột Sống & Phương Pháp Đo Lường Tư Thế Thông Minh  
> **Phiên bản**: v1.1.2  
> **Ngày cập nhật**: 2026-09-20  

---

## 1. MÔ HÌNH TRỰC QUAN CƠ SINH HỌC VÀ ĐO LƯỜNG TƯ THẾ

Sơ đồ dưới đây minh họa 3 dạng sai lệch tư thế thường gặp khi ngồi làm việc và nguyên lý sử dụng trọng lực Trái Đất để xác định góc nghiêng cơ thể:

![Mô hình cơ sinh học và nguyên lý đo lường tư thế](images/biomechanics_diagram.svg)

*Ý nghĩa cơ sinh học*: Cột sống con người khi ngồi đúng có đường cong chữ S tự nhiên giúp cơ thể phân tán trọng lượng đầu và thân trên một cách nhẹ nhàng nhất. Khi ngồi làm việc lâu, người dùng thường có xu hướng gù lưng cúi đầu, nghiêng vai hoặc vặn người sang bên. Thiết bị sử dụng chính trọng lực Trái Đất làm thước đo tự nhiên để theo dõi liên tục các góc lệch này, từ đó nhắc nhở kịp thời và bảo vệ sức khỏe cột sống.

---

## 2. BA DẠNG BIẾN DẠNG CỘT SỐNG PHỔ BIẾN KHI NGỒI

| Dạng tư thế | Biểu hiện thực tế khi ngồi | Tác động cơ sinh học & Ngưỡng nhắc |
| :--- | :--- | :--- |
| **1. Góc Cúi / Ngửa**<br>*(Gù lưng - Slouching)* | Lưng trên gập về trước, đầu chúi về phía màn hình máy tính hoặc cúi nhìn điện thoại. | Tăng áp lực đè lên đĩa đệm và cổ gấp 3-4 lần bình thường. Máy kích hoạt cảnh báo khi độ gù vượt quá 12° sau 5 giây. |
| **2. Góc Nghiêng Vai**<br>*(Vẹo sang một bên)* | Một bên vai bị hạ thấp hoặc nhô cao khi ngồi tì một bên tay lên bàn, chống cằm hoặc ngồi vẹo mông. | Làm cột sống cong lệch sang một bên, gây mỏi cơ lưng bất đối xứng. Máy kích hoạt cảnh báo khi góc lệch quá 8°. |
| **3. Góc Xoay Người**<br>*(Vặn trục cột sống)* | Thân trên bị vặn sang trái hoặc phải khi màn hình đặt lệch góc so với vị trí bàn phím. | Gây lực xoắn đĩa đệm và căng cứng các cơ cạnh sống. Góc xoay được theo dõi và hiển thị trực tiếp trên mô hình 3D. |

---

## 3. NGUYÊN LÝ ĐO LƯỜNG VÀ TÍNH TOÁN ĐỘ NGHIÊNG ĐƠN GIẢN

1. **Thước đo tự nhiên từ Trọng Lực Trái Đất**:
   - Thiết bị được gắn dọc theo sống lưng (ở khoảng giữa hai bả vai). Trọng lực Trái Đất luôn có một hướng cố định duy nhất là hướng thẳng đứng xuống tâm đất.
   - Khi người dùng ngồi thẳng lưng, trọng lực chạy song song dọc theo thân máy.
   - Khi người dùng cúi gập lưng hoặc nghiêng người, máy nghiêng theo độ cong của cột sống. Sự thay đổi góc của trọng lực cho phép bộ vi xử lý tính toán ngay lập tức độ nghiêng chính xác (độ nhạy cao đến 0.5°).

2. **Thuật toán lọc mượt và chống rung ảo**:
   - Trong thực tế, các cử động nhẹ như hô hấp, nhịp chân hay bước đi ngắn có thể tạo ra rung động cơ học tạm thời.
   - Thiết bị sử dụng thuật toán lọc kết hợp thông minh: sử dụng cảm biến góc để giữ độ chuẩn dài hạn, kết hợp cảm biến vận tốc quay để phản ứng nhanh và làm phẳng các rung lắc tức thời, mang lại góc đo mượt mà, không bị giật lag.

---

## 4. QUY TRÌNH ĐÁNH GIÁ VÀ BẢO VỆ TRẢI NGHIỆM NGƯỜI DÙNG

1. **Mốc chuẩn cá nhân hóa**:
   - Mỗi người có vóc dáng cơ thể và độ cong cột sống khác nhau. Khi bấm giữ nút 2 giây, thiết bị sẽ ghi nhận tư thế ngồi thẳng tự nhiên nhất của chính người đó làm mốc 0° chuẩn riêng, không áp đặt số đo cứng nhắc.

2. **Bộ đệm trễ 5 giây thông minh**:
   - Khi người dùng cúi xuống nhặt bút, với tay lấy cốc nước hay vươn vai rồi ngồi thẳng lại trong vài giây, máy sẽ không rung để tránh làm phiền.
   - Chỉ khi sai tư thế duy trì liên tục quá 5 giây, cơ chế rung nhắc mới kích hoạt.

3. **Chấm Điểm Tư Thế (0 - 100 điểm) & Mô hình 3D**:
   - Tỷ lệ thời gian ngồi đúng được tổng hợp thành điểm số tư thế trực quan trên trang web, kèm mô hình 3D cột sống chuyển động theo từng nhịp thở và cử động lưng của người dùng.
