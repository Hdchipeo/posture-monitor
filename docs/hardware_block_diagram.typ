#set terms(hanging-indent: 1.5em)

#set table(
  inset: 6pt,
  stroke: 0.5pt + rgb("#d1d5db"),
  fill: (col, row) => if row == 0 { rgb("#f3f4f6") } else { none }
)

#show table.cell.where(y: 0): set text(weight: "bold", fill: rgb("#1f2937"))

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
  inset: 7pt,
  radius: 3pt,
  width: 100%,
  breakable: false,
  text(size: 8pt, it)
)

#show quote.where(block: true): it => rect(
  fill: rgb("#f9fafb"),
  stroke: (left: 3pt + rgb("#4b5563"), rest: 0.5pt + rgb("#e5e7eb")),
  inset: (x: 10pt, y: 7pt),
  radius: (right: 3pt),
  width: 100%,
  it.body
)

#set page(
  paper: "us-letter",
  margin: (x: 1in, y: 0.9in),
  numbering: "1"
)

#set par(
  justify: true,
  leading: 1.3 * 0.65em
)

#set text(
  font: ("Times New Roman",),
  size: 10.5pt,
  lang: "vi",
  region: "VN"
)

#align(center)[
  #text(weight: "bold", size: 1.45em)[SƠ ĐỒ KHỐI PHẦN CỨNG THIẾT BỊ GIÁM SÁT TƯ THẾ]
]
#v(0.2em)

#quote(block: true)[
#strong[Thiết bị]: Máy Giám Sát Và Cảnh Báo Tư Thế Ngồi \
#strong[Tài liệu]: Thuyết Minh Sơ Đồ Khối Phần Cứng (Hardware Block Diagram) \
#strong[Phiên bản]: v1.1.2 \
#strong[Ngày cập nhật]: 2026-09-20
]

#v(0.3em)
#divider()
#v(0.3em)

== SƠ ĐỒ KHỐI PHẦN CỨNG TỔNG THỂ

Sơ đồ dưới đây thể hiện cấu trúc phân chia các khối chức năng vật lý bên trong thiết bị, sự tương tác giữa các thành phần và luồng kết nối tín hiệu điều khiển:

#v(0.3em)
#align(center)[
  #figure(
    image("images/hardware_block_diagram.svg", width: 100%),
    caption: [Sơ đồ khối phần cứng và các đường truyền tín hiệu chính của thiết bị]
  )
]
#v(0.3em)

#text(style: "italic", fill: rgb("#4b5563"))[
*Tóm tắt nguyên lý phần cứng*: Nguồn điện từ pin sạc được ổn áp để nuôi dưỡng toàn bộ hệ thống. Khối cảm biến liên tục ghi nhận góc nghiêng cơ thể và truyền về Bộ điều khiển trung tâm. Khi phát hiện người dùng ngồi sai tư thế trong thời gian quy định, bộ điều khiển sẽ kích hoạt bộ rung và đèn báo để nhắc nhở, đồng thời truyền dữ liệu qua sóng Wi-Fi không dây đến điện thoại hoặc máy tính.
]

#pagebreak()

== 1. CHI TIẾT CHỨC NĂNG TỪNG KHỐI PHẦN CỨNG

#figure(
  align(center)[#table(
    columns: (26%, 32%, 42%),
    align: (left, left, left),
    table.header([Khối chức năng], [Thành phần chính], [Nhiệm vụ và vai trò hoạt động],),
    [#strong[Khối Nguồn & Sạc Pin]], [Pin sạc lithium, Cổng sạc Type-C, Mạch bảo vệ và ổn áp], [Cung cấp nguồn 3.3V ổn định; hỗ trợ sạc tiện lợi qua cổng Type-C và tự ngắt khi đầy pin để đảm bảo an toàn.],
    [#strong[Bộ Điều Khiển Trung Tâm]], [Vi xử lý tính toán tốc độ cao & Bộ phát Wi-Fi tích hợp], [Đóng vai trò "bộ não" máy; tiếp nhận dữ liệu góc nghiêng, chạy thuật toán phát hiện gù lưng, điều khiển rung/đèn/còi và phát sóng Wi-Fi.],
    [#strong[Khối Cảm Biến Góc]], [Cảm biến đo chuyển động và gia tốc không gian], [Liên tục đo độ gập lưng (cúi/ngửa), độ lệch hai vai và độ xoay thân người theo thời gian thực.],
    [#strong[Khối Cảnh Báo Phản Hồi]], [Động cơ rung xúc giác, Đèn báo nhiều chế độ, Còi chip], [Tạo nhắc nhở cơ học thông qua nhịp rung nhẹ vào lưng; chớp đèn báo trạng thái; phát âm thanh cảnh báo khi cần thiết.],
    [#strong[Nút Bấm Vật Lý Đơn]], [Nút nhấn chống kẹt trên thân máy], [Giao tiếp trực tiếp: bấm 1 lần tắt rung nhanh, bấm đúp tạm dừng 10 phút, nhấn giữ 2 giây để hiệu chuẩn dáng ngồi chuẩn.],
    [#strong[Mạch Đo Mức Pin]], [Mạch đọc điện áp phân áp chính xác], [Theo dõi mức năng lượng còn lại của pin sạc, cảnh báo người dùng kịp thời cắm sạc khi pin yếu.],
    [#strong[Thiết Bị Hiển Thị]], [Điện thoại thông minh, Máy tính bảng, Laptop cá nhân], [Kết nối Wi-Fi xem mô hình 3D cột sống cử động trực tiếp, theo dõi điểm số và cài đặt thông số qua trình duyệt web.],
  )]
  , kind: table
)

#v(0.3em)
#divider()
#v(0.3em)

== 2. NGUYÊN LÝ HOẠT ĐỘNG VÀ LUỒNG TRUYỀN DẪN

Hệ thống phần cứng vận hành nhịp nhàng qua 4 luồng truyền dẫn chính:

+ #strong[Luồng cấp nguồn]: Năng lượng từ pin sạc qua mạch ổn áp biến đổi thành điện áp chuẩn 3.3V cấp cho toàn bộ vi mạch. Cổng Type-C hỗ trợ sạc nhanh và ngắt an toàn.
+ #strong[Luồng dữ liệu cảm biến]: Cảm biến liên tục truyền dữ liệu góc nghiêng cơ thể về vi điều khiển để so sánh với mốc dáng ngồi chuẩn đã lưu.
+ #strong[Luồng kích hoạt cảnh báo]: Khi góc gù lưng vượt ngưỡng cho phép quá 5 giây, vi điều khiển kích hoạt động cơ rung nhắc người dùng ngồi thẳng lưng lại.
+ #strong[Luồng truyền thông không dây]: Vi điều khiển tự phát sóng Wi-Fi cục bộ (tên `Posture-Monitor-AP`, mật khẩu `posture123`) truyền thông số thời gian thực lên trang web theo dõi.
