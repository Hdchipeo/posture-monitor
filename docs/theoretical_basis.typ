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
  margin: (x: 1in, y: 0.88in),
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
  #text(weight: "bold", size: 1.45em)[CƠ SỞ LÝ THUYẾT VÀ PHƯƠNG PHÁP TÍNH TOÁN CƠ SINH HỌC]
]
#v(0.2em)

#quote(block: true)[
#strong[Thiết bị]: Máy Giám Sát Và Cảnh Báo Tư Thế Ngồi \
#strong[Chuyên đề]: Cơ Sinh Học Cột Sống & Phương Pháp Đo Lường Tư Thế Thông Minh \
#strong[Phiên bản]: v1.1.2 \
#strong[Ngày cập nhật]: 2026-09-20
]

#v(0.3em)
#divider()
#v(0.3em)

== MÔ HÌNH TRỰC QUAN CƠ SINH HỌC VÀ ĐO LƯỜNG TƯ THẾ

Sơ đồ dưới đây minh họa 3 dạng sai lệch tư thế thường gặp khi ngồi làm việc và nguyên lý sử dụng trọng lực Trái Đất để xác định góc nghiêng cơ thể:

#v(0.3em)
#align(center)[
  #figure(
    image("images/biomechanics_diagram.svg", width: 100%),
    caption: [Mô hình các trục biến dạng cột sống và nguyên lý xác định độ nghiêng tự nhiên]
  )
]
#v(0.3em)

#text(style: "italic", fill: rgb("#4b5563"))[
*Ý nghĩa cơ sinh học*: Cột sống con người khi ngồi đúng có đường cong chữ S tự nhiên giúp cơ thể phân tán trọng lượng đầu và thân trên một cách nhẹ nhàng nhất. Khi ngồi làm việc lâu, người dùng thường có xu hướng gù lưng cúi đầu, nghiêng vai hoặc vặn người sang bên. Thiết bị sử dụng chính trọng lực Trái Đất làm thước đo tự nhiên để theo dõi liên tục các góc lệch này, từ đó nhắc nhở kịp thời và bảo vệ sức khỏe cột sống.
]

#pagebreak()

== 1. BA DẠNG BIẾN DẠNG CỘT SỐNG PHỔ BIẾN KHI NGỒI

#figure(
  align(center)[#table(
    columns: (26%, 34%, 40%),
    align: (left, left, left),
    table.header([Dạng tư thế], [Biểu hiện thực tế khi ngồi], [Tác động cơ sinh học & Ngưỡng nhắc],),
    [#strong[1. Góc Cúi / Ngửa] \ (Gù lưng - Slouching)], [Lưng trên gập về trước, đầu chúi về phía màn hình máy tính hoặc cúi nhìn điện thoại.], [Tăng áp lực đè lên đĩa đệm và cổ gấp 3-4 lần bình thường. Máy kích hoạt cảnh báo khi độ gù vượt quá 12° sau 5 giây.],
    [#strong[2. Góc Nghiêng Vai] \ (Vẹo sang một bên)], [Một bên vai bị hạ thấp hoặc nhô cao khi ngồi tì một bên tay lên bàn, chống cằm hoặc ngồi vẹo mông.], [Làm cột sống cong lệch sang một bên, gây mỏi cơ lưng bất đối xứng. Máy kích hoạt cảnh báo khi góc lệch quá 8°.],
    [#strong[3. Góc Xoay Người] \ (Vặn trục cột sống)], [Thân trên bị vặn sang trái hoặc phải khi màn hình đặt lệch góc so với vị trí bàn phím.], [Gây lực xoắn đĩa đệm và căng cứng các cơ cạnh sống. Góc xoay được theo dõi và hiển thị trực tiếp trên mô hình 3D.],
  )]
  , kind: table
)

#v(0.3em)
#divider()
#v(0.3em)

== 2. NGUYÊN LÝ ĐO LƯỜNG VÀ TÍNH TOÁN ĐỘ NGHIÊNG ĐƠN GIẢN

+ #strong[Thước đo tự nhiên từ Trọng Lực Trái Đất]:
  - Thiết bị được gắn dọc theo sống lưng (ở khoảng giữa hai bả vai). Trọng lực Trái Đất luôn có một hướng cố định duy nhất là hướng thẳng đứng xuống tâm đất.
  - Khi người dùng ngồi thẳng lưng, trọng lực chạy song song dọc theo thân máy.
  - Khi người dùng cúi gập lưng hoặc nghiêng người, máy nghiêng theo độ cong của cột sống. Sự thay đổi góc của trọng lực cho phép bộ vi xử lý tính toán ngay lập tức độ nghiêng chính xác (độ nhạy cao đến 0.5°).
+ #strong[Thuật toán lọc mượt và chống rung ảo]:
  - Trong thực tế, các cử động nhẹ như hô hấp, nhịp chân hay bước đi ngắn có thể tạo ra rung động cơ học tạm thời.
  - Thiết bị sử dụng thuật toán lọc kết hợp thông minh: sử dụng cảm biến góc để giữ độ chuẩn dài hạn, kết hợp cảm biến vận tốc quay để phản ứng nhanh và làm phẳng các rung lắc tức thời, mang lại góc đo mượt mà, không bị giật lag.

#v(0.3em)
#divider()
#v(0.3em)

== 3. QUY TRÌNH ĐÁNH GIÁ VÀ BẢO VỆ TRẢI NGHIỆM NGƯỜI DÙNG

+ #strong[Mốc chuẩn cá nhân hóa]: Mỗi người có vóc dáng cơ thể và độ cong cột sống khác nhau. Khi bấm giữ nút 2 giây, thiết bị sẽ ghi nhận tư thế ngồi thẳng tự nhiên nhất của chính người đó làm mốc 0° chuẩn riêng, không áp đặt số đo cứng nhắc.
+ #strong[Bộ đệm trễ 5 giây thông minh]: Khi người dùng cúi xuống nhặt bút, với tay lấy cốc nước hay vươn vai rồi ngồi thẳng lại trong vài giây, máy sẽ không rung để tránh làm phiền. Chỉ khi sai tư thế duy trì liên tục quá 5 giây, cơ chế rung nhắc mới kích hoạt.
+ #strong[Chấm Điểm Tư Thế (0 - 100 điểm) & Mô hình 3D]: Tỷ lệ thời gian ngồi đúng được tổng hợp thành điểm số tư thế trực quan trên trang web, kèm mô hình 3D cột sống chuyển động theo từng nhịp thở và cử động lưng của người dùng.
