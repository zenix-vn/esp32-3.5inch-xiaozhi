#pragma once
// Payment app (V2.5): type an amount and the device turns the saved VietQR code into a dynamic one carrying
// that amount (EMV tag 54, point of initiation 12, CRC16 recomputed), so the customer's bank app pre-fills it.
// The amount expires after 5 minutes and the code goes back to the static one.

static uint16_t emvCrc(const String &s) {
  uint16_t c = 0xFFFF;
  for (size_t i = 0; i < s.length(); i++) {
    c ^= (uint8_t)s[i] << 8;
    for (int k = 0; k < 8; k++) c = c & 0x8000 ? (c << 1) ^ 0x1021 : c << 1;
  }
  return c;
}

// Rebuilds the EMV string with `amount` (VND, digits only; empty = static code). Returns "" if the base
// payload is not a valid EMV TLV string.
static String vietQrWithAmount(const String &base, const String &amount) {
  String out;
  bool hasAmount = amount.length() > 0;
  bool inserted = false;
  size_t i = 0;
  while (i + 4 <= base.length()) {
    String tag = base.substring(i, i + 2);
    int len = base.substring(i + 2, i + 4).toInt();
    if (i + 4 + len > base.length()) return "";
    String val = base.substring(i + 4, i + 4 + len);
    i += 4 + len;
    if (tag == "63" || tag == "54") continue;  // CRC and old amount are rebuilt
    if (tag == "01") val = hasAmount ? "12" : "11";
    if (hasAmount && !inserted && tag.toInt() > 54) {
      char b[6];
      snprintf(b, sizeof(b), "54%02d", (int)amount.length());
      out += b;
      out += amount;
      inserted = true;
    }
    char h[5];
    snprintf(h, sizeof(h), "%s%02d", tag.c_str(), len);
    out += h;
    out += val;
  }
  if (i != base.length()) return "";
  if (hasAmount && !inserted) {
    char b[6];
    snprintf(b, sizeof(b), "54%02d", (int)amount.length());
    out += b;
    out += amount;
  }
  out += "6304";
  char crc[5];
  snprintf(crc, sizeof(crc), "%04X", emvCrc(out));
  return out + crc;
}

static String vndFormat(const String &digits) {
  String out;
  int n = digits.length();
  for (int i = 0; i < n; i++) {
    out += digits[i];
    int rest = n - 1 - i;
    if (rest > 0 && rest % 3 == 0) out += '.';
  }
  return out;
}

static lv_obj_t *payQr, *payAmountLbl, *payState;
static String payDigits;
static bool payActive = false;  // a code with an amount is shown
static uint32_t payExpire = 0;

static void payRender() {
  String base = cfg.qrPayload.length() ? cfg.qrPayload : String(QR_DEFAULT_PAYLOAD);
  String code = payActive ? vietQrWithAmount(base, payDigits) : base;
  if (!code.length()) code = base;
  lv_qrcode_update(payQr, code.c_str(), code.length());
  lv_label_set_text_fmt(payAmountLbl, "%s đ", payDigits.length() ? vndFormat(payDigits).c_str() : "0");
  if (payActive)
    lv_label_set_text_fmt(payState, isVi() ? "Mã có số tiền %s đ" : "Code for %s VND", vndFormat(payDigits).c_str());
  else
    lv_label_set_text(payState, DL("Mã tĩnh (khách tự nhập tiền)", "Static code (payer types the amount)"));
  lv_obj_set_style_text_color(payState, payActive ? C_OK : C_MUTED, 0);
}

static void payKey(lv_event_t *e) {
  lv_obj_t *m = lv_event_get_target(e);
  const char *k = lv_btnmatrix_get_btn_text(m, lv_btnmatrix_get_selected_btn(m));
  if (!k) return;
  if (!strcmp(k, LV_SYMBOL_BACKSPACE)) {
    if (payDigits.length()) payDigits.remove(payDigits.length() - 1);
  } else if (payDigits.length() + strlen(k) <= 11) {
    if (!(payDigits.length() == 0 && k[0] == '0')) payDigits += k;  // no leading zeros
  }
  payActive = false;  // editing goes back to the static code until "Create" is tapped
  payRender();
}

static void pageQr() {
  lv_obj_t *c;
  lv_obj_t *s = pageBegin("Payment", &c);
  lv_obj_del(c);
  payDigits = "";
  payActive = false;

  lv_obj_t *card = lv_obj_create(s);
  lv_obj_remove_style_all(card);
  lv_obj_set_size(card, 204, 204);
  lv_obj_set_pos(card, 12, SB_H + 52);
  lv_obj_set_style_radius(card, 16, 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  payQr = lv_qrcode_create(card, 188, lv_color_hex(0x000000), lv_color_hex(0xFFFFFF));
  lv_obj_center(payQr);
  lv_obj_t *nm = makeLabel(s, cfg.qrName.c_str(), F_CAPTION, C_MUTED);
  lv_obj_set_width(nm, 204);
  lv_label_set_long_mode(nm, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(nm, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_pos(nm, 12, SB_H + 260);
  lv_obj_t *bank = makeLabel(s, cfg.qrBank.c_str(), F_CAPTION, C_ACCENT);
  lv_obj_align(bank, LV_ALIGN_TOP_RIGHT, -14, SB_H + 14);

  payAmountLbl = makeLabel(s, "", F_HEADING, C_TEXT);
  lv_obj_set_width(payAmountLbl, 244);
  lv_obj_set_style_text_align(payAmountLbl, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(payAmountLbl, 224, SB_H + 50);
  payState = makeLabel(s, "", F_CAPTION, C_MUTED);
  lv_obj_set_width(payState, 244);
  lv_obj_set_style_text_align(payState, LV_TEXT_ALIGN_RIGHT, 0);
  lv_label_set_long_mode(payState, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(payState, 224, SB_H + 84);

  static const char *keys[] = {"1", "2", "3", "\n", "4", "5", "6", "\n", "7", "8", "9", "\n", "000", "0", LV_SYMBOL_BACKSPACE, ""};
  lv_obj_t *kp = lv_btnmatrix_create(s);
  lv_btnmatrix_set_map(kp, keys);
  lv_obj_set_size(kp, 244, 150);
  lv_obj_set_pos(kp, 224, SB_H + 104);
  lv_obj_set_style_bg_opa(kp, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(kp, 0, 0);
  lv_obj_set_style_pad_all(kp, 0, 0);
  lv_obj_set_style_pad_gap(kp, 6, 0);
  lv_obj_set_style_text_font(kp, F_TITLE, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(kp, C_CARD, LV_PART_ITEMS);
  lv_obj_set_style_text_color(kp, C_TEXT, LV_PART_ITEMS);
  lv_obj_set_style_border_width(kp, 1, LV_PART_ITEMS);
  lv_obj_set_style_border_color(kp, C_BORDER, LV_PART_ITEMS);
  lv_obj_set_style_shadow_width(kp, 0, LV_PART_ITEMS);
  lv_obj_set_style_radius(kp, 12, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(kp, C_BORDER, LV_PART_ITEMS | LV_STATE_PRESSED);
  lv_obj_add_event_cb(kp, payKey, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_t *clr = smallBtn(s, DL("Mã tĩnh", "Static"), C_WARN, [](lv_event_t *) {
    payDigits = "";
    payActive = false;
    payRender();
  }, nullptr, 96);
  lv_obj_set_height(clr, 36);
  lv_obj_set_pos(clr, 224, SB_H + 258);
  lv_obj_t *go = smallBtn(s, DL("Tạo mã QR", "Create QR"), C_OK, [](lv_event_t *) {
    if (!payDigits.length()) {
      uiToast(DL("Nhập số tiền trước", "Type an amount first"), C_WARN);
      return;
    }
    payActive = true;
    payExpire = millis() + 5 * 60000UL;
    payRender();
    soundPlay(SND(SND_OK), 0, false);
  }, nullptr, 142);
  lv_obj_set_height(go, 36);
  lv_obj_set_pos(go, 326, SB_H + 258);

  pageTimer(s, [](lv_timer_t *) {
    if (payActive && (int32_t)(millis() - payExpire) >= 0) {
      payActive = false;
      payDigits = "";
      payRender();
      uiToast(DL("Mã có số tiền đã hết hạn", "The amount code expired"));
    }
  }, 1000);
  payRender();
  pageShow(s);
}
