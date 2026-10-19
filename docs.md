# Luyện rồng (Dragon Training) - thiết kế và kịch bản

Game nuôi thú ảo thời gian thực chạy trên Zin100 (ESP32-2432S035). Mở từ trang **Ứng dụng > Luyện rồng** (tiếng Anh: *Dragon training*).
Dữ liệu được ghi vào **thẻ nhớ microSD**: `/zin100/dragon.json`. Không có thẻ nhớ thì game không chơi được và hiện thông báo yêu cầu cắm thẻ.

- Mã nguồn: `firmware/src/dragon.h`
- Cấu hình qua web: mục **Luyện rồng** tại `http://zin100.local`

---

## 1. Kịch bản

### Bối cảnh
Ở một nơi xa gọi là **Long Giới**, các loài rồng sống trên những tầng mây. Mỗi nghìn năm, **Ngọc Rồng Thần** gửi xuống thế giới loài người một quả trứng, và chỉ rồng nào được một con người ấp ủ, chăm sóc từ bé mới có thể thức tỉnh sức mạnh thần thánh. Đêm sao băng, quả trứng đó rơi xuống bàn của bạn.

### Hành trình 9 cấp độ

| # | Cấp độ | Thời gian khỏe mạnh để lên cấp* | Ngoại hình | Câu chuyện khi đạt cấp |
|---|--------|------|-----------|-----------|
| 0 | **Trứng** | 10 phút | Quả trứng kem có đốm màu, nứt dần khi gần nở | Đêm sao băng, quả trứng cổ phát sáng rơi xuống bàn bạn. Hãy giữ ấm cho nó. |
| 1 | **Rồng con** | 2 giờ | Nhỏ, xanh lá, 2 sừng nhỏ | Tách! Vỏ trứng nứt ra, rồng con dụi mắt nhìn bạn: bạn là gương mặt đầu tiên nó thấy. |
| 2 | **Rồng cấp 1** | 8 giờ | Xanh ngọc, có tay | Biết bay những vòng đầu tiên, cánh nhỏ bắt đầu mọc. |
| 3 | **Rồng cấp 2** | 1 ngày | Xanh dương, có cánh | Phun được lửa nhỏ, thích nghe bạn kể chuyện mỗi tối. |
| 4 | **Rồng cấp 3** | 2 ngày | Tím chàm, sừng dài | Không còn sợ sấm sét, còn gọi được mưa về. |
| 5 | **Rồng trưởng thành** | 4 ngày | Tím, cánh lớn, 4 sừng | Đôi cánh che cả căn phòng, cả Long Giới nghe thấy tiếng gầm. |
| 6 | **Rồng vương** | 7 ngày | Đỏ rực, thêm sừng giữa | Các rồng non đến xin được che chở. |
| 7 | **Rồng hoàng** | 10 ngày | Cam, vương miện vàng, hào quang | Mỗi lần vẫy cánh rải đốm lửa vàng. |
| 8 | **Rồng thần** | (cấp cuối) | Vàng kim, cánh vàng, vòng sáng, sao | Ngọc rồng thần thức tỉnh, rồng bay lên mây ngũ sắc. |

\* Số "phút khỏe mạnh" ở bảng là thời gian thật mà rồng phải ở trạng thái tốt (xem mục 3). Tổng cộng khoảng **24 ngày** chăm sóc tốt để từ trứng thành rồng thần.

### Luân hồi
Rồng thần không biến mất: nó để lại **một quả trứng mới**. Khi bấm **Luân hồi** (nút xuất hiện ở góc trên khi đạt cấp cuối):
- Bạn giữ lại xu, kỷ lục và thống kê.
- Số **thế hệ** tăng thêm 1.
- Mỗi thế hệ trước làm các cấp lên nhanh hơn **5%** (tối đa 4 thế hệ, tức nhanh hơn 20%).
- Không phát lại cảnh mở đầu.

Rồng không bao giờ chết: nếu bỏ bê lâu thì rồng bị **ốm** và ngừng lớn cho đến khi bạn cho uống thuốc.

---

## 2. Chỉ số

Có 4 thanh từ 0 đến 100:

| Chỉ số | Màu | Giảm mỗi giờ (thức) | Khi ngủ |
|--------|-----|---------------------|---------|
| **No bụng** | cam | -6 | -2 |
| **Vui vẻ** | hồng | -4 (ốm: -8) | -1 |
| **Sạch sẽ** | xanh lơ | -3 | -1 |
| **Năng lượng** | xanh lá | -3 (ban đêm 22h-6h: -6) | **+18** |

- Thanh dưới 25 chuyển sang màu đỏ và rồng nhắc bạn bằng câu thoại.
- **Trứng** chỉ có một thanh **Độ ấm** (mất 6 mỗi giờ). Độ ấm trên 20 thì trứng mới "ấp" tiếp.
- Biểu cảm: vui (>= 65, má hồng, cười), bình thường (30-64), buồn (< 30, có giọt nước mắt).

## 3. Tăng trưởng và bệnh

- Cứ mỗi **phút thật** mà cả 4 chỉ số đều > 25 và rồng không ốm, rồng được **+1 điểm lớn**. Đủ điểm của cấp hiện tại thì tiến hóa (xem bảng ở mục 1).
- Một chỉ số < 10 liên tục hơn **30 phút** thì rồng **bị ốm**: ngừng lớn, vui vẻ giảm nhanh hơn, không chơi được.
- **Thuốc** (20 xu) chữa ốm và đưa các chỉ số thấp lên tối thiểu 30.
- Khi tiến hóa: +`50 x cấp` xu, vui vẻ về 100, hiện màn hình câu chuyện. Khi nở từ trứng, rồng có sẵn No bụng 70, Sạch 90, Năng lượng 80.

## 4. Thao tác

| Nút | Tác dụng |
|-----|----------|
| **Ấp** (chỉ ở giai đoạn trứng) | Độ ấm +25, +0,5 điểm lớn |
| **Ăn** | Mở thực đơn: **Trái cây** (miễn phí, no +20) / **Thịt nướng** (5 xu, no +45, vui +5) / **Bánh ngọc** (15 xu, no +30, vui +20, +2 điểm lớn). No trên 90 thì từ chối ăn và vui vẻ -8 |
| **Chơi** | Mini game **Bắt ngọc** 12 giây: chạm các viên ngọc hiện ngẫu nhiên (mỗi viên tồn tại 1,1 giây). Được `số ngọc` xu, vui +`8 + 3 x số ngọc`, năng lượng -8, tối đa +5 điểm lớn. Cần năng lượng >= 15 và rồng không ốm |
| **Tắm** | Sạch +45, vui +3 |
| **Ngủ / Dậy** | Chuyển chế độ ngủ (không ngủ được khi năng lượng > 90). Tự dậy khi năng lượng đầy. Khi ngủ, nền chuyển sang ban đêm và rồng phục hồi năng lượng |
| **Thuốc** | Chữa bệnh (20 xu) |
| **Luân hồi** | Chỉ ở cấp Rồng thần, sinh trứng mới |

Khi rồng đang ngủ, các nút Ăn/Tắm/Chơi không dùng được ("Rồng đang ngủ, đừng đánh thức!").

## 5. Xu (kinh tế)

Nguồn: mini game, **quà hằng ngày** (mở game lần đầu mỗi ngày: `10 + 5 x cấp` xu, Rồng thần thêm **+100**), phần thưởng tiến hóa, và sự kiện **rồng nhặt được xu** (ngẫu nhiên, trung bình mỗi 30 phút khi game đang chạy: +3 xu, vui +5).
Chi tiêu: thức ăn ngon, bánh ngọc, thuốc.

## 6. Thời gian thực

- Game chạy **nền** ngay khi thiết bị bật, kể cả khi bạn không mở trang rồng. Giữa các lần xem, các chỉ số vẫn giảm.
- Khi **tắt máy**, lần bật sau game tính bù thời gian vắng bằng đồng hồ thật (NTP): chia theo từng giờ, mọi tốc độ (giảm chỉ số và lớn) chỉ bằng **một nửa** (rồng nghỉ ngơi), tối đa **72 giờ**. Cần có Wi-Fi để biết giờ; tới khi có giờ thì việc tính bù tạm hoãn.
- Ban đêm (22h-6h theo múi giờ cài đặt), nếu năng lượng dưới 60 rồng nhắc "Khuya rồi, cho rồng đi ngủ nhé".

## 7. Màn hình

```
+--------------------------------------------------+
| <  Rồng ảo                    [Luân hồi]  195 xu |
| Tiểu Long  -  Rồng cấp 1                         |
| +-----------------------+  No bụng 72  =====     |
| |                       |  Vui vẻ  88  =====     |
| |     (hình con rồng)   |  Sạch sẽ 64  ====      |
| |                       |  Năng lượng 80 =====   |
| +-----------------------+  Tiến hóa: 12%         |
| Rồng đang rất vui!                               |
| [ Ăn ] [ Chơi ] [ Tắm ] [ Ngủ ] [ Thuốc ]        |
+--------------------------------------------------+
```

Con rồng được vẽ hoàn toàn bằng hình học LVGL (không dùng ảnh), thay đổi theo cấp độ, tâm trạng, giấc ngủ và bệnh. Có hiệu ứng thở nhẹ. Nền chuyển sang ban đêm khi rồng ngủ hoặc sau 22h.

## 8. Lưu dữ liệu

File JSON `/zin100/dragon.json`, ghi **tạm rồi đổi tên** (`dragon.tmp` -> `dragon.json`) để mất điện giữa chừng không hỏng file. Lưu: sau mỗi thao tác, mỗi lần tiến hóa, mỗi 60 giây và khi thoát trang rồng.

Các trường: `v`, `name`, `stage` (0-8), `growth`, `satiety`, `mood`, `clean`, `energy`, `lowMin`, `sick`, `sleeping`, `intro`, `coins`, `lastTs`, `bornTs`, `dailyDay`, `gen`, `best`, `fed`, `played`, `bathed`, `meds`.

## 9. Cấu hình qua web

Mục **Luyện rồng** tại `http://zin100.local`: xem cấp độ, tiến độ, xu, thế hệ, kỷ lục; **đặt tên rồng**; **nuôi lại từ đầu** (xóa dữ liệu rồng).

API: `POST /api/dragon` với `name=...`, `reset=1`, hoặc `speed=N` (nhân tốc độ lớn lên, chỉ để thử nghiệm, tự về 1 sau khi khởi động lại; ví dụ `speed=300` tiến hóa nhanh gấp 300 lần).

## 10. Ý tưởng mở rộng

- Âm thanh/giọng nói: cho rồng "nói" qua trợ lý Xiaozhi.
- Thêm mini game (nhớ hình, bắn lửa), thức ăn theo mùa.
- Huy hiệu thành tựu (nuôi 7 ngày liên tục, đạt Rồng vương...).
- Trang bị phụ kiện (mũ, khăn) mua bằng xu.

---
---

# Zin100 Version 2 - thiết kế giao diện (đã triển khai, firmware 2.0.0)

Mục tiêu: biến Zin100 từ "màn hình hiển thị" thành **một người bạn sống động trên bàn**. Ba ý chính:

1. **Con rồng là linh hồn của thiết bị**: rồng của bạn xuất hiện ở trang chủ, làm avatar cho trợ lý AI, ngủ cùng màn hình chờ.
2. **Giao diện thay đổi theo thời gian và thời tiết**: sáng, trưa, chiều tối và đêm có màu nền khác nhau; trời mưa thì có mưa rơi, đêm quang thì có sao.
3. **Mọi màn hình chung một ngôn ngữ thiết kế**: một thanh trạng thái, một kiểu thẻ, một kiểu thông báo, một bộ biểu tượng.

V1 (commit `8b6aab9`) là mốc ổn định. V2 làm theo từng giai đoạn (mục 10), mỗi giai đoạn đều nạp và dùng được ngay.

## 1. Giới hạn phần cứng (thiết kế phải tôn trọng)

| Tài nguyên | Hiện trạng | Hệ quả cho thiết kế |
|---|---|---|
| RAM trống | ~130 KB (không có PSRAM) | Không giữ ảnh toàn màn hình trong RAM; mỗi trang tạo khi mở, xóa khi đóng |
| Flash còn trống | ~800 KB (đang dùng 74%) | Ảnh mới tối đa ~350 KB; nền dùng gradient và hình học thay vì ảnh |
| Màn hình | 480x320, SPI 40 MHz | Vẽ lại cả màn hình mất ~60 ms (~15 fps); hiệu ứng chỉ được làm ở vùng nhỏ |
| Cảm ứng | Điện trở, có nhiễu | Nút >= 44 px, không dùng thao tác đa điểm; vuốt đơn giản (trái/phải/xuống) |
| Thẻ nhớ | Có | Có thể chứa hình nền, âm thanh, dữ liệu game |
| Cần kiểm tra | Loa (cổng SPEAK), đèn LED RGB, cảm biến ánh sáng (LDR) | Nếu có: âm báo, đèn trạng thái, tự chỉnh độ sáng |

**Quy tắc hiệu năng**: mỗi màn hình tối đa ~120 đối tượng LVGL. Hiệu ứng động chỉ chạy trên vùng nhỏ hơn 1/4 màn hình. Không có hiệu ứng nào chạy liên tục khi màn hình đã tắt.

## 2. Ngôn ngữ thiết kế "Bầu trời Zin"

- **Nền theo giờ** (gradient dọc, chuyển mượt khi đổi khung giờ):

  | Khung giờ | Trên | Dưới | Điểm nhấn |
  |---|---|---|---|
  | Bình minh 5h-8h | `#2B2F6B` | `#F08A6C` | cam đào |
  | Ban ngày 8h-17h | `#1E4FA8` | `#5FA8E8` | xanh trời |
  | Hoàng hôn 17h-19h | `#3B1E5E` | `#F07A4A` | cam |
  | Đêm 19h-5h | `#070B1A` | `#1D1146` | tím xanh (như V1) |

- **Thời tiết trên nền** (chỉ ở trang chủ, tối đa 16 hạt): mưa là các vạch xanh rơi, đêm quang là sao nhấp nháy, nắng là vệt sáng xoay chậm ở góc, sương mù là dải mờ trôi ngang.
- **Thẻ (card)**: nền kính mờ (màu thẻ, độ mờ 70%), bo góc 16, viền 1 px sáng nhẹ.
- **Chữ**: vẫn là Montserrat tiếng Việt; thêm cỡ 36 cho số liệu lớn. Cỡ 48 chỉ dùng cho chữ số.
- **Biểu tượng**: bộ icon riêng 48x48 vẽ bằng script (như rồng), thay ký hiệu LV_SYMBOL ở launcher.
- **Chuyển động**: vào trang thì trượt và mờ dần trong 220 ms; nút nhấn co lại còn 95%; thông báo trượt từ trên xuống.

## 3. Khung chung

### 3.1 Thanh trạng thái (mọi màn hình)
Một dải cao 22 px ở trên cùng (lớp `lv_layer_top`):
```
 09:41                          [rồng đói]  AI  Wi-Fi ▂▄▆
```
- Bên trái là giờ (ẩn ở trang chủ vì đã có đồng hồ lớn).
- Bên phải là các chấm trạng thái: biểu tượng rồng khi rồng cần chăm (đói, bẩn, ốm), "AI" xanh khi trợ lý sẵn sàng, cột sóng Wi-Fi.

### 3.2 Trung tâm điều khiển (vuốt xuống từ mép trên)
```
+------------------------------------------------+
|  Độ sáng   [==========o-----]                  |
|  [ Wi-Fi: Zin ]  [ Không làm phiền ]  [ Ngủ ]  |
|  [ Màn hình chờ ] [ Cài đặt ]                  |
+------------------------------------------------+
```
Truy cập nhanh mà không phải vào Cài đặt. "Ngủ" là tắt màn hình ngay; "Không làm phiền" là tắt nhắc rồng và âm báo.

### 3.3 Thông báo (toast)
Một kiểu thống nhất: thẻ nhỏ trượt xuống dưới thanh trạng thái, tự ẩn sau 3 giây. Ví dụ: "Rồng đã tiến hóa!", "Đã lưu cài đặt", "Mất kết nối Wi-Fi".

## 4. Trang chủ V2

```
+--------------------------------------------------+
| (thanh trạng thái)                   AI  Wi-Fi   |
|                         Chào buổi sáng, Zin      |
|      (con rồng của      09:41                    |
|       bạn, đứng         Thứ Hai, 19 tháng 10     |
|       trên mây,         +-----------------------+|
|       thở, chớp mắt)    | (nắng) 28°  Hà Nội    ||
|   "Chạm để trò chuyện"  | 13h 30° | 16h 29° ... ||
|                         +-----------------------+|
|            o  o  o   (chấm trang)                |
+--------------------------------------------------+
```
- **Rồng thay khuôn mặt mascot**: dùng đúng sprite của cấp rồng hiện tại. Chạm vào rồng là nói chuyện với trợ lý AI; khi nghe, rồng có vòng sóng âm quanh đầu. Chạm giữ là mở game. Chưa có thẻ nhớ thì giữ khuôn mặt mascot như V1.
- **Rồng có lời thoại ngắn** (bong bóng) theo bối cảnh: buổi sáng chào, trời mưa nhắc mang ô, rồng đói thì kêu đói.
- **Thời tiết mở rộng**: nhiệt độ hiện tại cùng 3 mốc giờ tới (lấy thêm `hourly` từ Open-Meteo). Chạm vào thẻ mở trang Thời tiết chi tiết (dự báo 5 ngày).
- **Nền và hiệu ứng** theo giờ và thời tiết (mục 2).

## 5. Màn hình chờ (Ambient)

Sau thời gian rảnh do người dùng chọn (thay cho "tắt màn hình", có thể chọn một trong hai):
```
+--------------------------------------------------+
|                                                  |
|                 09:41                            |
|             Thứ Hai, 19/10   28°                 |
|                                                  |
|        (rồng ngủ cuộn tròn, Z z, tối dần)        |
+--------------------------------------------------+
```
- Đèn nền giảm còn 15%. Chữ đồng hồ đổi vị trí vài pixel mỗi phút.
- Chỉ vẽ lại mỗi phút một lần.
- Chạm bất kỳ chỗ nào để thức dậy. Ban đêm có thể tự vào màn hình chờ theo lịch.

## 6. Launcher V2

```
+--------------------------------------------------+
| Ứng dụng                                 09:41   |
|  [icon]    [icon]    [icon]    [icon]            |
|  Trợ lý    Rồng ảo•  Thời tiết  QR               |
|  [icon]    [icon]    [icon]    [icon]            |
|  Wi-Fi     Bộ nhớ    Cài đặt   Giới thiệu        |
+--------------------------------------------------+
```
- Icon riêng có màu và chiều sâu, chữ dưới icon (không còn nằm trong ô vuông).
- **Huy hiệu** (badge): chấm đỏ trên "Rồng ảo" khi rồng cần chăm.
- Thêm app **Thời tiết** (dự báo 5 ngày) và **Đồng hồ** (báo thức, hẹn giờ, bấm giờ; báo thức cần có loa).
- Bỏ "Âm nhạc" và "Ảnh" khỏi mặc định cho tới khi có nội dung thật. Riêng "Ảnh" có thể làm khung ảnh từ thẻ nhớ (ảnh JPG đặt sẵn đúng cỡ 480x320).

## 7. QR thanh toán V2

```
+--------------------------------------------------+
| Thanh toán                       MB Bank         |
|  +-----------+   Số tiền                         |
|  |  QR code  |   150.000 đ                       |
|  |           |   [1][2][3]                       |
|  |           |   [4][5][6]   [000]               |
|  +-----------+   [7][8][9]   [Xóa]               |
|   HO KINH DOANH  [  0  ]     [Tạo mã]            |
+--------------------------------------------------+
```
- **Nhập số tiền ngay trên máy**: thiết bị tự tạo mã VietQR động. Firmware thêm trường số tiền (tag 54) vào chuỗi EMV và tính lại CRC16, nên khách quét là đã có sẵn số tiền.
- Nút "Không nhập tiền" để quay về mã tĩnh.
- Mã có số tiền tự hết hạn sau 5 phút rồi về mã tĩnh.

## 8. Rồng ảo V2

- **Phòng của rồng**: nền thay theo giờ. Có đồ trang trí (đèn, gối, cây) mua bằng xu và đặt vào các vị trí cố định.
- **Rồng di chuyển**: đi qua lại trong phòng, quay đầu, vỗ cánh (đổi giữa 2 khung hình cánh).
- **Vuốt ve**: xoa tay lên rồng thì hiện tim bay lên và tăng điểm vui vẻ (giới hạn số lần mỗi giờ).
- **Hiệu ứng khi chăm**: khi ăn có thức ăn bay vào miệng, khi tắm có bọt xà phòng, khi tiến hóa có màn sáng lóe kèm pháo hoa.
- **Mini game thứ 2: "Rồng bay"**: chạm để vỗ cánh, bay tránh cột mây (kiểu Flappy), chỉ vẽ lại vùng nhỏ.
- **Thành tựu**: 12 huy hiệu (7 ngày liên tục, cho ăn 100 lần, kỷ lục 20 ngọc, đạt Rồng thần...).
- **Nhật ký rồng** (đọc trên web): dòng thời gian các lần tiến hóa và thành tựu.

## 9. Cài đặt và trang web V2

- Trang Cài đặt trên máy chia nhóm có icon. Thêm "Màn hình chờ", "Không làm phiền", "Tự chỉnh độ sáng" (nếu có LDR).
- Trang web thêm: chọn **chủ đề màu** (Bầu trời / Rừng / Hoàng hôn / Đơn sắc), bật/tắt hiệu ứng thời tiết, lịch màn hình chờ, chọn widget trang chủ (thời tiết / lịch âm / câu nói mỗi ngày).
- **Lịch âm** trên trang chủ (tính ngay trên máy, không cần mạng). Rất hợp với người dùng Việt.

## 10. Lộ trình

| Giai đoạn | Nội dung | Rủi ro |
|---|---|---|
| **V2.1 Nền tảng** | Nền theo giờ, thanh trạng thái, toast, trung tâm điều khiển | Thấp |
| **V2.2 Trang chủ** | Rồng trên trang chủ, bong bóng thoại, thời tiết theo giờ, hiệu ứng thời tiết, lịch âm | Trung bình (hiệu năng hiệu ứng) |
| **V2.3 Màn hình chờ** | Ambient, lịch tự vào màn hình chờ | Thấp |
| **V2.4 Launcher và app** | Bộ icon mới, huy hiệu, app Thời tiết và Đồng hồ | Trung bình (dung lượng flash cho icon) |
| **V2.5 QR động** | Nhập số tiền, tạo VietQR có tiền, kiểm tra CRC | Thấp, cần thử bằng app ngân hàng thật |
| **V2.6 Rồng ảo V2** | Phòng, di chuyển, vuốt ve, hiệu ứng, mini game Rồng bay, thành tựu | Cao nhất (nhiều đồ họa) |

Ngân sách flash thực tế: icon 91 KB, đồ trang trí và hiệu ứng 57 KB. Firmware dùng 80% flash (còn khoảng 620 KB).

## 11. Quyết định đã chốt

1. Rồng **thay hẳn** khuôn mặt mascot ở trang chủ (không có thẻ nhớ thì vẫn hiện mascot cũ).
2. Bo mạch **có loa**: âm báo, báo thức, tiếng rồng kêu (cổng SPEAK, GPIO26).
3. **Có** lịch âm và câu nói mỗi ngày trên trang chủ (tắt được trên web).
4. **Bỏ "Âm nhạc"**. **"Ảnh"** thành khung ảnh từ thẻ nhớ.
5. Làm **theo lộ trình** V2.1 đến V2.6.

## 12. Trạng thái triển khai

| Giai đoạn | Đã làm | Mã nguồn |
|---|---|---|
| V2.1 | Nền theo giờ, 4 chủ đề màu; thanh trạng thái (giờ, nhắc rồng, AI, sóng Wi-Fi); toast; trung tâm điều khiển (chạm hoặc vuốt thanh trạng thái): độ sáng, âm lượng, Wi-Fi, Im lặng, Tắt màn, Chờ, Cài đặt. Chạm đánh thức màn hình không bấm nhầm nút bên dưới | `theme.h`, `statusbar.h`, `sound.h`, `touch.h` |
| V2.2 | Trang chủ: rồng làm bạn đồng hành (chạm = trò chuyện, giữ = mở game), bong bóng thoại theo ngữ cảnh, lịch âm (Can Chi), thời tiết +2/+4/+6 giờ, câu nói mỗi ngày, hiệu ứng mưa/tuyết/sao/nắng/sương | `home.h`, `lunar.h`, `quotes.h`, `weather.cpp` |
| V2.3 | Màn hình chờ: đồng hồ lớn, rồng ngủ, đèn 15%, chữ dịch chuyển mỗi phút; vào sau thời gian rảnh hoặc theo lịch đêm | `ambient.h` |
| V2.4 | Launcher icon mới, chấm đỏ khi rồng cần chăm; app **Thời tiết** (12 giờ, 5 ngày), **Đồng hồ** (báo thức, hẹn giờ, bấm giờ), **Ảnh** (khung ảnh), **Thanh toán** | `tools/gen_icons.py`, `weather_page.h`, `clock_app.h`, `photos.h` |
| V2.5 | VietQR động: nhập số tiền, thêm tag 54, đặt tag 01 = 12, tính lại CRC16; hết hạn sau 5 phút | `qr_page.h` |
| V2.6 | Phòng rồng (cửa sổ ngày/đêm, sàn), 6 đồ trang trí mua bằng xu, rồng đi lại, xoa để vuốt ve (tối đa 20 lần/giờ tăng vui vẻ), hiệu ứng ăn/tắm/tiến hóa/nhặt xu, mini game **Rồng bay**, 12 huy hiệu, chuỗi ngày, nhật ký rồng (xem trên web) | `dragon.h`, `tools/gen_decor.py` |

Khác với bản thiết kế:
- **Rồng không quay đầu và không vỗ cánh** (cần thêm khoảng 500 KB sprite lật và khung cánh, vượt ngân sách flash). Thay vào đó rồng thở và đi lại trong phòng.
- Mỗi lần mua đồ trang trí đặt vào một vị trí cố định, không kéo thả.

Cách dùng nhanh:
- **Khung ảnh**: chép ảnh JPG (nên 480x320, kiểu baseline) vào thư mục `/photos` trên thẻ nhớ. Chạm phải/trái để chuyển ảnh, giữ để thoát. Thời gian mỗi ảnh chỉnh trên web.
- **Báo thức** đặt trong app Đồng hồ. Báo thức kêu cả khi đang Im lặng và tự tắt sau 2 phút.
- **Thanh toán**: nhập số tiền, bấm "Tạo mã QR". Nên quét thử bằng app ngân hàng thật trước khi dùng cho khách.
- **Công cụ phát triển** trên web (có mật khẩu nếu đã đặt): `GET /api/screenshot` trả ảnh BMP của màn hình; `/api/touch?x=&y=[&x2=&y2=&ms=]` giả lập chạm hoặc vuốt.

Huy hiệu (mỗi huy hiệu +20 xu): Phá vỏ, Lớn khôn (Rồng cấp 3), Trưởng thành, Thần long, Sành ăn (100 lần ăn), Sạch bong (50 lần tắm), Thợ săn ngọc (Bắt ngọc 20 điểm), Phi công (Rồng bay 10 điểm), Đại gia (1000 xu), Bạn thân (7 ngày liền), Nhà trang trí (đủ 6 món), Luân hồi (thế hệ 2).

