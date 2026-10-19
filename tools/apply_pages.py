"""One-off patch: move app pages out of main.cpp into pages.h, add screen-off timer."""
p = 'D:/Code/ESP32-Display/firmware/src/main.cpp'
s = open(p, encoding='utf-8').read()

a = s.index("// ---------- confirm dialog ----------")
b = s.index("// ---------- home (showcase) ----------")
s = s[:a] + '#include "pages.h"\n\n' + s[b:]

# load the screen-off timeout after the backlight is up
old = "  backlightInit();\n"
assert s.count(old) == 1
s = s.replace(old, old + "  {\n    Preferences p;\n    p.begin(\"ui\", true);\n    sleepMin = p.getUChar(\"sleep\", 0);\n    p.end();\n  }\n")

# screen-off in loop()
old = "void loop() {\n  wifiLoop();\n  lv_timer_handler();\n  delay(5);\n}"
assert s.count(old) == 1
s = s.replace(old, """// Turn the backlight off after the configured idle time; the first touch wakes it.
static void sleepTick() {
  static bool asleep = false;
  uint32_t idle = lv_disp_get_inactive_time(nullptr);
  if (!asleep && sleepMin && idle > (uint32_t)sleepMin * 60000UL) {
    asleep = true;
    ledcWrite(0, 0);
  } else if (asleep && idle < 1000) {
    asleep = false;
    ledcWrite(0, blLevel);
  }
}

void loop() {
  wifiLoop();
  lv_timer_handler();
  sleepTick();
  delay(5);
}""")
open(p, 'w', encoding='utf-8').write(s)
print("patched")
