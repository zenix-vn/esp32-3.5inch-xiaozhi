# Nuôi rồng ảo (Dragon Pet) - thiết kế và kịch bản

Game nuôi thú ảo thời gian thực chạy trên Zin100 (ESP32-2432S035). Mở từ trang **Ứng dụng > Rồng ảo** (tiếng Anh: *Dragon*).
Dữ liệu được ghi vào **thẻ nhớ microSD**: `/zin100/dragon.json`. Không có thẻ nhớ thì game không chơi được và hiện thông báo yêu cầu cắm thẻ.

- Mã nguồn: `firmware/src/dragon.h`
- Cấu hình qua web: mục **Nuôi rồng ảo** tại `http://zin100.local`

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

Mục **Nuôi rồng ảo** tại `http://zin100.local`: xem cấp độ, tiến độ, xu, thế hệ, kỷ lục; **đặt tên rồng**; **nuôi lại từ đầu** (xóa dữ liệu rồng).

API: `POST /api/dragon` với `name=...`, `reset=1`, hoặc `speed=N` (nhân tốc độ lớn lên, chỉ để thử nghiệm, tự về 1 sau khi khởi động lại; ví dụ `speed=300` tiến hóa nhanh gấp 300 lần).

## 10. Ý tưởng mở rộng

- Âm thanh/giọng nói: cho rồng "nói" qua trợ lý Xiaozhi.
- Thêm mini game (nhớ hình, bắn lửa), thức ăn theo mùa.
- Huy hiệu thành tựu (nuôi 7 ngày liên tục, đạt Rồng vương...).
- Trang bị phụ kiện (mũ, khăn) mua bằng xu.
