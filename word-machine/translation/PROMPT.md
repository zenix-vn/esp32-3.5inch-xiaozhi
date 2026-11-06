# Prompt giao việc dịch từ điển

Copy toàn bộ phần trong khung dưới, điền tên phần được giao vào dòng đầu, rồi gửi cho AI.
Mỗi AI một phần khác nhau. Xem [STATUS.md](STATUS.md) để biết phần nào còn `chờ`.

---

Bạn dịch phần **P0xx** (thay bằng phần được giao, ví dụ P002) của dự án Việt hóa máy học từ vựng.

## Bối cảnh

Đây là máy học từ vựng cầm tay màn hình e-ink 2,66 inch, gốc Trung Quốc. Toàn bộ nghĩa và câu ví dụ đang là tiếng Trung. Dự án thay chúng bằng tiếng Việt. Từ tiếng Anh, phiên âm và âm thanh phát âm giữ nguyên, bạn không đụng tới.

## Việc cần làm

1. Đọc file nguồn `E:\ESP32-LCD\word-machine\translation\source\P0xx.jsonl`. Mỗi dòng là một mục từ.
2. Ghi bản dịch ra `E:\ESP32-LCD\word-machine\translation\done\P0xx.jsonl`.
3. Chạy kiểm tra, từ thư mục `E:\ESP32-LCD\word-machine\translation`:
   ```
   python tools/validate.py P0xx
   ```
4. Sửa đến khi báo `OK`. Dòng `over the soft limit` chỉ là cảnh báo, không chặn, nhưng nên rút gọn nếu dễ.
5. Mở `claims.csv`, thêm một dòng: `P0xx,<tên bạn>,<ngày bắt đầu>,<ngày xong>`

File có 300 mục. Nên làm theo đợt 50–75 mục rồi ghi nối tiếp, đừng cố xuất cả 300 trong một lần.

## Định dạng

Dữ liệu vào:

```json
{"id": "D00301", "word": "again", "ipa": "[əˈɡen]",
 "meaning_zh": ["adv.还，再一次,还原,同样地,而且,再说一遍"],
 "example_en": "The car's got some sort of engine trouble again.",
 "example_zh": "车子的引擎又出了什么故障.",
 "soft_max_bytes": 186}
```

Kết quả ra, **mỗi mục đúng một dòng JSON**, không xuống dòng giữa chừng, không thụt lề:

```json
{"id": "D00301", "meaning_vi": ["adv. lại, lần nữa"], "example_vi": "Xe lại gặp trục trặc động cơ."}
```

Chỉ có ba trường: `id`, `meaning_vi`, `example_vi`. Không chép lại `word`, `ipa`, `example_en`.

## Quy tắc dịch

1. **Nguồn dịch.** Nghĩa dịch từ `meaning_zh`, dùng `word` để hiểu đúng nghĩa tiếng Anh. Câu ví dụ dịch từ `example_en`, tham khảo thêm `example_zh`. **Khi tiếng Trung và tiếng Anh lệch nhau thì theo tiếng Anh.**
2. **Giữ đúng số dòng.** `meaning_vi` phải có đúng số phần tử như `meaning_zh`. Dòng gốc rỗng thì dòng dịch cũng rỗng.
3. **Giữ nhãn từ loại** ở đầu mỗi dòng, viết thường rồi một dấu cách: `n. `, `v. `, `adj. `, `adv. `, `prep. `, `pron. `, `conj. `, `int. `, `det. `, `aux. `, `num. `, `modal. `
4. **Ngắn gọn.** Màn hình chỉ rộng khoảng 210 điểm ảnh. Ngăn các nghĩa bằng dấu phẩy, gộp nghĩa trùng, ưu tiên nghĩa thông dụng. Được bỏ bớt nghĩa hiếm khi vượt `soft_max_bytes`.
5. **Câu ví dụ** dịch tự nhiên, không bám từng chữ. Nếu `example_zh` rỗng thì `example_vi` **phải** rỗng.
6. **Nhãn trong ngoặc** dịch sang tiếng Việt: `〈口〉` thành `(khẩu ngữ)`, `<美俚>` thành `(lóng Mỹ)`, `[航]` thành `(hàng không)`, `[植]` thành `(thực vật)`.
7. **Không để sót chữ Hán.** Dấu câu dùng kiểu ASCII: `, . ; : ! ? ( ) " ' -`. Tuyệt đối không dùng `，` `。` `；` `（` `）` `、` `…` kiểu Trung, không dùng emoji.
8. **Tên riêng** giữ nguyên hoặc phiên âm quen dùng. Tên người Trung Quốc viết Hán Việt: `小李` thành `Tiểu Lý`.
9. **Từ thô tục hoặc nhạy cảm** dịch trung thực, trung tính, như từ điển.
10. **Nhất quán** trong cả phần: cùng một thuật ngữ thì dùng cùng một cách dịch.

## Ví dụ mẫu

Nguồn:
```json
{"id": "D00002", "word": "cook", "ipa": "[kʊk]", "meaning_zh": ["v.烹饪，煮，烧", "n.厨师,做饭的人"], "example_en": "The cook hollowed out the melons.", "example_zh": "厨师把瓜掏空.", "soft_max_bytes": 122}
```

Đúng:
```json
{"id": "D00002", "meaning_vi": ["v. nấu ăn, nấu, nướng", "n. đầu bếp, người nấu ăn"], "example_vi": "Đầu bếp khoét ruột quả dưa."}
```

Sai và lý do:

| Viết sai | Lý do |
|---|---|
| `"meaning_vi": "v. nấu ăn; n. đầu bếp"` | Phải là mảng 2 phần tử, không gộp thành chuỗi |
| `"meaning_vi": ["v. nấu ăn, nấu, nướng"]` | Thiếu dòng thứ hai, số dòng không khớp |
| `"meaning_vi": ["nấu ăn, nấu, nướng", "đầu bếp"]` | Mất nhãn từ loại |
| `"meaning_vi": ["v. nấu ăn，nấu", ...]` | Dùng dấu phẩy kiểu Trung |
| `"example_vi": "Người đầu bếp đã khoét rỗng những quả dưa đó."` | Dài dòng không cần thiết |

## Tuyệt đối không làm

- Không sửa gì trong `source/`, `tools/`, `backup/`, `vi-test/`, `out/`
- Không chạy `tools/build.py` và không ghi vào thẻ nhớ
- Không nhận phần khác ngoài phần được giao
- Nếu thấy dữ liệu nguồn có lỗi, ghi lại trong báo cáo của bạn, đừng tự sửa

## Báo cáo khi xong

Nêu rõ: phần đã làm, kết quả `validate.py`, số mục vượt `soft_max_bytes`, và những chỗ bạn không chắc.
