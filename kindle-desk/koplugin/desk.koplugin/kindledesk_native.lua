--[[--
Kindle Desk, drawn on the Kindle itself with KOReader widgets.

Clock, flashcards, Pomodoro, calendar, quotes, menu and page switching use local state only, so every
tap is instant and the desk keeps working offline. The server is only asked for /data.json (weather,
cards, lunar calendar, events, settings) every few minutes, plus /sleep.png for the sleep screen.

Touch: weekday (top left) = menu, dots = next page, city = refresh data, card = flip, dock = card direction.
Swipe left/right = next/previous card (or page); up/down = page. Page-turn keys = next/previous card.
--]]--

local Blitbuffer = require("ffi/blitbuffer")
local CenterContainer = require("ui/widget/container/centercontainer")
local DataStorage = require("datastorage")
local Device = require("device")
local Event = require("ui/event")
local Font = require("ui/font")
local FrameContainer = require("ui/widget/container/framecontainer")
local Geom = require("ui/geometry")
local GestureRange = require("ui/gesturerange")
local HorizontalGroup = require("ui/widget/horizontalgroup")
local HorizontalSpan = require("ui/widget/horizontalspan")
local ImageWidget = require("ui/widget/imagewidget")
local InputContainer = require("ui/widget/container/inputcontainer")
local LeftContainer = require("ui/widget/container/leftcontainer")
local LineWidget = require("ui/widget/linewidget")
local NetworkMgr = require("ui/network/manager")
local OverlapGroup = require("ui/widget/overlapgroup")
local RightContainer = require("ui/widget/container/rightcontainer")
local TextBoxWidget = require("ui/widget/textboxwidget")
local TextWidget = require("ui/widget/textwidget")
local UIManager = require("ui/uimanager")
local VerticalGroup = require("ui/widget/verticalgroup")
local VerticalSpan = require("ui/widget/verticalspan")
local json = require("json")
local lfs = require("libs/libkoreader-lfs")
local ltn12 = require("ltn12")
local socket = require("socket")
local socketutil = require("socketutil")
local Screen = Device.screen

local PLUGIN_DIR = (debug.getinfo(1, "S").source or ""):match("^@(.*)/[^/]*$") or "/mnt/us/koreader/plugins/desk.koplugin"
local ICON_DIR = PLUGIN_DIR .. "/icons/"
local TMP_MOON, TMP_RING = "/tmp/kindle_desk_moon.svg", "/tmp/kindle_desk_ring.svg"
local SLEEP_DIR = "/tmp/kindle_desk_sleep"
local SLEEP_IMG = SLEEP_DIR .. "/sleep.png"
local LOG = "/mnt/us/koreader/kindle_desk.log"
local DATA_TTL = 900        -- seconds between /data.json fetches
local SLEEP_TTL = 600       -- seconds between sleep-screen fetches
local SCREENSAVER_KEYS = { "screensaver_type", "screensaver_dir", "screensaver_show_message" }
local NIGHT_PRESETS = { { 22, 6 }, { 23, 6 }, { 0, 6 }, { 21, 7 } }
-- Rotation is ours, not KOReader's: the page is painted into an off-screen buffer and rotated before it
-- is shown, so gsensor auto-rotate, the file manager's layout and night mode cannot interfere.
local ROTATIONS = { portrait = 0, landscape_cw = 90, portrait_inv = 180, landscape_ccw = 270 } -- degrees, clockwise
local ROT_LABELS = { { "auto", "Tự động" }, { "portrait", "Dọc" }, { "portrait_inv", "Dọc ngược" },
                    { "landscape_cw", "Ngang (xoay trái)" }, { "landscape_ccw", "Ngang (xoay phải)" } }
-- KOReader reports how the *device* is turned; the page has to be turned the opposite way.
local DEVICE_TO_PAGE = { [0] = 0, [1] = 270, [2] = 180, [3] = 90 }
local ACCEL_TO_PAGE = { U = 0, R = 270, D = 180, L = 90 }
local MODE_LABELS = { en2vi = "EN › VI", vi2en = "VI › EN", mixed = "Trộn" }
local CARD_PAGES = { desk = true, vocab = true }
-- Physical swipe direction -> direction on the (unrotated) page, per rotation.
local SWIPE = {
    [0] = { east = "east", west = "west", north = "north", south = "south" },
    [90] = { south = "east", north = "west", west = "south", east = "north" },
    [180] = { east = "west", west = "east", north = "south", south = "north" },
    [270] = { north = "east", south = "west", east = "south", west = "north" },
}
local WEEKDAYS_UPPER = { "THỨ HAI", "THỨ BA", "THỨ TƯ", "THỨ NĂM", "THỨ SÁU", "THỨ BẢY", "CHỦ NHẬT" }

local SERIF_B, SANS, SANS_B, SANS_I = "NotoSerif-Bold.ttf", "NotoSans-Regular.ttf", "NotoSans-Bold.ttf", "NotoSans-Italic.ttf"
local BLACK, WHITE = Blitbuffer.COLOR_BLACK, Blitbuffer.COLOR_WHITE
local GRAY, SOFT = Blitbuffer.COLOR_GRAY_5, Blitbuffer.COLOR_LIGHT_GRAY

local Native = InputContainer:extend{
    name = "KindleDeskNative",
    covers_fullscreen = true,
    server = "",
    token = "",
    page = 1,
    draws = 0,
    menu_open = false,
    sleep_at = 0,
    data_at = 0,
}

local function now() return socket.gettime() end

local function log(fmt, ...)
    local ok, line = pcall(string.format, fmt, ...)
    local f = io.open(LOG, "a")
    if f then
        f:write(os.date("%H:%M:%S "), ok and line or tostring(fmt), "\n")
        f:close()
    end
end

------------------------------------------------------------------ helpers ----

function Native:cfg(key, default)
    local v = self.settings:readSetting(key)
    if v == nil then return default end
    return v
end

function Native:set(key, value)
    self.settings:saveSetting(key, value)
    self.settings:flush()
end

function Native:px(n) return math.floor(n * self.k + 0.5) end

function Native:face(name, n)
    local sf = Screen:scaleBySize(1000) / 1000
    return Font:getFace(name, math.max(8, math.floor(n * self.k / sf + 0.5)))
end

function Native:text(str, name, n, color, max_w)
    return TextWidget:new{ text = tostring(str), face = self:face(name, n), fgcolor = color or BLACK, max_width = max_w }
end

-- A TextWidget box is ~1.36x its font size tall (CSS gave us ~1x for display type), so big numbers get
-- centred in a shorter box; the glyphs overflow the empty leading equally above and below.
function Native:tight(widget, size, w)
    return self:center(widget, w or widget:getSize().w, math.floor(size * self.k * 1.02))
end

function Native:box(w, h)
    return CenterContainer:new{ dimen = Geom:new{ w = w, h = h }, HorizontalSpan:new{ width = 0 } }
end

function Native:center(widget, w, h)
    return CenterContainer:new{ dimen = Geom:new{ w = w, h = h }, widget }
end

function Native:gap(n) return VerticalSpan:new{ width = self:px(n) } end
function Native:hgap(n) return HorizontalSpan:new{ width = self:px(n) } end

function Native:icon(kind, size)
    -- original_in_nightmode=false: KOReader would otherwise invert "original" images a second time in
    -- night mode, which is what gave the icons a light box.
    return ImageWidget:new{ file = ICON_DIR .. kind .. ".svg", width = size, height = size, alpha = true,
                            file_do_cache = false, original_in_nightmode = false }
end

local function iconKind(c)
    if c == 0 then return "sun" end
    if c <= 2 then return "part" end
    if c == 3 then return "cloud" end
    if c == 45 or c == 48 then return "fog" end
    if c >= 95 then return "storm" end
    if (c >= 71 and c <= 77) or c == 85 or c == 86 then return "snow" end
    return "rain"
end

-- Marks a widget as tappable; its position is read back from dimen after it has been painted.
function Native:tappable(action, widget)
    local frame = FrameContainer:new{ bordersize = 0, padding = 0, margin = 0, widget }
    self.hits[#self.hits + 1] = { frame = frame, action = action }
    return frame
end

-- FrameContainer:getSize() is content-based (it ignores width/height), so the panel's content is wrapped in
-- a container of the exact inner size; that way the layout boxes and what gets painted always agree.
function Native:panel(w, h, content, opts)
    opts = opts or {}
    local border, padx, pady = self:px(4), self:px(opts.padx or 40), self:px(opts.pady or 22)
    return FrameContainer:new{
        radius = self:px(opts.radius or 30), bordersize = border, background = WHITE, margin = 0,
        padding = 0, padding_left = padx, padding_right = padx, padding_top = pady, padding_bottom = pady,
        CenterContainer:new{ dimen = Geom:new{ w = w - 2 * (border + padx), h = h - 2 * (border + pady) }, content },
    }
end

------------------------------------------------------------------- time ----

function Native:clock()
    local off = (self.data and self.data.tz) or 7
    return os.date("!*t", os.time() + math.floor(off * 3600))
end

function Native:weekdayIndex(t) return (t.wday + 5) % 7 + 1 end -- Monday = 1

function Native:timeString(t)
    local d = self.data or {}
    if d.hour12 then
        return string.format("%d:%02d", t.hour % 12 == 0 and 12 or t.hour % 12, t.min), (t.hour < 12 and "SA" or "CH")
    end
    return string.format("%02d:%02d", t.hour, t.min), nil
end

function Native:dateLine(t)
    local d = self.data or {}
    local line = string.format("%d tháng %d, %d", t.day, t.month, t.year)
    if d.lunar then line = line .. " · " .. d.lunar.text .. " · " .. d.lunar.year end
    return line
end

local function dayNumber(y, m, d) return math.floor(os.time{ year = y, month = m, day = d, hour = 12 } / 86400 + 0.5) end

function Native:daysUntil(iso)
    local y, m, d = tostring(iso):match("^(%d+)-(%d+)-(%d+)")
    if not y then return nil end
    local t = self:clock()
    return dayNumber(tonumber(y), tonumber(m), tonumber(d)) - dayNumber(t.year, t.month, t.day)
end

function Native:events()
    local out = {}
    for _, e in ipairs((self.data or {}).events or {}) do
        local n = self:daysUntil(e.date)
        if n then out[#out + 1] = { name = e.name, date = e.date, days = n } end
    end
    table.sort(out, function(a, b)
        if (a.days < 0) ~= (b.days < 0) then return a.days >= 0 end
        return math.abs(a.days) < math.abs(b.days)
    end)
    return out
end

---------------------------------------------------------------- flashcards ----

function Native:cardList() return (self.data or {}).cards or {} end

function Native:period() return (self.data or {}).card_seconds or 90 end
function Native:flipAfter() return (self.data or {}).flip_after_seconds or 20 end

function Native:cardSlot(t)
    return self.card.count + math.floor((t - self.card.t0) / self:period())
end

-- Deterministic per-day shuffle (same order all day, new order tomorrow).
function Native:permutation(n)
    local seed = dayNumber(self:clock().year, self:clock().month, self:clock().day)
    if self.perm and self.perm_seed == seed and #self.perm == n then return self.perm end
    local p = {}
    for i = 1, n do p[i] = i end
    local x = seed % 2147483647
    for i = n, 2, -1 do
        x = (x * 48271) % 2147483647
        local j = x % i + 1
        p[i], p[j] = p[j], p[i]
    end
    self.perm, self.perm_seed = p, seed
    return p
end

function Native:cardState(t)
    local cards = self:cardList()
    local n = #cards
    if n == 0 then return nil end
    local slot = self:cardSlot(t)
    local phase = (t - self.card.t0) % self:period()
    local flipped = self.card.flip_slot == slot or phase >= self:flipAfter()
    local idx = slot % n + 1
    if (self.data or {}).shuffle and n > 1 then idx = self:permutation(n)[idx] end
    local c = cards[idx]
    local mode = self.card.mode or (self.data or {}).mode or "mixed"
    local dir = mode
    if mode == "mixed" then dir = (slot % 2 == 0) and "en2vi" or "vi2en" end
    local s = { mode = mode, dir = dir, idx = idx, total = n, flipped = flipped, c = c }
    if dir == "en2vi" then
        s.front, s.sub, s.back, s.back_sub = c.en, c.ipa or "", c.vi, c.ex_en or ""
    else
        s.front, s.sub, s.back = c.vi, "", c.en
        local ex, ipa = c.ex_vi or "", c.ipa or ""
        s.back_sub = (ex ~= "" and ipa ~= "") and (ex .. "  ·  " .. ipa) or (ex .. ipa)
    end
    return s
end

function Native:cardAction(kind, arg)
    local t = os.time()
    local slot = self:cardSlot(t)
    if kind == "next" or kind == "prev" then
        self.card.count = slot + (kind == "next" and 1 or -1)
        self.card.t0 = t
        self.card.flip_slot = nil
    elseif kind == "flip" then
        self.card.flip_slot = slot
    elseif kind == "mode" then
        self.card.mode = arg
    end
end

-- Seconds until the visible flashcard flips or changes.
function Native:cardNext(t)
    local period, flip = self:period(), self:flipAfter()
    local phase = (t - self.card.t0) % period
    local best = period - phase
    if self.card.flip_slot ~= self:cardSlot(t) and phase < flip then best = math.min(best, flip - phase) end
    return math.max(1, math.floor(best) + 1)
end

------------------------------------------------------------------ pomodoro ----

function Native:pomoView(t)
    local d = self.data or {}
    local st = self.pomo.state
    if (st == "focus" or st == "break") and t >= self.pomo.stop then st = "done_" .. st end
    local base = st:gsub("^done_", "")
    local total = (base == "break") and (d.pomo_break or 5) or (d.pomo_focus or 25)
    local remain = (st == "focus" or st == "break") and math.max(0, self.pomo.stop - t) or 0
    local frac = (st == "focus" or st == "break") and (1 - remain / (total * 60)) or (st:find("^done") and 1 or 0)
    local labels = { idle = "Sẵn sàng", focus = "Đang tập trung", ["break"] = "Nghỉ ngơi",
                     done_focus = "Hết giờ tập trung!", done_break = "Hết giờ nghỉ!" }
    local hints = { idle = "Chạm để bắt đầu", focus = "Chạm để huỷ", ["break"] = "Chạm để dừng",
                    done_focus = "Chạm để nghỉ", done_break = "Chạm để tập trung tiếp" }
    return { state = st, remain_min = math.ceil(remain / 60), total = total, frac = frac, label = labels[st], hint = hints[st] }
end

function Native:pomoToggle()
    local d = self.data or {}
    local t = os.time()
    local st = self:pomoView(t).state
    if st == "idle" or st == "done_break" then
        self.pomo = { state = "focus", stop = t + (d.pomo_focus or 25) * 60 }
    elseif st == "done_focus" then
        self.pomo = { state = "break", stop = t + (d.pomo_break or 5) * 60 }
    else
        self.pomo = { state = "idle", stop = 0 }
    end
end

----------------------------------------------------------------- builders ----

function Native:header(t)
    local d = self.data or {}
    local hh = self:px(74)
    -- left: weekday (menu), middle: page dots, right: city + "read a book" button
    local wl, wm = math.floor(self.IW * 0.30), math.floor(self.IW * 0.20)
    local wr = self.IW - wl - wm
    local names = d.weekdays_upper or WEEKDAYS_UPPER
    local left = LeftContainer:new{ dimen = Geom:new{ w = wl, h = hh }, self:text(names[self:weekdayIndex(t)] or "", SANS_B, 34) }
    local dots = HorizontalGroup:new{}
    local pages = d.pages or { "desk" }
    for i = 1, #pages do
        local on = (i == self.page)
        if i > 1 then dots[#dots + 1] = self:hgap(14) end
        dots[#dots + 1] = FrameContainer:new{
            bordersize = self:px(3), radius = self:px(8), padding = 0, margin = 0, background = on and BLACK or WHITE,
            self:box(self:px(10), self:px(10)),
        }
    end
    local middle = self:center(dots, wm, hh)
    local exit_btn = FrameContainer:new{
        radius = self:px(28), bordersize = self:px(3), margin = 0, background = WHITE, padding = 0,
        padding_top = self:px(8), padding_bottom = self:px(8), padding_left = self:px(20), padding_right = self:px(20),
        self:text("Đọc sách", SANS_B, 28),
    }
    local exit_w = exit_btn:getSize().w
    local city = HorizontalGroup:new{ self:icon("pin", self:px(34)), self:hgap(10),
        self:text(d.city or "", SANS, 32, nil, math.max(self:px(80), wr - exit_w - self:px(34 + 10 + 24))) }
    local city_box = RightContainer:new{ dimen = Geom:new{ w = wr - exit_w - self:px(24), h = hh }, city }
    return HorizontalGroup:new{
        align = "center",
        self:tappable("menu", left), self:tappable("page", middle),
        self:tappable("refresh", city_box), self:hgap(24), self:tappable("@exit", exit_btn),
    }
end

function Native:clockBlock(w, t, land)
    local ts, ampm = self:timeString(t)
    local tsz = land and 220 or 250
    local big = self:tight(self:text(ts, SERIF_B, tsz), tsz)
    local row = big
    if ampm then
        row = HorizontalGroup:new{ align = "bottom", big, self:hgap(12), self:text(ampm, SANS_B, 60) }
    end
    local block = VerticalGroup:new{
        align = "center", row, self:gap(6), self:text(self:dateLine(t), SANS, land and 30 or 36, GRAY, w),
    }
    return self:center(block, w, block:getSize().h + self:px(6))
end

function Native:weatherRows(W, iw, land, s, deg)
    local function n(v) return v * s end
    local rows = {}

    -- current conditions
    local ic = self:icon(iconKind(W.code), self:px(n(land and 190 or 150)))
    local tsize = n(land and 170 or 150)
    local temp = self:tight(self:text(tostring(W.temp) .. deg, SERIF_B, tsize), tsize)
    local side = VerticalGroup:new{ align = "right",
        self:text(W.label or "", SANS_B, n(46)),
        self:text(string.format("Cao %d° · Thấp %d°", W.hi, W.lo), SANS, n(land and 32 or 34), GRAY) }
    local used = ic:getSize().w + self:px(26) + temp:getSize().w + side:getSize().w
    rows[#rows + 1] = HorizontalGroup:new{ align = "center", ic, self:hgap(26), temp,
        HorizontalSpan:new{ width = math.max(self:px(10), iw - used) }, side }

    -- stats
    local stats = {
        { "Cảm giác ", tostring(W.feels) .. "°" }, { "Ẩm ", tostring(W.hum) .. "%" },
        { "Gió ", tostring(W.wind) .. " " .. (W.wind_unit or "km/h") }, { "UV ", tostring(W.uv) },
    }
    if W.aqi then stats[#stats + 1] = { "AQI ", tostring(W.aqi) .. " " .. (W.aqi_label or "") } end
    stats[#stats + 1] = { "", (W.sunrise or "") .. "–" .. (W.sunset or "") }
    local fs = n(land and 28 or 26)
    local items, sum = {}, 0
    for i, st in ipairs(stats) do
        local g = HorizontalGroup:new{}
        if st[1] ~= "" then g[#g + 1] = self:text(st[1], SANS, fs, GRAY) end
        g[#g + 1] = self:text(st[2], SANS_B, fs)
        items[i] = g
        sum = sum + g:getSize().w
    end
    local per = (sum + self:px(12) * (#items - 1) <= iw) and #items or 3
    local stat_rows = VerticalGroup:new{ align = "left" }
    for i = 1, #items, per do
        local row = HorizontalGroup:new{}
        local last = math.min(i + per - 1, #items)
        local rowsum = 0
        for j = i, last do rowsum = rowsum + items[j]:getSize().w end
        local space = last > i and math.floor((iw - rowsum) / (last - i)) or 0
        for j = i, last do
            if j > i then row[#row + 1] = HorizontalSpan:new{ width = math.max(self:px(8), space) } end
            row[#row + 1] = items[j]
        end
        stat_rows[#stat_rows + 1] = row
        stat_rows[#stat_rows + 1] = self:gap(6 * s)
    end
    rows[#rows + 1] = VerticalGroup:new{ align = "left",
        LineWidget:new{ background = SOFT, dimen = Geom:new{ w = iw, h = self:px(2) } }, self:gap(10 * s),
        stat_rows, LineWidget:new{ background = SOFT, dimen = Geom:new{ w = iw, h = self:px(2) } } }

    -- strips
    local function strip(list, labelKey, hiKey, loKey)
        local g = HorizontalGroup:new{ align = "top" }
        local cnt = math.max(1, #list)
        local cw = math.floor(iw / cnt)
        for _, it in ipairs(list) do
            local col = VerticalGroup:new{ align = "center",
                self:text(it[labelKey], SANS, n(land and 30 or 26), GRAY),
                self:icon(iconKind(it.code), self:px(n(land and 84 or 54))),
                self:text(tostring(it[hiKey]) .. "°", SANS_B, n(land and 42 or 32)),
                self:text(it[loKey], SANS, n(land and 28 or 24), GRAY) }
            g[#g + 1] = self:center(col, cw, col:getSize().h)
        end
        return g
    end
    local hours, days = {}, {}
    for _, hr in ipairs(W.hours or {}) do hours[#hours + 1] = { h = hr.h, code = hr.code, t = hr.t, pop = tostring(hr.pop) .. "%" } end
    for _, dy in ipairs(W.days or {}) do days[#days + 1] = { d = dy.d, code = dy.code, hi = dy.hi, lo = tostring(dy.lo) .. "°" } end
    rows[#rows + 1] = strip(hours, "h", "t", "pop")
    rows[#rows + 1] = strip(days, "d", "hi", "lo")
    return rows
end

-- Text metrics differ between devices, so shrink the content step by step until it fits the panel.
function Native:weatherPanel(w, h, land)
    local W = (self.data or {}).weather
    if not W then
        return self:panel(w, h, self:text("Chưa có dữ liệu thời tiết", SANS, 40))
    end
    local border, padx, pady = self:px(4), self:px(land and 36 or 40), self:px(24)
    local iw = w - 2 * (border + padx)
    local avail = h - 2 * (border + pady)
    local deg = ((self.data or {}).unit == "F") and "°F" or "°"
    local rows, total
    for _, s in ipairs({ 1, 0.93, 0.86, 0.79, 0.72, 0.65, 0.58, 0.5 }) do
        rows = self:weatherRows(W, iw, land, s, deg)
        total = 0
        for _, r in ipairs(rows) do total = total + r:getSize().h end
        if total + self:px(6) * (#rows - 1) <= avail then break end
    end
    local space = math.max(self:px(6), math.min(self:px(60), math.floor((avail - total) / (#rows - 1))))
    local col = VerticalGroup:new{ align = "left" }
    for i, r in ipairs(rows) do
        if i > 1 then col[#col + 1] = VerticalSpan:new{ width = space } end
        col[#col + 1] = r
    end
    return self:panel(w, h, col, { padx = land and 36 or 40, pady = 24 })
end

function Native:flashPanel(w, h, big, land)
    local st = self:cardState(os.time())
    local iw = w - self:px(88)
    if not st then return self:panel(w, h, self:text("Chưa có thẻ nào", SANS, 40)) end
    local title = (big and "TỪ VỰNG · " or "FLASHCARD · ") .. MODE_LABELS[st.dir]
    local counter = string.format("%02d / %d", st.idx, st.total)
    local avail = h - 2 * (self:px(4) + self:px(22))
    local col
    for _, s in ipairs({ 1, 0.9, 0.8, 0.7, 0.6, 0.5 }) do
        local function n(v) return v * s end
        local l, r = self:text(title, SANS_B, big and 30 or 26, GRAY), self:text(counter, SANS_B, big and 30 or 26, GRAY)
        local top = HorizontalGroup:new{ l, HorizontalSpan:new{ width = math.max(self:px(10), iw - l:getSize().w - r:getSize().w) }, r }
        local fsz = n(big and (land and 130 or 170) or (land and 120 or 92))
        local bsz = n(big and (land and 84 or 112) or (land and 84 or 70))
        local front = VerticalGroup:new{ align = "center",
            TextBoxWidget:new{ text = st.front, face = self:face(SERIF_B, fsz), width = iw, alignment = "center" },
            self:text(st.sub ~= "" and st.sub or " ", SANS, n(big and 48 or (land and 42 or 34)), GRAY, iw) }
        local rule = LineWidget:new{ style = "dashed", background = BLACK, dimen = Geom:new{ w = iw, h = self:px(3) } }
        local back
        if st.flipped then
            back = VerticalGroup:new{ align = "center",
                TextBoxWidget:new{ text = st.back, face = self:face(SERIF_B, bsz), width = iw, alignment = "center" },
                self:gap(6),
                TextBoxWidget:new{ text = st.back_sub, face = self:face(SANS_I, n(big and 42 or (land and 34 or 30))), width = iw, alignment = "center", fgcolor = GRAY } }
        else
            back = self:text("•  •  •", SANS, big and 90 or 60, SOFT)
        end
        local used = top:getSize().h + front:getSize().h + rule:getSize().h + self:px(10)
        local area = avail - used
        local need = st.flipped and back:getSize().h or self:px(40)
        col = VerticalGroup:new{ align = "center", top, front, self:gap(10), rule, self:center(back, iw, math.max(need, area)) }
        if area >= need then break end
    end
    return self:tappable("flip", self:panel(w, h, col))
end

function Native:dock(land)
    local s = self:cardState(os.time())
    local mode = s and s.mode or "mixed"
    local g = HorizontalGroup:new{}
    for i, m in ipairs({ "en2vi", "vi2en", "mixed" }) do
        local on = (m == mode)
        if i > 1 then g[#g + 1] = self:hgap(8) end
        g[#g + 1] = self:tappable("mode:" .. m, FrameContainer:new{
            radius = self:px(58), bordersize = 0, margin = 0, background = on and WHITE or BLACK,
            padding = 0, padding_top = self:px(land and 14 or 17), padding_bottom = self:px(land and 14 or 17),
            padding_left = self:px(land and 22 or 36), padding_right = self:px(land and 22 or 36),
            self:text(MODE_LABELS[m], SANS_B, land and 28 or 34, on and BLACK or WHITE),
        })
    end
    return FrameContainer:new{ radius = self:px(70), bordersize = 0, margin = 0, padding = self:px(12), background = BLACK, g }
end

function Native:pageDesk(t)
    local land = self.land
    local hdr = self:header(t)
    if not land then
        local clock = self:clockBlock(self.IW, t, false)
        local card_h = self:px(392)
        local dock = self:dock(false)
        local fixed = hdr:getSize().h + clock:getSize().h + card_h + dock:getSize().h + self:px(22) * 4
        local weather = self:weatherPanel(self.IW, self.H - 2 * self.pad - fixed, false)
        return VerticalGroup:new{ align = "center", hdr, self:gap(22), clock, self:gap(22), weather, self:gap(22),
            self:flashPanel(self.IW, card_h, false, false), self:gap(22), dock }
    end
    local gapx = self:px(26)
    local lw = math.floor((self.IW - gapx) * 0.45)
    local rw = self.IW - gapx - lw
    local clock = self:clockBlock(lw, t, true)
    local dock = self:dock(true)
    local body_h = self.H - 2 * self.pad - hdr:getSize().h - self:px(18)
    local card_h = body_h - clock:getSize().h - dock:getSize().h - self:px(18) * 2
    local left = VerticalGroup:new{ align = "center", clock, self:gap(18), self:flashPanel(lw, card_h, false, true), self:gap(18),
        self:center(dock, lw, dock:getSize().h) }
    return VerticalGroup:new{ align = "left", hdr, self:gap(18),
        HorizontalGroup:new{ align = "top", left, HorizontalSpan:new{ width = gapx }, self:weatherPanel(rw, body_h, true) } }
end

function Native:pageVocab(t)
    local hdr = self:header(t)
    local dock = self:dock(self.land)
    local card_h = self.H - 2 * self.pad - hdr:getSize().h - dock:getSize().h - self:px(22) * 2
    return VerticalGroup:new{ align = "center", hdr, self:gap(22), self:flashPanel(self.IW, card_h, true, self.land), self:gap(22), dock }
end

function Native:pageQuote(t)
    local d = self.data or {}
    local hdr = self:header(t)
    local quotes = d.quotes or {}
    local q = quotes[1] and quotes[(dayNumber(t.year, t.month, t.day) + self.quote_offset) % #quotes + 1] or { text = "", by = "" }
    local qw = math.min(self.IW, self:px(self.land and 1360 or 1040))
    local body = VerticalGroup:new{ align = "center",
        self:tight(self:text("“", SERIF_B, 300), 190), self:gap(10),
        TextBoxWidget:new{ text = q.text, face = self:face(SERIF_B, self.land and 68 or 76), width = qw, alignment = "center" },
        self:gap(20), self:text("— " .. (q.by or ""), SANS, 44, GRAY, qw), self:gap(30), self:text(self:dateLine(t), SANS, 34, GRAY, self.IW) }
    local area = self.H - 2 * self.pad - hdr:getSize().h
    return VerticalGroup:new{ align = "center", hdr, self:tappable("quote", self:center(body, self.IW, area)) }
end

local function moonSvg(phase, size)
    local r, cx, cy = 45, 50, 50
    local k = math.cos(phase * 2 * math.pi)
    local rx = math.abs(k) * r
    local waxing = phase < 0.5
    local limb = waxing and 1 or 0
    local term = waxing and (k > 0 and 0 or 1) or (k > 0 and 1 or 0)
    local lit = string.format("M %d %d A %d %d 0 0 %d %d %d A %.2f %d 0 0 %d %d %d Z", cx, cy - r, r, r, limb, cx, cy + r, rx, r, term, cx, cy - r)
    return string.format('<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 100 100"><circle cx="50" cy="50" r="45" fill="#fff" stroke="#000" stroke-width="4"/><path d="%s" fill="#000"/></svg>', size, size, lit)
end

local function writeFile(path, s)
    local f = io.open(path, "w")
    if f then f:write(s) f:close() end
end

function Native:pageCalendar(t)
    local d = self.data or {}
    local cal = d.cal
    local hdr = self:header(t)
    if not cal then return VerticalGroup:new{ hdr } end
    local land = self.land
    local gapp = self:px(22)
    local body_h = self.H - 2 * self.pad - hdr:getSize().h - gapp
    local gw, sw, sh_events
    if land then
        gw = math.floor((self.IW - gapp) * 0.625)
        sw = self.IW - gapp - gw
    else
        gw, sw = self.IW, self.IW
    end

    -- side column: moon + events
    local moon = d.moon or { phase = 0, name = "", illum = 0, age = 0 }
    writeFile(TMP_MOON, moonSvg(moon.phase, 130))
    local msz = self:px(130)
    local moon_img = ImageWidget:new{ file = TMP_MOON, width = msz, height = msz, alpha = true, file_do_cache = false,
        original_in_nightmode = false }
    local moon_panel = self:panel(sw, self:px(180), HorizontalGroup:new{ align = "center", moon_img, self:hgap(26),
        VerticalGroup:new{ align = "left", self:text(moon.name, SANS_B, 40, nil, sw - self:px(300)),
            self:text(string.format("Sáng %d%% · ngày %.1f", moon.illum, moon.age), SANS, 30, GRAY, sw - self:px(300)) } })
    local evs = VerticalGroup:new{ align = "left", self:text("SỰ KIỆN SẮP TỚI", SANS_B, 28, GRAY), self:gap(10) }
    local list = self:events()
    for i = 1, math.min(3, #list) do
        local e = list[i]
        local unit = e.days < 0 and " ngày trước" or (e.days == 0 and " HÔM NAY" or " ngày nữa")
        evs[#evs + 1] = LineWidget:new{ background = SOFT, dimen = Geom:new{ w = sw - self:px(88), h = self:px(2) } }
        evs[#evs + 1] = HorizontalGroup:new{ align = "center",
            self:tight(self:text(tostring(math.abs(e.days)), SERIF_B, 76), 76), self:text(unit, SANS, 26),
            self:hgap(20),
            VerticalGroup:new{ align = "left", self:text(e.name, SANS_B, 34, nil, sw - self:px(420)), self:text((tostring(e.date):gsub("^(%d+)-(%d+)-(%d+).*$", "%3/%2/%1")), SANS, 26, GRAY) } }
        evs[#evs + 1] = self:gap(6)
    end
    if #list == 0 then evs[#evs + 1] = self:text("Thêm sự kiện trong trang cài đặt", SANS, 30, GRAY, sw - self:px(88)) end
    local ev_h = land and (body_h - self:px(180) - gapp) or self:px(420)
    local events_panel = self:panel(sw, ev_h, evs)

    -- month grid
    local grid_h = land and body_h or (body_h - self:px(180) - ev_h - gapp * 2)
    local padx = self:px(30)
    local iw = gw - 2 * (self:px(4) + padx)
    local cw = math.floor(iw / 7)
    local title = self:text(string.format("Tháng %d · %d", cal.month, cal.year), SERIF_B, 64)
    local names = { "T2", "T3", "T4", "T5", "T6", "T7", "CN" }
    local head = HorizontalGroup:new{}
    for _, n in ipairs(names) do head[#head + 1] = self:center(self:text(n, SANS_B, 28, GRAY), cw, self:px(40)) end
    local cells = {}
    for _ = 1, cal.start do cells[#cells + 1] = false end
    for _, c in ipairs(cal.days) do cells[#cells + 1] = c end
    local nrows = math.ceil(#cells / 7)
    local rows_h = grid_h - 2 * (self:px(4) + self:px(26)) - title:getSize().h - self:px(14) - head:getSize().h
    local rh = math.floor(rows_h / nrows)
    local grid = VerticalGroup:new{ align = "left", self:center(title, iw, title:getSize().h), self:gap(14), head }
    for r = 0, nrows - 1 do
        local row = HorizontalGroup:new{}
        for c = 1, 7 do
            local cell = cells[r * 7 + c]
            if not cell then
                row[#row + 1] = self:box(cw, rh)
            else
                local today = (cell.d == cal.today and cal.year == t.year and cal.month == t.month)
                local fg = today and WHITE or BLACK
                local col = VerticalGroup:new{ align = "center", self:text(tostring(cell.d), SANS_B, math.min(46, math.floor(rh / self.k * 0.48)), fg) }
                if d.show_lunar then
                    col[#col + 1] = self:text((cell.ev and "• " or "") .. cell.lunar, SANS, 22, today and WHITE or GRAY)
                end
                local inner = self:center(col, cw - self:px(6), rh - self:px(6))
                row[#row + 1] = FrameContainer:new{ radius = self:px(20), bordersize = 0, margin = self:px(3), padding = 0,
                    background = today and BLACK or nil, inner }
            end
        end
        grid[#grid + 1] = row
    end
    local grid_panel = self:panel(gw, grid_h, grid, { padx = 30, pady = 26 })

    if land then
        return VerticalGroup:new{ align = "left", hdr, self:gap(gapp), HorizontalGroup:new{ align = "top", grid_panel,
            HorizontalSpan:new{ width = gapp }, VerticalGroup:new{ align = "left", moon_panel, self:gap(18), events_panel } } }
    end
    return VerticalGroup:new{ align = "center", hdr, self:gap(22), grid_panel, self:gap(22), moon_panel, self:gap(22), events_panel }
end

function Native:pageFocus(t)
    local hdr = self:header(t)
    local P = self:pomoView(os.time())
    local size = self:px(self.land and 640 or 700)
    -- Ring as an SVG path arc (robust: no dasharray needed).
    local frac = math.min(1, P.frac)
    local arc = ""
    if frac >= 0.999 then
        arc = '<circle cx="100" cy="100" r="88" fill="none" stroke="#000" stroke-width="12"/>'
    elseif frac > 0.002 then
        local a = frac * 2 * math.pi
        arc = string.format('<path d="M 100 12 A 88 88 0 %d 1 %.2f %.2f" fill="none" stroke="#000" stroke-width="12" stroke-linecap="round"/>',
            frac > 0.5 and 1 or 0, 100 + 88 * math.sin(a), 100 - 88 * math.cos(a))
    end
    local check = P.state:find("^done") and '<path d="M62 102 L90 130 L142 74" fill="none" stroke="#000" stroke-width="14" stroke-linecap="round" stroke-linejoin="round"/>' or ""
    writeFile(TMP_RING, '<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200" viewBox="0 0 200 200"><circle cx="100" cy="100" r="88" fill="none" stroke="#d9d9d9" stroke-width="12"/>' .. arc .. check .. '</svg>')
    local ring = ImageWidget:new{ file = TMP_RING, width = size, height = size, alpha = true, file_do_cache = false,
        original_in_nightmode = false }
    local mid
    if P.state:find("^done") then
        mid = self:box(1, 1)
    else
        mid = VerticalGroup:new{ align = "center",
            self:tight(self:text(tostring(P.state == "idle" and P.total or P.remain_min), SERIF_B, 260), 260),
            self:text(P.state == "idle" and "PHÚT" or "PHÚT CÒN LẠI", SANS, 46, GRAY) }
    end
    local dial = OverlapGroup:new{ dimen = Geom:new{ w = size, h = size }, ring, self:center(mid, size, size) }
    local body = VerticalGroup:new{ align = "center", dial, self:gap(30), self:text(P.label, SANS_B, 64, nil, self.IW),
        self:gap(14), self:text(P.hint, SANS, 38, GRAY, self.IW) }
    local area = self.H - 2 * self.pad - hdr:getSize().h
    return VerticalGroup:new{ align = "center", hdr, self:tappable("pomo", self:center(body, self.IW, area)) }
end

function Native:pageMenu()
    local land = self.land
    local rowh, fs = self:px(land and 66 or 88), land and 30 or 36
    local function button(label, action, on, w)
        local fg = on and WHITE or BLACK
        local b = self:px(4)
        return self:tappable(action, FrameContainer:new{
            radius = self:px(28), bordersize = b, margin = 0, padding = 0, background = on and BLACK or WHITE,
            self:center(self:text(label, SANS_B, fs, fg, w - self:px(24)), w - 2 * b, rowh - 2 * b),
        })
    end
    local function row(items)
        local n = #items
        local gap = self:px(14)
        local w = math.floor((self.IW - gap * (n - 1)) / n)
        local g = HorizontalGroup:new{}
        for i, it in ipairs(items) do
            if i > 1 then g[#g + 1] = HorizontalSpan:new{ width = gap } end
            g[#g + 1] = button(it[1], it[2], it[3], w)
        end
        return g
    end
    local function label(s) return VerticalGroup:new{ align = "left", self:gap(land and 6 or 10), self:text(s, SANS_B, 26, GRAY), self:gap(6) } end
    local rot = self:cfg("rotation", "auto")
    local rots = {}
    for _, r in ipairs(ROT_LABELS) do rots[#rots + 1] = { r[2], "@rot:" .. r[1], rot == r[1] } end
    local refs = {}
    for _, m in ipairs({ 1, 2, 5, 10 }) do refs[#refs + 1] = { m .. " phút", "@ref:" .. m, self:cfg("refresh_min", 1) == m } end
    local night = self:cfg("night", false)
    local from, to = self:cfg("night_from", 23), self:cfg("night_to", 6)
    local col = VerticalGroup:new{ align = "left",
        self:center(self:text("Cài đặt", SERIF_B, land and 64 or 84), self.IW, self:px(land and 84 or 110)),
        label("TRANG"), row({ { "< Trang trước", "@page:-1" }, { "Trang kế >", "@page:1" } }),
        label("XOAY MÀN HÌNH"), row({ rots[1], rots[2], rots[3] }), self:gap(10), row({ rots[4], rots[5] }),
        label("LÀM MỚI DỮ LIỆU MỖI"), row(refs),
        label("XOÁ BÓNG ẢNH (NHẤP NHÁY MỖI)"),
        row({ { "1 lần", "@full:1", self:cfg("full_every", 3) == 1 }, { "3 lần", "@full:3", self:cfg("full_every", 3) == 3 },
              { "5 lần", "@full:5", self:cfg("full_every", 3) == 5 }, { "Xoá ngay", "@clean" } }),
        label("NGỦ"), row({ { "Màn hình ngủ", "@sleepscreen", self:cfg("sleep_screen", true) }, { "Ngủ ngay", "@sleep" } }), self:gap(10),
        row({ { string.format("Ngủ đêm %dh–%dh", from, to), "@night", night }, { "Đổi giờ đêm", "@nightpreset" } }),
        label("GIAO DIỆN"), row({ { "Tự vẽ trên Kindle", "@ui:native", true }, { "Ảnh từ server", "@ui:image" } }),
        self:gap(14), row({ { "Làm mới ngay", "@refresh" }, { "Đóng menu", "@close", true }, { "Thoát Desk", "@exit" } }) }
    return col
end

function Native:buildPage()
    local t = self:clock()
    self.hits = {}
    if self.menu_open then return self:pageMenu() end
    local pages = (self.data or {}).pages or { "desk" }
    local name = pages[(self.page - 1) % #pages + 1]
    self.page_name = name
    if name == "calendar" then return self:pageCalendar(t) end
    if name == "focus" then return self:pageFocus(t) end
    if name == "vocab" then return self:pageVocab(t) end
    if name == "quote" then return self:pageQuote(t) end
    return self:pageDesk(t)
end

------------------------------------------------------------------ painting ----

function Native:measure()
    self.PW, self.PH = Screen:getWidth(), Screen:getHeight()
    self.dimen.w, self.dimen.h = self.PW, self.PH
    local setting = self:cfg("rotation", "auto")
    self.rot = (setting == "auto") and (self.auto_rot or 0) or (ROTATIONS[setting] or 0)
    local swap = (self.rot == 90 or self.rot == 270)
    -- W/H are the logical page size; the rotated image is PW x PH on the glass.
    self.W, self.H = swap and self.PH or self.PW, swap and self.PW or self.PH
    self.land = self.W > self.H
    self.k = math.min(self.W, self.H) / 1264
    self.pad = math.floor(52 * self.k * (self.land and 0.78 or 1))
    self.IW = self.W - 2 * self.pad
end

-- Whether Blitbuffer:rotatedCopy(90) turns the picture clockwise; checked on a 3x2 test image instead of
-- trusting documentation, because the touch mapping depends on it.
local CW90
local function rotatesClockwise()
    if CW90 ~= nil then return CW90 end
    local ok, res = pcall(function()
        local t = Blitbuffer.new(3, 2, Screen.bb:getType())
        t:fill(WHITE)
        t:setPixel(0, 0, BLACK)
        local r = t:rotatedCopy(90)
        local function dark(x, y) return r:getPixel(x, y):getColor8().a < 128 end
        local result
        if dark(r:getWidth() - 1, 0) then result = true elseif dark(0, r:getHeight() - 1) then result = false end
        t:free()
        r:free()
        return result
    end)
    if not ok or res == nil then
        log("rotation check failed (%s); assuming rotatedCopy(90) is clockwise", tostring(res))
        res = true
    end
    CW90 = res
    log("rotatedCopy(90) is %s", CW90 and "clockwise" or "counter-clockwise")
    return CW90
end

-- Current orientation straight from the Kindle's accelerometer (U/R/D/L), if it can be read.
local function readAccelerometer()
    local ok, out = pcall(function()
        local f = io.popen("lipc-get-prop com.lab126.winmgr accelerometer 2>/dev/null")
        if not f then return nil end
        local v = f:read("*l")
        f:close()
        return v
    end)
    return ok and out and ACCEL_TO_PAGE[tostring(out):match("%a")] or nil
end

-- KOReader sends SetRotationMode when the gsensor sees the device turn. Always consume it, otherwise
-- the file manager under us would rotate the real screen and break our layout; follow it in auto mode.
function Native:onSetRotationMode(mode)
    if self:cfg("rotation", "auto") == "auto" then
        local want = DEVICE_TO_PAGE[mode]
        if want ~= nil and want ~= self.auto_rot then
            log("gsensor: device rotation %s -> page rotation %d", tostring(mode), want)
            self.auto_rot = want
            self:measure()
            self:rebuild(true)
            self:fetchSleep(true)
            self:scheduleTick()
        end
    end
    return true
end

-- Degrees to hand to rotatedCopy so that the picture ends up rotated clockwise by self.rot.
function Native:copyDegrees()
    local cw = rotatesClockwise()
    if self.rot == 90 then return cw and 90 or 270 end
    if self.rot == 270 then return cw and 270 or 90 end
    return self.rot
end

-- Touch coordinates on the glass -> coordinates on the unrotated page, and back (used by tests).
function Native:toLogical(px, py)
    if self.rot == 90 then return py, self.H - 1 - px end
    if self.rot == 270 then return self.W - 1 - py, px end
    if self.rot == 180 then return self.W - 1 - px, self.H - 1 - py end
    return px, py
end

function Native:toPhysical(lx, ly)
    if self.rot == 90 then return self.H - 1 - ly, lx end
    if self.rot == 270 then return ly, self.W - 1 - lx end
    if self.rot == 180 then return self.W - 1 - lx, self.H - 1 - ly end
    return lx, ly
end

function Native:rebuild(full)
    local t0 = now()
    self:measure()
    local ok, page = xpcall(function() return self:buildPage() end, debug.traceback)
    if not ok then
        log("build error on page %s: %s", tostring(self.page_name), tostring(page))
        self.errors = (self.errors or 0) + 1
        if self.errors >= 2 then
            log("native UI keeps failing, switching to server images")
            self:switchUI("image")
        end
        return false
    end
    self.errors = 0
    local frame = FrameContainer:new{
        background = WHITE, bordersize = 0, margin = 0, padding = 0, padding_left = self.pad, padding_right = self.pad,
        padding_top = self.pad, padding_bottom = self.pad, width = self.W, height = self.H, page,
    }
    -- Paint the page into an off-screen buffer (also records every tappable frame's position), rotate it,
    -- and show the result as one image. Night mode inverts the final blit, so icons match the text.
    local painted, shown = pcall(function()
        local bb = Blitbuffer.new(self.W, self.H, Screen.bb:getType())
        bb:fill(WHITE)
        frame:paintTo(bb, 0, 0)
        local out = bb
        if self.rot ~= 0 then
            out = bb:rotatedCopy(self:copyDegrees())
            bb:free()
        end
        return ImageWidget:new{ image = out, image_disposable = true, original_in_nightmode = false }
    end)
    if not painted then
        log("paint error: %s", tostring(shown))
        return false
    end
    if self[1] and self[1].free then pcall(function() self[1]:free() end) end
    self[1] = shown
    self.draws = self.draws + 1
    -- "partial" is cleaner than the fast "ui" waveform; a flashing "full" clears ghosts on every
    -- navigation (page, menu, rotation) and every Nth minute tick.
    local every = self:cfg("full_every", 3)
    local mode = (full or every <= 1 or self.draws % every == 0) and "full" or "partial"
    UIManager:setDirty(self, function() return mode, self.dimen end)
    log("page %s built in %.2fs", tostring(self.page_name), now() - t0)
    return true
end

------------------------------------------------------------------- data ----

function Native:cachePath() return DataStorage:getSettingsDir() .. "/kindle_desk_data.json" end

function Native:loadCache()
    local f = io.open(self:cachePath(), "r")
    if not f then return end
    local s = f:read("*a")
    f:close()
    local ok, d = pcall(json.decode, s)
    if ok and type(d) == "table" then self.data = d end
end

function Native:get(path, dest)
    local f = io.open(dest, "wb")
    if not f then return false end
    local client = self.server:match("^https") and require("ssl.https") or require("socket.http")
    socketutil:set_timeout(10, 30)
    local ok, code = pcall(function()
        return socket.skip(1, client.request{
            url = self.server .. path .. (path:find("?", 1, true) and "&" or "?") .. "k=" .. self.token,
            method = "GET", sink = ltn12.sink.file(f),
        })
    end)
    socketutil:reset_timeout()
    if not ok then log("request error: %s", tostring(code)) return false end
    if code ~= 200 then log("http %s for %s", tostring(code), path) end
    return code == 200
end

function Native:fetchData()
    local t0 = now()
    local part = self:cachePath() .. ".part"
    if not self:get("/data.json", part) then return false end
    local f = io.open(part, "r")
    local s = f and f:read("*a")
    if f then f:close() end
    local ok, d = pcall(json.decode, s or "")
    if not (ok and type(d) == "table") then log("bad data.json") return false end
    self.data = d
    self.data_at = os.time()
    os.rename(part, self:cachePath())
    log("data fetched in %.2fs", now() - t0)
    return true
end

local function battery()
    local ok, cap = pcall(function() return Device:getPowerDevice():getCapacity() end)
    return ok and cap or nil
end

function Native:fetchSleep(force)
    if not self:cfg("sleep_screen", true) then return end
    if not force and os.time() - self.sleep_at < SLEEP_TTL then return end
    local bat = battery()
    local path = string.format("/sleep.png?w=%d&h=%d&rot=%d%s", self.PW or self.W, self.PH or self.H, self.rot or 0,
        bat and ("&bat=" .. bat) or "")
    if self:get(path, SLEEP_IMG .. ".part") then
        os.rename(SLEEP_IMG .. ".part", SLEEP_IMG)
        self.sleep_at = os.time()
    end
end

-- Pull fresh data when it is stale (or the date changed), without ever blocking taps.
function Native:syncIfNeeded(force)
    local t = self:clock()
    local d = self.data
    local stale = force or not d or os.time() - self.data_at > DATA_TTL
        or (d.date and d.date ~= string.format("%04d-%02d-%02d", t.year, t.month, t.day))
    if not stale then return end
    if not self:fetchData() and force then
        NetworkMgr:runWhenOnline(function() self:fetchData() self:rebuild() end)
    end
end

---------------------------------------------------------------- lifecycle ----

function Native:init()
    self.dimen = Geom:new{ x = 0, y = 0, w = Screen:getWidth(), h = Screen:getHeight() }
    self.hits = {}
    self.card = { count = 0, t0 = os.time() }
    self.pomo = { state = "idle", stop = 0 }
    self.quote_offset = 0
    if Device:hasKeys() then
        self.key_events.NextCard = { { Device.input.group.PgFwd } }
        self.key_events.PrevCard = { { Device.input.group.PgBack } }
    end
    if Device:isTouchDevice() then
        self.ges_events.Tap = { GestureRange:new{ ges = "tap", range = self.dimen } }
        self.ges_events.Swipe = { GestureRange:new{ ges = "swipe", range = self.dimen } }
    end
    self[1] = FrameContainer:new{ background = WHITE, bordersize = 0, padding = 0, margin = 0,
        width = self.dimen.w, height = self.dimen.h, TextWidget:new{ text = "Kindle Desk ...", face = Font:getFace("cfont", 28) } }
    self._tick = function() self:onTick() end
end

function Native:overrideScreensaver()
    if not self:cfg("sleep_screen", true) or self.settings:has("saved_screensaver") then return end
    local saved = {}
    for _, k in ipairs(SCREENSAVER_KEYS) do saved[k] = G_reader_settings:readSetting(k) end
    self.settings:saveSetting("saved_screensaver", saved)
    self.settings:flush()
    G_reader_settings:saveSetting("screensaver_type", "random_image")
    G_reader_settings:saveSetting("screensaver_dir", SLEEP_DIR)
    G_reader_settings:saveSetting("screensaver_show_message", false)
end

function Native.restoreScreensaver(s)
    local saved = s:readSetting("saved_screensaver")
    if not saved then return end
    for _, k in ipairs(SCREENSAVER_KEYS) do
        if saved[k] == nil then G_reader_settings:delSetting(k) else G_reader_settings:saveSetting(k, saved[k]) end
    end
    s:delSetting("saved_screensaver")
    s:flush()
end

function Native:applyRotation()
    -- Intentionally empty: rotation happens in rebuild() on our own image.
end

function Native:onShow()
    if (lfs.attributes(LOG, "size") or 0) > 200000 then os.remove(LOG) end
    lfs.mkdir(SLEEP_DIR)
    self:loadCache()
    self:overrideScreensaver()
    if self:cfg("rotation", "auto") == "auto" then self.auto_rot = readAccelerometer() or 0 end
    UIManager:preventStandby()
    log("--- native desk opened %dx%d, rotation %s, night mode %s, cached data: %s", Screen:getWidth(), Screen:getHeight(),
        tostring(self:cfg("rotation", "auto")), tostring(Screen.night_mode), self.data and "yes" or "no")
    self:rebuild(true)
    UIManager:nextTick(function()
        self:syncIfNeeded(true)
        self:fetchSleep(true)
        self:rebuild()
        self:scheduleTick()
    end)
end

function Native:onCloseWidget()
    UIManager:unschedule(self._tick)
    UIManager:allowStandby()
    Native.restoreScreensaver(self.settings)
    UIManager:setDirty(nil, "full")
end

function Native:onClose()
    UIManager:close(self)
    return true
end

function Native:switchUI(mode)
    self:set("ui", mode)
    UIManager:close(self)
    if self.switch_cb then UIManager:nextTick(function() self.switch_cb() end) end
end

function Native:onResume()
    if self:cfg("rotation", "auto") == "auto" then self.auto_rot = readAccelerometer() or self.auto_rot end
    self:syncIfNeeded()
    self:rebuild(true)
    self:scheduleTick()
end

function Native:inNight()
    if not self:cfg("night", false) then return false end
    local from, to = self:cfg("night_from", 23), self:cfg("night_to", 6)
    local h = self:clock().hour
    if from > to then return h >= from or h < to end
    return h >= from and h < to
end

-- Next wake-up: top of the next refresh interval, sooner for a card flip or a running timer.
function Native:scheduleTick()
    UIManager:unschedule(self._tick)
    local t = os.time()
    local delay = 60 - tonumber(os.date("%S")) + 1
    if not self.menu_open then
        if CARD_PAGES[self.page_name or ""] then delay = math.min(delay, math.max(3, self:cardNext(t))) end
        if self.page_name == "focus" then
            local p = self.pomo
            if (p.state == "focus" or p.state == "break") and p.stop > t then delay = math.min(delay, math.max(1, p.stop - t)) end
        end
    end
    UIManager:scheduleIn(delay, self._tick)
end

function Native:onTick()
    UIManager:broadcastEvent(Event:new("InputEvent"))
    pcall(function() Device:getPowerDevice():resetT1Timeout() end)
    if self:inNight() then
        if not self.night_done then
            self.night_done = true
            self:sleepNow()
            return
        end
    else
        self.night_done = false
    end
    self:syncIfNeeded()
    self:fetchSleep()
    if not self.menu_open then self:rebuild() end
    self:scheduleTick()
end

-- Black flash then white flash, then the page again: the strongest ghost remover e-ink offers.
function Native:deepClean()
    local black = FrameContainer:new{ background = BLACK, bordersize = 0, margin = 0, padding = 0,
        width = self.W, height = self.H, HorizontalSpan:new{ width = 0 } }
    self[1] = black
    UIManager:setDirty(self, function() return "full", self.dimen end)
    UIManager:scheduleIn(1.2, function()
        self[1] = FrameContainer:new{ background = WHITE, bordersize = 0, margin = 0, padding = 0,
            width = self.W, height = self.H, HorizontalSpan:new{ width = 0 } }
        UIManager:setDirty(self, function() return "full", self.dimen end)
        UIManager:scheduleIn(1.2, function() self:rebuild(true) self:scheduleTick() end)
    end)
end

function Native:sleepNow()
    self:fetchSleep(true)
    UIManager:scheduleIn(1, function() Device:onPowerEvent("Suspend") end)
end

------------------------------------------------------------------- input ----

function Native:turnPage(d)
    local n = #((self.data or {}).pages or { 1 })
    self.menu_open = false
    self.page = (self.page - 1 + d) % math.max(1, n) + 1
    self:rebuild(true)
    self:scheduleTick()
end

function Native:step(d)
    if self.menu_open then return end
    if CARD_PAGES[self.page_name or ""] then
        self:cardAction(d > 0 and "next" or "prev")
        self:rebuild()
        self:scheduleTick()
    else
        self:turnPage(d)
    end
end

function Native:onNextCard() self:step(1) return true end
function Native:onPrevCard() self:step(-1) return true end

function Native:act(a)
    local kind, arg = a:match("^(%a+):?(.*)$")
    if a:sub(1, 1) == "@" then kind, arg = a:match("^@(%a+):?(.*)$") end
    if a == "menu" then
        self.menu_open = true
        self:rebuild(true)
    elseif a == "refresh" then
        self:syncIfNeeded(true)
        self:fetchSleep(true)
        self:rebuild()
    elseif a == "page" then
        self:turnPage(1)
    elseif a == "flip" then
        self:cardAction("flip")
        self:rebuild()
    elseif a:sub(1, 5) == "mode:" then
        self:cardAction("mode", arg)
        self:rebuild()
    elseif a == "pomo" then
        self:pomoToggle()
        self:rebuild()
    elseif a == "quote" then
        self.quote_offset = self.quote_offset + 1
        self:rebuild()
    elseif a:sub(1, 1) == "@" then
        self:menuAction(kind, arg)
        return
    end
    self:scheduleTick()
end

function Native:menuAction(kind, arg)
    if kind == "close" then
        self.menu_open = false
        self:rebuild(true)
    elseif kind == "page" then
        self:turnPage(tonumber(arg) or 1)
        return
    elseif kind == "rot" then
        self:set("rotation", arg)
        self:measure()
        self:fetchSleep(true)
        self:rebuild(true)
    elseif kind == "ref" then
        self:set("refresh_min", tonumber(arg) or 1)
        self:rebuild()
    elseif kind == "full" then
        self:set("full_every", tonumber(arg) or 3)
        self:rebuild(true)
    elseif kind == "clean" then
        self:deepClean()
        return
    elseif kind == "sleepscreen" then
        local on = not self:cfg("sleep_screen", true)
        self:set("sleep_screen", on)
        if on then self:overrideScreensaver() self:fetchSleep(true) else Native.restoreScreensaver(self.settings) end
        self:rebuild()
    elseif kind == "sleep" then
        self.menu_open = false
        self:rebuild()
        self:sleepNow()
    elseif kind == "night" then
        self:set("night", not self:cfg("night", false))
        self:rebuild()
    elseif kind == "nightpreset" then
        local from, to = self:cfg("night_from", 23), self:cfg("night_to", 6)
        local idx = 0
        for i, p in ipairs(NIGHT_PRESETS) do if p[1] == from and p[2] == to then idx = i end end
        local nxt = NIGHT_PRESETS[idx % #NIGHT_PRESETS + 1]
        self:set("night_from", nxt[1])
        self:set("night_to", nxt[2])
        self:rebuild()
    elseif kind == "ui" then
        if arg ~= "native" then self:switchUI(arg) else self.menu_open = false self:rebuild() end
        return
    elseif kind == "refresh" then
        self:syncIfNeeded(true)
        self:rebuild()
    elseif kind == "exit" then
        self:onClose()
        return
    end
    self:scheduleTick()
end

function Native:onTap(_, ges)
    local x, y = self:toLogical(ges.pos.x, ges.pos.y)
    local slop = self:px(10)
    for i = #self.hits, 1, -1 do
        local h = self.hits[i]
        local d = h.frame.dimen
        if d and x >= d.x - slop and x <= d.x + d.w + slop and y >= d.y - slop and y <= d.y + d.h + slop then
            self:act(h.action)
            return true
        end
    end
    return true
end

function Native:onSwipe(_, ges)
    if self.menu_open then return true end
    local d = (SWIPE[self.rot or 0] or SWIPE[0])[ges.direction]
    if d == "west" then self:step(1)
    elseif d == "east" then self:step(-1)
    elseif d == "north" then self:turnPage(1)
    elseif d == "south" then self:turnPage(-1) end
    return true
end

return Native
