#include "i18n.h"
#include <Preferences.h>
#include <string.h>

uint8_t gLang = LANG_VI;

struct Pair {
  const char *en, *vi;
};

static const Pair TABLE[] = {
    // home
    {"No Wi-Fi", "Chưa có Wi-Fi"},
    {"Offline", "Ngoại tuyến"},
    {"Xiaozhi online", "AI sẵn sàng"},
    {"Pairing", "Đang ghép nối"},
    {"Connecting...", "Đang kết nối..."},
    {"Retrying", "Đang thử lại"},
    {"Pair this device", "Ghép nối thiết bị"},
    {"Enter this code at xiaozhi.me > Add device", "Nhập mã này tại xiaozhi.me > Thêm thiết bị"},
    {"Good morning", "Chào buổi sáng"},
    {"Good afternoon", "Chào buổi chiều"},
    {"Good evening", "Chào buổi tối"},
    {"Good night", "Chúc ngủ ngon"},
    {"Hello", "Xin chào"},
    {"Syncing time...", "Đang đồng bộ giờ..."},
    {"Waiting for network", "Đang chờ mạng"},
    {"Device linked to xiaozhi.me", "Đã liên kết với xiaozhi.me"},
    {"Listening...", "Đang nghe..."},
    {"Tap the face to talk", "Chạm để trò chuyện"},
    {"Weather unavailable", "Không có dữ liệu thời tiết"},
    {"Loading weather...", "Đang tải thời tiết..."},
    {"Updating...", "Đang cập nhật..."},
    // weather
    {"Clear", "Trời quang"},
    {"Mostly clear", "Ít mây"},
    {"Partly cloudy", "Mây rải rác"},
    {"Overcast", "Nhiều mây"},
    {"Fog", "Sương mù"},
    {"Drizzle", "Mưa phùn"},
    {"Rain", "Mưa"},
    {"Snow", "Tuyết"},
    {"Showers", "Mưa rào"},
    {"Snow showers", "Tuyết rơi"},
    {"Thunderstorm", "Dông"},
    {"Cloudy", "Có mây"},
    // qr
    {"Scan to pay", "Quét để thanh toán"},
    {"Account name", "Tên tài khoản"},
    {"Account number", "Số tài khoản"},
    {"Open your banking app and scan the code", "Mở ứng dụng ngân hàng và quét mã"},
    // apps
    {"Apps", "Ứng dụng"},
    {"Assistant", "Trợ lý"},
    {"Music", "Âm nhạc"},
    {"Games", "Trò chơi"},
    {"Dragon", "Luyện rồng"},
    {"Weather", "Thời tiết"},
    {"Clock", "Đồng hồ"},
    {"Payment", "Thanh toán"},
    {"Calendar", "Lịch"},
    {"Sleep sounds", "Ru ngủ"},
    {"Games", "Trò chơi"},
    {"Radio", "Radio"},
    {"Reincarnate?", "Luân hồi?"},
    {"Your divine dragon leaves a new egg. You keep your coins and the next dragon grows faster.",
     "Rồng thần để lại một quả trứng mới. Bạn giữ nguyên xu và rồng kế tiếp sẽ lớn nhanh hơn."},
    {"Photos", "Ảnh"},
    {"Storage", "Bộ nhớ"},
    {"Settings", "Cài đặt"},
    {"About", "Giới thiệu"},
    {"Coming soon", "Sắp ra mắt"},
    {"%s is not available yet", "%s chưa khả dụng"},
    // settings
    {"DISPLAY", "MÀN HÌNH"},
    {"Brightness", "Độ sáng"},
    {"Turn screen off after", "Tắt màn hình sau"},
    {"When idle for", "Khi không dùng trong"},
    {"Then", "Thì chuyển sang"},
    {"Turn screen off", "Tắt màn hình"},
    {"Ambient clock", "Màn hình chờ"},
    {"Never", "Không"},
    {"1 min", "1 phút"},
    {"5 min", "5 phút"},
    {"10 min", "10 phút"},
    {"GENERAL", "CHUNG"},
    {"Time zone", "Múi giờ"},
    {"Language", "Ngôn ngữ"},
    {"Not connected", "Chưa kết nối"},
    {"Connected", "Đã kết nối"},
    {"Web setup", "Cấu hình web"},
    {"SYSTEM", "HỆ THỐNG"},
    {"Recalibrate touch", "Hiệu chuẩn cảm ứng"},
    {"Restart device", "Khởi động lại thiết bị"},
    {"Factory reset", "Khôi phục cài đặt gốc"},
    {"Change language?", "Đổi ngôn ngữ?"},
    {"The device restarts to apply the new language.", "Thiết bị sẽ khởi động lại để áp dụng ngôn ngữ mới."},
    // dialogs
    {"Cancel", "Hủy"},
    {"Confirm", "Xác nhận"},
    {"Forget Wi-Fi?", "Quên mạng Wi-Fi?"},
    {"The device restarts and opens the Zin100-Setup hotspot.", "Thiết bị sẽ khởi động lại và mở hotspot Zin100-Setup."},
    {"Recalibrate touch?", "Hiệu chuẩn lại cảm ứng?"},
    {"The device restarts and shows 4 targets to tap.", "Thiết bị khởi động lại và hiện 4 điểm để bạn chạm."},
    {"Restart?", "Khởi động lại?"},
    {"The device will reboot now.", "Thiết bị sẽ khởi động lại ngay."},
    {"Factory reset?", "Khôi phục cài đặt gốc?"},
    {"Erases Wi-Fi, touch calibration and all settings, then restarts.",
     "Xóa Wi-Fi, hiệu chuẩn cảm ứng và mọi cài đặt rồi khởi động lại."},
    // wi-fi
    {"Could not connect. Check the password and try again.", "Không kết nối được. Kiểm tra mật khẩu rồi thử lại."},
    {"Done", "Xong"},
    {"Try again", "Thử lại"},
    {"Connecting to %s...", "Đang kết nối tới %s..."},
    {"Networks", "Mạng"},
    {"None found", "Không tìm thấy"},
    {"Scan", "Quét"},
    {"Failed, tap refresh", "Lỗi, chạm để quét lại"},
    {"Scanning...", "Đang quét..."},
    {"Hotspot Zin100-Setup is open", "Hotspot Zin100-Setup đang mở"},
    {"Forget", "Quên"},
    {"Available networks", "Mạng khả dụng"},
    {"Password", "Mật khẩu"},
    {"Wi-Fi: starting", "Wi-Fi: đang khởi động"},
    {"Join Wi-Fi %s, open 192.168.4.1", "Kết nối Wi-Fi %s, mở 192.168.4.1"},
    // storage
    {"No card detected", "Không phát hiện thẻ"},
    {"%d%% used", "Đã dùng %d%%"},
    {"Capacity", "Dung lượng"},
    {"Used", "Đã dùng"},
    {"Free", "Còn trống"},
    // about
    {"HARDWARE", "PHẦN CỨNG"},
    {"Board", "Bo mạch"},
    {"Memory", "Bộ nhớ"},
    {"Display", "Màn hình"},
    {"Touch", "Cảm ứng"},
    {"Resistive XPT2046", "Điện trở XPT2046"},
    {"3.5-inch 480x320 ST7796", "3,5 inch 480x320 ST7796"},
    {"No card", "Không có thẻ"},
    {"NETWORK", "MẠNG"},
    {"IP address", "Địa chỉ IP"},
    {"Uptime", "Thời gian chạy"},
    {"Memory now", "RAM hiện tại"},
    {"Last restart", "Lần khởi động trước"},
    {"Firmware %s  -  built %s", "Phần mềm %s  -  biên dịch %s"},
    {"%d MHz, %d cores", "%d MHz, %d nhân"},
    {"%u MB flash, %u KB RAM", "%u MB flash, %u KB RAM"},
    {"%u KB free", "Còn trống %u KB"},
    {"Power on", "Bật nguồn"},
    {"Software restart", "Khởi động lại phần mềm"},
    {"Crash", "Lỗi treo máy"},
    {"Brownout", "Sụt áp"},
    {"Deep sleep", "Ngủ sâu"},
    {"Other", "Khác"},
    // touch calibration
    {"Touch calibration\nTap each circle", "Hiệu chuẩn cảm ứng\nChạm vào từng vòng tròn"},
    // xiaozhi status
    {"Waiting for Wi-Fi", "Đang chờ Wi-Fi"},
    {"Contacting Xiaozhi...", "Đang kết nối Xiaozhi..."},
    {"Server error %d, retrying", "Lỗi máy chủ %d, đang thử lại"},
    {"Bad server reply, retrying", "Máy chủ trả lời lỗi, đang thử lại"},
    {"Activated", "Đã kích hoạt"},
    {"Enter the code at xiaozhi.me", "Nhập mã tại xiaozhi.me"},
};

bool isVi() { return gLang == LANG_VI; }

const char *tr(const char *en) {
  if (gLang != LANG_VI || !en || !*en) return en;
  for (const Pair &p : TABLE)
    if (strcmp(p.en, en) == 0) return p.vi;
  return en;
}

void i18nLoad() {
  Preferences p;
  p.begin("ui", true);
  gLang = p.getUChar("lang", LANG_VI) == LANG_EN ? LANG_EN : LANG_VI;
  p.end();
}

void i18nSave(uint8_t lang) {
  Preferences p;
  p.begin("ui", false);
  p.putUChar("lang", lang);
  p.end();
  gLang = lang;
}
