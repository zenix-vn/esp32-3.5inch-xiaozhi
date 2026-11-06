# Dịch từ điển máy học từ sang tiếng Việt

Tài liệu này dành cho mọi AI và người tham gia dịch. Đọc hết mục "Cách nhận và làm một phần" và "Quy tắc dịch" trước khi bắt đầu.

## Mục tiêu

Máy học từ 乐爱斯 S8 hiển thị nghĩa và câu ví dụ bằng tiếng Trung. Dự án thay toàn bộ nội dung đó bằng tiếng Việt. Giao diện đã được Việt hóa riêng, không thuộc phạm vi tài liệu này.

Từ tiếng Anh, phiên âm, câu ví dụ tiếng Anh và âm thanh MP3 được giữ nguyên.

## Khối lượng

| Nhóm | Phần | Số mục | Nội dung |
|---|---|---:|---|
| A | P001–P028 | 8.369 | Từ có trong các sách trên máy, xếp theo mức xuất hiện, **làm đầu tiên** |
| T | T001 | 242 | Tên sách và tên chương còn tiếng Trung |
| G | G001 | 1.196 | Nghĩa của cụm từ lưu riêng trong sách |
| B | P029–P123 | 28.524 | Từ chỉ có trong từ điển, không thuộc sách nào |
| | **Tổng** | **38.331** | |

Danh sách đầy đủ để tra cứu: [all_entries.csv](all_entries.csv). Mở được bằng Excel. Tiến độ hiện tại: [STATUS.md](STATUS.md).

## Thứ tự thực hiện

1. **Giai đoạn 1:** dịch nhóm A, 28 phần.
2. **Giai đoạn 2:** dịch T001 và G001.
3. **Giai đoạn 3:** thử trên máy thật (xem "Việc phải thử trên máy").
4. **Giai đoạn 4:** dịch nhóm B, 95 phần. Có thể dừng ở bước này nếu kết quả thử cho thấy máy không hiển thị các từ nhóm B.
5. **Giai đoạn 5:** kiểm tra toàn bộ, dựng file và nạp một lần.

Chỉ nạp vào máy sau khi tài liệu dịch đã hoàn tất, trừ phép thử ở giai đoạn 3.

## Cấu trúc thư mục

```
translation/
  README.md          tài liệu này
  STATUS.md          tiến độ, sinh tự động
  claims.csv         ai đang nhận phần nào
  all_entries.csv    toàn bộ mục từ để tra cứu
  source/            dữ liệu gốc, KHÔNG sửa
  done/              bản dịch, mỗi phần một file cùng tên với source
  tools/             validate.py, progress.py, build.py
```

## Giao việc cho AI khác

Dùng [PROMPT.md](PROMPT.md): copy phần trong khung, điền tên phần được giao, gửi cho AI. Mỗi AI một phần khác nhau.

## Cách nhận và làm một phần

1. Xem [STATUS.md](STATUS.md), chọn phần đầu tiên đang ở trạng thái `chờ`, theo thứ tự trong bảng.
2. Thêm một dòng vào [claims.csv](claims.csv): `P001,tên-agent,2026-11-06,`. Mỗi phần chỉ một người nhận.
3. Đọc `source/P001.jsonl`. Mỗi phần có khoảng 300 mục, mỗi dòng là một mục.
4. Ghi kết quả vào `done/P001.jsonl`, giữ nguyên thứ tự, không bỏ mục nào.
5. Chạy kiểm tra, từ thư mục `translation/`:

   ```
   python tools/validate.py P001
   ```

6. Sửa đến khi báo `OK`. Dòng cảnh báo "over the soft limit" không chặn, nhưng hãy rút gọn nếu được.
7. Điền ngày vào cột `finished`, rồi chạy `python tools/progress.py`.

Không sửa file trong `source/` hay `tools/`. Nếu phát hiện lỗi ở dữ liệu gốc, ghi vào phần báo cáo công việc của bạn, đừng tự sửa.

## Định dạng

### Nhóm A và B: file `P*.jsonl`

Dữ liệu vào:

```json
{"id": "D00001", "word": "work", "ipa": "[wɜːk]",
 "meaning_zh": ["v.（使）工作，干活,从事…工作,用功", "n.工作，职业,职责,作品"],
 "example_en": "He did the work without effort.",
 "example_zh": "他很轻松地就把工作做好了.",
 "soft_max_bytes": 480}
```

Kết quả ra, mỗi mục một dòng JSON, không xuống dòng giữa chừng:

```json
{"id": "D00001", "meaning_vi": ["v. làm việc, hoạt động, có tác dụng", "n. công việc, nghề nghiệp, tác phẩm"], "example_vi": "Anh ấy hoàn thành công việc một cách dễ dàng."}
```

### Nhóm T và G

```json
{"id": "T0002", "vi": "Oxford Thượng Hải L1 kỳ 2"}
```

`T001` có sẵn trường `draft` để tham khảo, và giới hạn **cứng** `max_bytes`. `G001` có giới hạn cứng `max_chars`. Vượt giới hạn là lỗi.

## Quy tắc dịch

1. **Nguồn dịch.** Nghĩa dịch từ `meaning_zh`, dùng `word` để hiểu đúng nghĩa tiếng Anh. Câu ví dụ dịch từ `example_en`, đối chiếu thêm `example_zh`. Khi tiếng Trung và tiếng Anh lệch nhau, theo tiếng Anh.
2. **Giữ số dòng.** `meaning_vi` phải có đúng số phần tử như `meaning_zh`. Nếu một dòng gốc rỗng thì dòng dịch cũng rỗng.
3. **Giữ nhãn từ loại** ở đầu dòng, viết `n.`, `v.`, `adj.`, `adv.`, `prep.`, `pron.`, `conj.`, `int.`, `det.`, theo sau là một dấu cách.
4. **Ngắn gọn.** Màn hình rất nhỏ, chỉ hơn hai trăm điểm ảnh ngang. Dùng dấu phẩy ngăn các nghĩa, gộp nghĩa trùng, ưu tiên nghĩa thông dụng. Được bỏ nghĩa hiếm khi vượt `soft_max_bytes`.
5. **Câu ví dụ.** Dịch tự nhiên, không bám từng chữ. Nếu `example_zh` rỗng thì `example_vi` cũng phải rỗng.
6. **Nhãn trong ngoặc của bản gốc** dịch sang tiếng Việt: `〈口〉` thành `(khẩu ngữ)`, `<美俚>` thành `(tiếng lóng Mỹ)`, `[航]` thành `(hàng không)`.
7. **Không để sót chữ Hán.** Dấu câu dùng kiểu ASCII: `, . ; : ! ? ( ) " ' -`. Không dùng `，。；（）「」、…` kiểu Trung, không dùng emoji.
8. **Tên riêng** giữ nguyên hoặc phiên âm quen dùng. Tên người Trung Quốc viết theo Hán Việt, ví dụ `小李` thành `Tiểu Lý`.
9. **Từ nhạy cảm hoặc thô tục** dịch trung thực, trung tính, như từ điển.
10. **Nhất quán.** Cùng một thuật ngữ thì dùng cùng một cách dịch trong toàn phần.

## Cách chữ Việt hiện trên máy (quan trọng với người dịch)

Người dịch chỉ viết tiếng Việt thường, có dấu đầy đủ. `build.py` tự vẽ lại thành các ô riêng, nên **không cần và không được** tìm cách tránh chữ có dấu.

- Máy dựng mọi ký tự ngoài ASCII trong ô cố định 14 điểm ảnh, và một số chữ Latin mở rộng (ô, ú, ũ, ờ, ớ, ộ, ệ, ừ) bị hiện thành khối nhiễu. Vì vậy chữ Việt không đi qua font của máy.
- `tools/cells.py` vẽ chữ bằng Consolas 12, mỗi chữ rộng đúng 7 điểm ảnh, **hai chữ một ô**. Mỗi cặp chữ khác nhau được gán một chữ Hán hiếm không có trong dữ liệu của máy, và nét vẽ của cặp được ghi vào ô font của chữ Hán đó.
- Nhãn từ loại (`n.`, `v.`, `adj.`...) giữ nguyên ASCII.
- Hệ quả cho người dịch: câu **không cần ngắt dòng**, máy tự ngắt (đôi khi giữa từ, như ở phần tiếng Anh). Dấu cách, dấu phẩy, số, chữ hoa đều dùng bình thường.
- Dung lượng: mỗi hai chữ tốn 3 byte, nên bản Việt lớn hơn tiếng Trung gốc khoảng 36%. Vì vậy `build.py` **dời cả từ điển ra cuối file** (file dài thêm khoảng 5,5MB, `make_card.py` nối dài chuỗi FAT và cập nhật kích thước trong thư mục gốc). Giới hạn `soft_max_bytes` vẫn chỉ là cảnh báo.

## Việc đã thử trên máy thật và việc còn mở

Đã thấy chạy (2026-11-05): máy nhận thẻ 16GB, đọc đúng bảng chỉ mục khi độ dài mục thay đổi, và hiện tiếng Việt từ các ô riêng của chữ Hán.
Chưa thử trên máy: **từ điển dời ra cuối file**. Nạp lần đầu ngày 2026-11-06. Nếu máy báo lỗi hoặc chữ lộn xộn ở mọi mục, hoàn lại bằng ảnh `../vi-test/card-last-good-p001.img`.

Còn mở:
1. Nhóm B (28.524 mục) có hiện trên máy không. Máy không có chức năng tra từ tự do, nên có thể bỏ nhóm B.
2. Các nhãn menu cố định của máy (chuỗi Hán trong firmware) dùng chung chữ Hán với nội dung khác nên chỉ Việt hóa được từng phần. Xem `../tools/ui_map.py`.
3. 65 tên sách hiếm còn tiếng Trung, ghi trong `out/titles_left.txt` sau mỗi lần dựng. Có thể dịch tay vào `done/T001.jsonl` theo dạng `{"zh": "tên gốc", "vi": "bản dịch"}`; bản dịch phải vừa số byte của tên gốc.

## Dựng file và nạp vào thẻ

Dành cho người vận hành, người dịch không cần làm.

```
python tools/build.py out/res.bin
python tools/make_card.py out/res.bin ../backup/card-restore.img out/card.img
python tools/simulate.py out/res.bin out/sim.png work water --titles   # xem trước màn hình
```

- Ghi vào thẻ **chỉ bằng ảnh thẻ, ghi trực tiếp theo sector**. Tuyệt đối không chép file qua Windows: file phải nằm liền một khối từ cluster 5 (LBA 8248), nếu lệch máy sẽ hiện toàn khối đen.
- Bản gốc nằm ở `../backup/`: `Gres260909.bin` và `card-restore.img`. Khóa giải mã đã sửa là `key_fixed.npy`, đừng dùng `key.npy` cũ vì sai 2 byte.
- Sau khi cắm thẻ vào máy, giữ nút tròn đỏ khoảng 15 giây để máy nhận lại thẻ.
