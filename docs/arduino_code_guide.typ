#set terms(hanging-indent: 1.5em)

#set table(
  inset: (x: 5pt, y: 4.5pt),
  stroke: 0.5pt + rgb("#d1d5db"),
  fill: (col, row) => if row == 0 { rgb("#f3f4f6") } else { none }
)

#show table.cell.where(y: 0): set text(weight: "bold", fill: rgb("#1f2937"))
#show figure.where(kind: table): set text(size: 9.5pt)

#let horizontalRule = line(length: 100%, stroke: 0.5pt + rgb("#e5e7eb"))
#let divider = if "divider" in std { divider } else { horizontalRule }

#show figure.where(
  kind: table
): set figure.caption(position: top)

#show figure.where(
  kind: image
): set figure.caption(position: bottom)

#set figure(supplement: [Hình])

#show raw.where(block: true): it => block(
  fill: rgb("#f9fafb"),
  stroke: 0.5pt + rgb("#e5e7eb"),
  inset: 6pt,
  radius: 3pt,
  width: 100%,
  breakable: false,
  text(size: 8pt, it)
)

#show quote.where(block: true): it => rect(
  fill: rgb("#f9fafb"),
  stroke: (left: 3pt + rgb("#4b5563"), rest: 0.5pt + rgb("#e5e7eb")),
  inset: (x: 10pt, y: 6pt),
  radius: (right: 3pt),
  width: 100%,
  it.body
)

#set page(
  paper: "us-letter",
  margin: (x: 0.95in, top: 0.72in, bottom: 0.72in),
  numbering: "1"
)

#set par(
  justify: true,
  leading: 1.3 * 0.62em
)

#set text(
  font: ("Times New Roman",),
  size: 10.5pt,
  lang: "vi",
  region: "VN"
)

#show heading.where(level: 2): it => block(above: 0.85em, below: 0.45em, it)

#align(center)[
  #text(weight: "bold", size: 1.4em)[CẤU TRÚC VÀ CÁC THÀNH PHẦN CODE TRONG ARDUINO]
]
#v(0.15em)

#quote(block: true)[
#strong[Dự án]: Máy Giám Sát Và Cảnh Báo Tư Thế Ngồi (Arduino ESP32 / ESP32-C3) \
#strong[Tài liệu]: Hướng Dẫn Giải Thích Các Khối Chức Năng Và Module Mã Nguồn \
#strong[Phiên bản Firmware]: v1.1.2 \
#strong[Ngày cập nhật]: 2026-09-20
]

#v(0.2em)
#divider()
#v(0.2em)

== TỔNG QUAN KIẾN TRÚC PHÂN KHỐI TRONG CHƯƠNG TRÌNH

Toàn bộ mã nguồn Arduino được thiết kế theo nguyên tắc *chia nhỏ thành từng module độc lập (Modular Design)*. Mỗi khối đảm nhiệm duy nhất một chức năng rõ ràng, giúp mã nguồn ngắn gọn, dễ đọc, dễ bảo trì và mở rộng:

#v(0.2em)
#align(center)[
  #figure(
    image("images/arduino_architecture_diagram.svg", width: 100%),
    caption: [Sơ đồ phân khối chức năng và luồng tương tác giữa các module trong Arduino]
  )
]
#v(0.2em)

#text(style: "italic", fill: rgb("#4b5563"))[
*Tóm tắt vai trò các khối*: Chương trình chính `posture_monitor.ino` đóng vai trò là "nhạc trưởng" điều phối hoạt động ở chu kỳ 50 lần mỗi giây. Module cấu hình `Config.h` quản lý toàn bộ chân cắm và ngưỡng góc. Module cảm biến đọc số liệu chuyển động; Module xử lý tính toán độ lệch tư thế; Module cảnh báo điều khiển nhịp rung/còi mà không làm đứng máy; Module lưu trữ bảo toàn cài đặt vào bộ nhớ vĩnh viễn.
]

#pagebreak()

== 1. CHI TIẾT 5 MODULE CHỨC NĂNG ĐỘC LẬP

#figure(
  align(center)[#table(
    columns: (22%, 28%, 50%),
    align: (left, left, left),
    table.header([Tên Module / Tệp], [Thành phần khai báo chính], [Chức năng và nhiệm vụ cụ thể],),
    [#strong[Khối Cấu Hình] \ (`Config.h`)], [• Khai báo chân cắm I/O \ • Ngưỡng góc gù (15°) \ • Thời gian đệm trễ (5s) \ • Thời gian tạm dừng (10p)], [Tập trung toàn bộ hằng số hệ thống tại một nơi; tự động nhận diện phần cứng (ESP32-C3 hoặc ESP32 Classic) và ngăn chặn việc gán nhầm chân cắm gây lỗi vi mạch.],
    [#strong[Khối Cảm Biến] \ (`MPU6050Driver`)], [• `begin()`: Bật cảm biến \ • `readRaw()`: Đọc số liệu \ • `sleep()` / `wakeUp()`], [Giao tiếp với cảm biến đo chuyển động; tích hợp cơ chế tự động khôi phục đường truyền khi bị gián đoạn tiếp xúc và hỗ trợ chế độ ngủ sâu để tiết kiệm pin.],
    [#strong[Khối Xử Lý Tư Thế] \ (`PostureCore`)], [• 3 góc: Cúi, Nghiêng, Xoay \ • 6 trạng thái tư thế \ • Bộ đệm trễ 5 giây \ • Điểm tư thế (0 - 100)], [Được xem là "bộ não" tính toán; so sánh dáng ngồi hiện tại với mốc chuẩn đã lưu; lọc bỏ các rung động giả do hô hấp; phân loại trạng thái ngồi đúng hay sai và chấm điểm chất lượng tư thế.],
    [#strong[Khối Điều Khiển Báo] \ (`ActuatorManager`)], [• `setPattern()`: Chọn nhịp \ • `update()`: Cập nhật nhịp \ • `isVibrating()`: Trạng thái], [Điều khiển động cơ rung và còi theo các nhịp điệu quy định (rung nhẹ nhắc nhở cấp 1, rung dồn dập kèm còi cấp 2); vận hành phi nghẽn, không làm gián đoạn các tác vụ khác.],
    [#strong[Khối Lưu Trữ] \ (`StorageManager`)], [• `loadCalibration()` \ • `saveCalibration()` \ • Kiểm tra toàn vẹn mã], [Ghi nhớ mốc tư thế chuẩn và các cài đặt vào bộ nhớ Flash (NVS) của chip; dữ liệu được bảo vệ an toàn, không bị mất ngay cả khi người dùng tắt máy hoặc hết pin.],
  )]
  , kind: table
)

#v(0.2em)
#divider()
#v(0.2em)

== 2. CHƯƠNG TRÌNH ĐIỀU PHỐI CHÍNH (posture_monitor.ino)

Tệp chính `posture_monitor.ino` thực thi 4 nhiệm vụ cốt lõi:

+ #strong[Khởi tạo hệ thống (`setup`)]: Thiết lập chế độ các chân nút bấm, đèn LED và còi; chớp nhanh đèn LED 3 lần để tự kiểm tra nguồn điện (POST); tải mốc tư thế chuẩn từ bộ nhớ trong và đánh thức cảm biến sẵn sàng làm việc.
+ #strong[Vòng lặp chính (`loop`)]: Vận hành tuần hoàn đều đặn ở tần số 50Hz (chu kỳ 20ms); lần lượt đọc cảm biến, kiểm tra nút bấm, cập nhật nhịp đèn LED, kích hoạt rung nếu phát hiện ngồi sai và in thông số chẩn đoán ra màn hình máy tính mỗi 5 giây.
+ #strong[Xử lý nút bấm thông minh (`handleButton`)]: Nhận dạng 3 thao tác: bấm 1 lần để tắt rung tức thì nếu chưa thể ngồi thẳng ngay; bấm đúp 2 lần để tạm dừng 10 phút khi nghỉ giải lao; nhấn giữ 2 giây để hiệu chuẩn lấy mốc tư thế chuẩn mới.
+ #strong[Cập nhật đèn báo trạng thái (`updateStatusLed`)]: Chớp nhẹ nhịp tim 2s/lần khi ngồi đúng, nhấp nháy đều khi cảnh báo gù lưng, chớp rất nhanh khi báo động kéo dài và chớp siêu tốc khi đang ghi nhận tư thế chuẩn.

#v(0.2em)
#divider()
#v(0.2em)

== 3. NGUYÊN TẮC THIẾT KẾ HOẠT ĐỘNG KHÔNG NGHẼN (NON-BLOCKING)

Toàn bộ chương trình Arduino tuân thủ nghiêm ngặt nguyên tắc *hoạt động không nghẽn*:
- Tuyệt đối không sử dụng lệnh dừng chương trình (`delay()`) trong vòng lặp chính.
- Mọi thao tác tạo nhịp rung, chớp đèn LED, nhận diện nút bấm và đếm trễ 5 giây đều được tính toán thông qua bộ đếm thời gian thực `millis()`.
- Nhờ đó, thiết bị luôn phản hồi ngay lập tức với các thao tác của người dùng mà không bao giờ bị đơ máy hay trễ nhịp.
