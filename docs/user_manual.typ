#set terms(hanging-indent: 1.5em)

#set table(
  inset: 7pt,
  stroke: 0.5pt + rgb("#d1d5db"),
  fill: (col, row) => if row == 0 { rgb("#f3f4f6") } else { none }
)

#show table.cell.where(y: 0): set text(weight: "bold", fill: rgb("#1f2937"))

#let horizontalRule = line(length: 100%, stroke: 0.5pt + rgb("#e5e7eb"))
// Polyfill divider to allow compiling with typst < 0.15:
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
  text(size: 7.2pt, it)
)

#show quote.where(block: true): it => rect(
  fill: rgb("#f9fafb"),
  stroke: (left: 3pt + rgb("#4b5563"), rest: 0.5pt + rgb("#e5e7eb")),
  inset: (x: 10pt, y: 7pt),
  radius: (right: 3pt),
  width: 100%,
  it.body
)

#let content-to-string(content) = {
  if content.has("text") {
    content.text
  } else if content.has("children") {
    content.children.map(content-to-string).join("")
  } else if content.has("body") {
    content-to-string(content.body)
  } else if content == [ ] {
    " "
  }
}
#let conf(
  title: none,
  subtitle: none,
  authors: (),
  keywords: (),
  date: none,
  abstract-title: none,
  abstract: none,
  thanks: none,
  cols: 1,
  margin: (x: 1in, y: 0.95in),
  paper: "us-letter",
  lang: "vi",
  region: "VN",
  font: ("Times New Roman",),
  fontsize: 10.5pt,
  mathfont: none,
  codefont: ("Menlo",),
  linestretch: 1.3,
  sectionnumbering: none,
  linkcolor: none,
  citecolor: none,
  filecolor: none,
  pagenumbering: "1",
  doc,
) = {
  set document(
    title: title,
    keywords: keywords,
  )
  set document(
      author: authors.map(author => content-to-string(author.name)).join(", ", last: " & "),
  ) if authors != none and authors != ()
  set page(
    paper: paper,
    margin: margin,
    numbering: pagenumbering,
    columns: cols
  )

  set par(
    justify: true,
    leading: linestretch * 0.65em
  )
  set text(lang: lang,
           region: region,
           size: fontsize)

  set text(font: font) if font != none
  show math.equation: set text(font: mathfont) if mathfont != none
  show raw: set text(font: codefont) if codefont != none

  set heading(numbering: sectionnumbering)

  show link: set text(fill: rgb(content-to-string(linkcolor))) if linkcolor != none
  show ref: set text(fill: rgb(content-to-string(citecolor))) if citecolor != none
  show link: this => {
    if filecolor != none and type(this.dest) == label {
      text(this, fill: rgb(content-to-string(filecolor)))
    } else {
      text(this)
    }
  }

  doc
}
#show: doc => conf(
  font: ("Times New Roman",),
  codefont: ("Menlo",),
  linestretch: 1.3,
  pagenumbering: "1",
  cols: 1,
  doc,
)

#align(center)[
  #text(weight: "bold", size: 1.45em)[HƯỚNG DẪN SỬ DỤNG VÀ VẬN HÀNH THIẾT BỊ]
]
#v(0.3em)

#quote(block: true)[
#strong[Thiết bị]: Máy Giám Sát Và Cảnh Báo Tư Thế Ngồi \
#strong[Phiên bản hướng dẫn]: v1.1.2 \
#strong[Ngày cập nhật]: 2026-09-20
]

#v(0.4em)
#divider()
#v(0.4em)

== 1. HƯỚNG DẪN BẮT ĐẦU NHANH
<hướng-dẫn-bắt-đầu-nhanh>
=== Bước 1: Đeo thiết bị lên cơ thể
<bước-1-đeo-thiết-bị-lên-cơ-thể>
+ Gắn thiết bị vào dây đeo ngực hoặc kẹp vào cổ áo / mặt sau áo bằng ngàm kẹp tích hợp.
+ #strong[Vị trí chuẩn]: Đặt thiết bị nằm dọc chính giữa lưng trên, ở khoảng giữa hai bả vai.
+ #strong[Chiều thiết bị]: Cổng sạc Type-C hướng xuống dưới, mặt có nút bấm và đèn báo hướng ra ngoài.

#v(0.3em)
#grid(
  columns: (1fr, 1fr),
  gutter: 14pt,
  align: center + top,
  figure(
    image("images/device_front.png", width: 72%),
    caption: [Mặt trước: Nút bấm và đèn báo]
  ),
  figure(
    image("images/device_side_clip.png", width: 72%),
    caption: [Cạnh bên: Ngàm kẹp áo và cổng Type-C]
  )
)
#v(0.2em)

```
       [ Cổ ]
         │
    ┌────┴────┐
 [Bả vai]  [Bả vai]
    └────┬────┘
     [THIẾT BỊ]  <-- Vị trí giữa hai bả vai
         │
      [ Lưng ]
```

=== Bước 2: Bật nguồn thiết bị
<bước-2-bật-nguồn-thiết-bị>
- Bật công tắc nguồn hoặc cắm sạc qua cổng sạc Type-C ở cạnh dưới máy.
- Đèn báo trạng thái sẽ nhấp nháy nhanh 3 lần để xác nhận thiết bị đã sẵn sàng.
- Thiết bị rung nhẹ 1 nhịp báo hiệu bắt đầu làm việc.

#pagebreak()

== 2. THAO TÁC VỚI NÚT BẤM
<thao-tác-với-nút-bấm>
Thiết bị chỉ có một nút bấm duy nhất trên thân máy, hỗ trợ 3 thao tác cơ bản:

```
[ Nút bấm ] ──┬── Bấm 1 lần     ---> Tắt rung cảnh báo tức thì
              ├── Bấm 2 lần     ---> Tạm dừng theo dõi trong 10 phút
              └── Giữ 2 giây    ---> Lấy lại tư thế chuẩn (Hiệu chuẩn)
```

#figure(
  align(center)[#table(
    columns: (22%, 24%, 26%, 28%),
    align: (left,left,left,left,),
    table.header([Thao tác], [Cách thực hiện], [Phản hồi từ thiết bị], [Chức năng],),
    [#strong[Bấm 1 lần]], [Nhấn và thả ngay], [Thiết bị ngừng rung ngay lập tức], [Tắt cảnh báo hiện tại nếu bạn chưa thể ngồi thẳng lưng ngay.],
    [#strong[Bấm đúp] (2 lần)], [Nhấn 2 lần liên tiếp], [Rung nhẹ 2 nhịp ngắn], [#strong[Tạm dừng 10 phút]: Tắt toàn bộ rung nhắc nhở khi bạn đứng dậy đi lại hoặc nghỉ ngơi.],
    [#strong[Nhấn giữ]], [Nhấn và giữ trong 2 giây], [Rung dài 1 nhịp], [#strong[Hiệu chuẩn tư thế]: Đặt lại mốc chuẩn cho dáng ngồi hiện tại của bạn.],
  )]
  , kind: table
)

#v(0.5em)
#divider()
#v(0.5em)

== 3. BẢNG MÃ ĐÈN BÁO TRẠNG THÁI
<bảng-mã-đèn-báo-trạng-thái>
Đèn báo trên thiết bị giúp bạn nhận biết tình trạng hoạt động nhanh chóng:

#figure(
  align(center)[#table(
    columns: (25%, 23%, 24%, 28%),
    align: (left,left,left,left,),
    table.header([Kiểu nhấp nháy], [Tần suất đèn], [Trạng thái thiết bị], [Ý nghĩa và hành động],),
    [#strong[Chớp nhanh 3 lần]], [Ngay khi mở nguồn], [Khởi động hoàn tất], [Thiết bị đã sẵn sàng làm việc.],
    [#strong[Chớp nhẹ theo nhịp]], [2 giây chớp 1 lần], [Tư thế chuẩn], [Bạn đang ngồi thẳng lưng đúng cách.],
    [#strong[Nhấp nháy đều đặn]], [Chớp tắt liên tục], [Cảnh báo sai tư thế], [Bạn đang có dấu hiệu gù lưng hoặc nghiêng người.],
    [#strong[Nhấp nháy dồn dập]], [Chớp rất nhanh kèm rung], [Báo động sai tư thế], [Bạn đã ngồi sai tư thế quá lâu, cần điều chỉnh lại.],
    [#strong[Chớp chậm]], [Chớp tắt cách quãng chậm], [Đang tạm dừng], [Thiết bị đang trong thời gian nghỉ 10 phút.],
    [#strong[Chớp liên tục siêu tốc]], [Nhấp nháy liên tục không nghỉ], [Đang hiệu chuẩn], [Thiết bị đang đo dáng ngồi, vui lòng giữ yên người.],
    [#strong[Sáng mờ hoặc tắt hẳn]], [Đèn đứng yên hoặc không sáng], [Pin yếu hoặc cần kiểm tra], [Cắm sạc pin cho thiết bị.],
  )]
  , kind: table
)

#pagebreak()

== 4. KẾT NỐI VÀ THEO DÕI QUA TRANG WEB
<kết-nối-và-theo-dõi-qua-trang-web>
Thiết bị phát sóng mạng không dây cục bộ, cho phép theo dõi tư thế trực tiếp trên điện thoại hoặc máy tính mà không cần cài đặt phần mềm hay kết nối internet:

```
+-------------------------------------------------------------------------------+
| BƯỚC 1: Mở cài đặt Wi-Fi trên điện thoại, tìm và kết nối mạng:                |
|         Tên Wi-Fi (SSID): Posture-Monitor-AP                                  |
|         Mật khẩu:         posture123                                          |
+-------------------------------------------------------------------------------+
                                         │
                                         ▼
+-------------------------------------------------------------------------------+
| BƯỚC 2: Mở trình duyệt web và truy cập địa chỉ:                               |
|         http://192.168.4.1                                                    |
+-------------------------------------------------------------------------------+
```

#v(0.3em)

#grid(
  columns: (42%, 58%),
  gutter: 14pt,
  align: (center + top, left + top),
  [
    #figure(
      image("images/web_monitor_mobile.png", width: 95%),
      caption: [Giao diện Web Monitor trên điện thoại]
    )
  ],
  [
    === Các tính năng trên trang web:
    + #strong[Mô hình 3D cột sống]:
      - Hiển thị mô phỏng dáng ngồi theo thời gian thực.
      - Vuốt trên màn hình để xoay góc nhìn quanh cơ thể.
      - Chụm / mở hai ngón tay để phóng to, thu nhỏ.
      - Nút đặt lại góc nhìn đưa camera về vị trí mặc định.
    + #strong[Thông số góc nghiêng cơ thể]:
      - #strong[Góc cúi/ngửa]: Độ gập lưng về trước (bằng 0 khi ngồi thẳng).
      - #strong[Góc nghiêng vai]: Độ lệch vai sang trái / phải.
      - #strong[Góc xoay người]: Độ vặn quanh trục cột sống.
      - #strong[Tổng độ lệch]: Độ lệch tổng thể so với tư thế chuẩn.
    + #strong[Biểu đồ hoạt động]:
      - Đồ thị diễn biến 30 giây gần nhất kèm vạch ngưỡng cảnh báo.
    + #strong[Thống kê và lịch sử]:
      - Theo dõi chất lượng tư thế theo từng giờ trong ngày.
      - Tỷ lệ % thời gian ngồi đúng và thời gian ngồi gù lưng.
      - Nút xuất dữ liệu để tải báo cáo về máy.
  ]
)

#pagebreak()

== 5. QUY TRÌNH HIỆU CHUẨN TƯ THẾ CHUẨN
<quy-trình-hiệu-chuẩn-tư-thế-chuẩn>
Hiệu chuẩn là bước quan trọng nhất để thiết bị ghi nhận đúng dáng vóc chuẩn của từng người:

#grid(
  columns: (48%, 52%),
  gutter: 12pt,
  align: top,
  [
```
          4 ĐIỂM TƯ THẾ CHUẨN
  
  1. Lưng tựa thẳng tự nhiên vào ghế.
  2. Hai vai thả lỏng, cân bằng.
  3. Cằm song song sàn, nhìn thẳng.
  4. Hai bàn chân đặt phẳng trên sàn.
```
  ],
  [
    === Các bước thực hiện:
    + Ngồi vào tư thế chuẩn theo 4 điểm.
    + #strong[Thao tác]: Nhấn giữ nút 2 giây hoặc bấm nút "Hiệu chuẩn" trên trang web.
    + #strong[Giữ yên cơ thể trong 3 giây]: Đèn báo chớp rất nhanh trong lúc máy đo.
    + Khi hoàn thành, thiết bị rung 2 nhịp ngắn xác nhận lưu thành công.
  ]
)

#v(0.4em)
#divider()
#v(0.4em)

== 6. CẬP NHẬT PHẦN MỀM KHÔNG DÂY
<cập-nhật-phần-mềm-không-dây>
Khi có phiên bản phần mềm mới, bạn có thể cập nhật trực tiếp qua trang web mà không cần cắm dây cáp:

#grid(
  columns: (52%, 48%),
  gutter: 12pt,
  align: top,
  [
```
[ Điện thoại / Máy tính ]
          │
          ▼ Chọn file nâng cấp mới
Trang web (http://192.168.4.1 -> "Thiết bị")
          │
          ▼ Bấm "Cập nhật"
[ Thiết bị tiếp nhận & tự động nâng cấp ]
          │
          ▼
[ Tự động khởi động lại khi hoàn tất ]
```
  ],
  [
    #quote(block: true)[
    #strong[Lưu ý quan trọng khi cập nhật]: \
    1. Đảm bảo pin trên 20% trước khi thực hiện. \
    2. Không tắt nguồn thiết bị hoặc ngắt kết nối mạng khi đang nạp (khoảng 15 giây). \
    3. Sau khi nạp xong, thiết bị tự khởi động lại và hoạt động bình thường.
    ]
  ]
)

#v(0.4em)
#divider()
#v(0.4em)

== 7. XỬ LÝ SỰ CỐ THƯỜNG GẶP
<xử-lý-sự-cố-thường-gặp>
#figure(
  align(center)[#table(
    columns: (28%, 34%, 38%),
    align: (left,left,left,),
    table.header([Hiện tượng], [Nguyên nhân có thể], [Cách khắc phục],),
    [#strong[Đèn sáng đứng yên hoặc báo lỗi]], [Thiết bị gặp gián đoạn tiếp xúc nội bộ], [Tắt nguồn, chờ 5 giây rồi bật lại. Nếu vẫn bị, liên hệ kỹ thuật để được hỗ trợ.],
    [#strong[Không thấy Wi-Fi Posture-Monitor-AP]], [Thiết bị đang tắt hoặc đã hết pin], [Bật công tắc nguồn hoặc cắm cáp sạc pin cho thiết bị.],
    [#strong[Mô hình 3D lệch khi ngồi thẳng]], [Chưa lấy mốc tư thế cho dáng người của bạn], [Ngồi thẳng lưng đúng tư thế chuẩn và nhấn giữ nút 2 giây để hiệu chuẩn lại.],
    [#strong[Thiết bị không rung khi ngồi gù]], [Tính năng rung tắt trong cài đặt hoặc pin yếu], [Mở trang web, bật công tắc "Động cơ rung", đồng thời cắm sạc pin.],
  )]
  , kind: table
)
