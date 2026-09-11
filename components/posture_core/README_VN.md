# Thành Phần Posture Core (Lõi Xử Lý Thuật Toán & Máy Trạng Thái)

## 1. Tổng Quan
Thành phần `posture_core` là bộ não quyết định của thiết bị, thực hiện tính toán sai số góc nghiêng cột sống so với điểm chuẩn tư thế đúng (Baseline Zero Offset), quản lý máy trạng thái (FSM) và thực thi chính sách cảnh báo lũy tiến (Progressive Escalation).

## 2. Máy Trạng Thái Hạn Định (FSM)
- `POSTURE_STATE_CALIBRATING`: Đang lấy mẫu tính trung bình góc ngồi chuẩn ban đầu.
- `POSTURE_STATE_GOOD`: Tư thế chuẩn, độ lệch góc nằm trong khoảng an toàn $|\Delta\theta| \le \theta_{\text{threshold}}$.
- `POSTURE_STATE_SUSPECTED_SLOUCH`: Phát hiện lệch góc nhưng đang trong thời gian ân hạn ($t < T_{\text{slouch}}$). Giúp lọc bỏ các hành động cúi người nhặt đồ, với tay tức thời.
- `POSTURE_STATE_ALERT_L1`: Duy trì sai tư thế quá thời gian cho phép ($t \ge T_{\text{slouch}}$). Kích hoạt rung ngắt quãng nhắc nhở nhẹ.
- `POSTURE_STATE_ALERT_L2`: Tiếp tục phớt lờ cảnh báo ($t \ge T_{\text{slouch}} + T_{\text{escalation}}$). Kích hoạt còi báo động kết hợp rung mạnh.
- `POSTURE_STATE_SNOOZED`: Chế độ tạm dừng cảnh báo (khi người dùng nghỉ ngơi).

## 3. Mô Hình Toán Học
$$\Delta \text{Pitch} = |\text{Pitch}_{\text{current}} - \text{Pitch}_{\text{offset}}|$$

$$\Delta \text{Roll} = |\text{Roll}_{\text{current}} - \text{Roll}_{\text{offset}}|$$

$$\text{Điều Kiện Sai Tư Thế} \iff (\Delta \text{Pitch} > \theta_{\text{thresh}}) \lor (\Delta \text{Roll} > \theta_{\text{thresh}})$$

## 4. Giao Diện Lập Trình (API)
- `posture_core_init()`: Nạp cấu hình hiệu chuẩn từ NVS.
- `posture_core_process_sample()`: Đánh giá mẫu góc và cập nhật FSM.
- `posture_core_start_calibration()`: Bắt đầu quá trình lấy điểm chuẩn.
- `posture_core_add_calibration_sample()`: Nạp mẫu đo vào bộ tính trung bình cân chỉnh.
- `posture_core_snooze()`: Tạm dừng cảnh báo trong khoảng thời gian chỉ định.
