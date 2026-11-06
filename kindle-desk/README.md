# Kindle Desk

Kindle Oasis (đã jailbreak + KOReader) thành màn hình để bàn. **Kindle tự vẽ giao diện** bằng widget KOReader nên chạm là phản hồi tức thì và vẫn chạy khi mất mạng; server chỉ cung cấp dữ liệu (thời tiết, bộ thẻ, lịch âm, sự kiện, cài đặt) mỗi 15 phút.

```
https://kindle.zenix.vn  --/data.json (7KB, mỗi 15 phút), /sleep.png (mỗi 10 phút)-->  Kindle: KOReader + desk.koplugin
```

Có hai chế độ giao diện, đổi trong menu (mục "Giao diện"):
- **Tự vẽ trên Kindle** (mặc định): `kindledesk_native.lua`.
- **Ảnh từ server** (dự phòng): server render PNG, Kindle chỉ hiển thị (`kindledesk_view.lua`). Tự chuyển sang chế độ này nếu giao diện tự vẽ lỗi 2 lần liên tiếp.

## Các trang (vuốt lên/xuống hoặc chạm chấm tròn)
| Trang | Nội dung |
|---|---|
| Desk | Giờ lớn, ngày dương/âm, thời tiết (AQI, UV, 6 mốc giờ, 4 ngày), flashcard + dock |
| Lịch | Lịch tháng kèm ngày âm, pha trăng, đếm ngược sự kiện |
| Focus | Pomodoro: chạm để bắt đầu/huỷ, vòng tiến độ |
| Từ vựng | Flashcard toàn màn hình |
| Trích dẫn | Câu trích mỗi ngày, chạm để đổi |

Màn hình ngủ (nút nguồn): cập nhật lúc mấy giờ, pin, thời tiết mai, pha trăng, sự kiện, trích dẫn.

## Điều khiển
- Chạm **tên thứ** (trên trái): menu (trang, xoay màn hình, làm mới, màn hình ngủ, ngủ ngay, ngủ đêm, giao diện, thoát).
- Chạm **chấm tròn**: trang kế. **Thành phố**: tải lại dữ liệu. **Thẻ**: lật. **Tab dock**: đổi chiều học.
- Vuốt trái/phải hoặc nút lật trang: thẻ kế/trước (trang không có thẻ thì chuyển trang). Vuốt lên/xuống: chuyển trang.
- Thẻ tự lật/đổi đúng giờ (đặt lịch theo thời điểm lật), đồng hồ cập nhật mỗi phút ngay trên máy.

## Cài đặt trên web
`https://kindle.zenix.vn/settings?k=<TOKEN>`: thành phố, °C/°F, 12/24 giờ, giao diện sáng/tối (chỉ chế độ ảnh), âm lịch, các trang, thời gian thẻ, xáo trộn, danh sách thẻ, sự kiện, Pomodoro, trích dẫn. Lưu ở `/opt/kindle-desk/data/`; Kindle nhận ở lần tải dữ liệu kế tiếp hoặc khi chạm thành phố.

## Server (kindle.zenix.vn)
Chạy ở `/opt/kindle-desk` trên zenix-server (SSH cổng 8430), container bind `127.0.0.1:8095`, HTTPS qua OpenLiteSpeed (`deploy/kindle.zenix.vn.conf`), chứng chỉ certbot webroot tự gia hạn.

    tar czf - server.py desk.html settings.html config.json cards.json Dockerfile docker-compose.yml fonts | ssh -p 8430 zenix-server "tar xzf - -C /opt/kindle-desk"
    ssh -p 8430 zenix-server "cd /opt/kindle-desk && docker compose up -d --build"

API (cần `?k=TOKEN`, trừ `/healthz`): `/data.json`, `/sleep.png?w=&h=&bat=`, `/screen.png?p=&w=&h=&rot=&do=` (chế độ ảnh), `/settings`, `/api/settings`.
Server riêng chưa có web server: dùng `docker-compose.caddy.yml` + `Caddyfile`.

## Kindle
Chép `koplugin/desk.koplugin/` (gồm `icons/`) vào `/mnt/us/koreader/plugins/`, sửa `kindledesk_config.lua` (server + token), khởi động lại KOReader. Mở bằng mục **Kindle Desk** trong thư viện (`kindle/KindleDesk.sh.launcher`) hoặc KOReader: Tools -> More tools -> Kindle Desk.

- Nhật ký thời gian/lỗi: `/mnt/us/koreader/kindle_desk.log`. Cài đặt riêng của máy: `koreader/settings/kindle_desk.lua`; cache dữ liệu: `kindle_desk_data.json`.
- Khi Desk mở, plugin tạm trỏ screensaver của KOReader vào ảnh ngủ và khôi phục khi thoát (kể cả sau khi KOReader bị tắt đột ngột).

## Kiểm thử (không cần Kindle)
`tests/` dựng widget KOReader giả (đúng cách tính kích thước/vẽ theo mã nguồn KOReader v2026.07.1) để chạy `kindledesk_native.lua`:

    python tests/native_preview.py http://127.0.0.1:8080/data.json?k=TOKEN out_dir   # render bố cục 6 trang x dọc/ngang
    python tests/logic_test.py data.json                                            # lật thẻ, Pomodoro, xoay, menu, chuyển trang, dự phòng

Cần `pip install lupa pillow fonttools`. Icon trong ảnh xem trước là khung xám; chữ đo bằng font Noto thật.
