--[[--
Full-screen Kindle Desk: shows /screen.png from the server, refreshed every N minutes.

The server does all the drawing (including rotation and the settings menu) and tells us which
rectangles of the image are buttons (X-Desk-Zones), so every touch is one request and one redraw.
Swipe left/right: next/previous card (or page). Swipe up/down: page. Page-turn keys: next/previous card.
Power button: sleep screen with fixed useful info.
--]]--

local Blitbuffer = require("ffi/blitbuffer")
local DataStorage = require("datastorage")
local Device = require("device")
local Event = require("ui/event")
local Font = require("ui/font")
local FrameContainer = require("ui/widget/container/framecontainer")
local Geom = require("ui/geometry")
local GestureRange = require("ui/gesturerange")
local ImageWidget = require("ui/widget/imagewidget")
local InputContainer = require("ui/widget/container/inputcontainer")
local LuaSettings = require("luasettings")
local NetworkMgr = require("ui/network/manager")
local TextWidget = require("ui/widget/textwidget")
local UIManager = require("ui/uimanager")
local json = require("json")
local lfs = require("libs/libkoreader-lfs")
local ltn12 = require("ltn12")
local logger = require("logger")
local socket = require("socket")
local socketutil = require("socketutil")
local url_lib = require("socket.url")
local Screen = Device.screen

local IMG = "/tmp/kindle_desk.png"
local SLEEP_DIR = "/tmp/kindle_desk_sleep"
local SLEEP_IMG = SLEEP_DIR .. "/sleep.png"
local LOG = "/mnt/us/koreader/kindle_desk.log"
local SLEEP_MAX_AGE = 600 -- seconds between sleep-screen refreshes

-- Rotation is done by the server (it rotates the image and the tap zones), so KOReader's own
-- screen rotation, gsensor and layout code stay out of the way.
local ROTATIONS = { portrait = 0, landscape_cw = 90, portrait_inv = 180, landscape_ccw = 270 }
-- Physical swipe direction -> direction in the (unrotated) page, per rotation.
local SWIPE = {
    [0] = { east = "east", west = "west", north = "north", south = "south" },
    [90] = { south = "east", north = "west", west = "south", east = "north" },
    [180] = { east = "west", west = "east", north = "south", south = "north" },
    [270] = { north = "east", south = "west", east = "south", west = "north" },
}
local NIGHT_PRESETS = { { 22, 6 }, { 23, 6 }, { 0, 6 }, { 21, 7 } }
local SCREENSAVER_KEYS = { "screensaver_type", "screensaver_dir", "screensaver_show_message" }
local CARD_PAGES = { desk = true, vocab = true }

local DeskView = InputContainer:extend{
    name = "KindleDeskView",
    covers_fullscreen = true,
    server = "",
    token = "",
    draws = 0,
    page = 0,
    npages = 1,
    page_name = "desk",
    zones = {},
    menu_open = false,
    next_event = nil,
    sleep_at = 0,
}

local function now() return socket.gettime() end

-- Timing/error log next to KOReader, so slowness can be diagnosed from a USB cable.
local function log(fmt, ...)
    local ok, line = pcall(string.format, fmt, ...)
    local f = io.open(LOG, "a")
    if f then
        f:write(os.date("%H:%M:%S "), ok and line or fmt, "\n")
        f:close()
    end
end

local function settings()
    return LuaSettings:open(DataStorage:getSettingsDir() .. "/kindle_desk.lua")
end

function DeskView:cfg(key, default)
    local v = self.settings:readSetting(key)
    if v == nil then return default end
    return v
end

function DeskView:set(key, value)
    self.settings:saveSetting(key, value)
    self.settings:flush()
end

function DeskView:rot() return ROTATIONS[self:cfg("rotation", "portrait")] or 0 end

function DeskView:init()
    self.settings = self.settings or settings()
    self.dimen = Geom:new{ x = 0, y = 0, w = Screen:getWidth(), h = Screen:getHeight() }
    if Device:hasKeys() then
        self.key_events.NextCard = { { Device.input.group.PgFwd } }
        self.key_events.PrevCard = { { Device.input.group.PgBack } }
    end
    if Device:isTouchDevice() then
        self.ges_events.Tap = { GestureRange:new{ ges = "tap", range = self.dimen } }
        self.ges_events.Swipe = { GestureRange:new{ ges = "swipe", range = self.dimen } }
    end
    self[1] = FrameContainer:new{
        background = Blitbuffer.COLOR_WHITE, bordersize = 0, padding = 0, margin = 0,
        width = self.dimen.w, height = self.dimen.h,
        TextWidget:new{ text = "Kindle Desk ...", face = Font:getFace("cfont", 28) },
    }
    self._tick = function() self:onTick() end
end

---------------------------------------------------------------- lifecycle ----

function DeskView:overrideScreensaver()
    if not self:cfg("sleep_screen", true) or self.settings:has("saved_screensaver") then return end
    local saved = {}
    for _, k in ipairs(SCREENSAVER_KEYS) do saved[k] = G_reader_settings:readSetting(k) end
    self.settings:saveSetting("saved_screensaver", saved)
    self.settings:flush()
    G_reader_settings:saveSetting("screensaver_type", "random_image")
    G_reader_settings:saveSetting("screensaver_dir", SLEEP_DIR)
    G_reader_settings:saveSetting("screensaver_show_message", false)
end

function DeskView.restoreScreensaver(s)
    local saved = s:readSetting("saved_screensaver")
    if not saved then return end
    for _, k in ipairs(SCREENSAVER_KEYS) do
        if saved[k] == nil then G_reader_settings:delSetting(k) else G_reader_settings:saveSetting(k, saved[k]) end
    end
    s:delSetting("saved_screensaver")
    s:flush()
end

function DeskView:onShow()
    if (lfs.attributes(LOG, "size") or 0) > 200000 then os.remove(LOG) end
    log("--- desk opened %dx%d rot=%d", self.dimen.w, self.dimen.h, self:rot())
    lfs.mkdir(SLEEP_DIR)
    self:overrideScreensaver()
    UIManager:preventStandby()
    UIManager:setDirty(self, function() return "full", self.dimen end)
    UIManager:nextTick(function()
        self:refresh(nil, true)
        self:fetchSleep(true)
    end)
end

function DeskView:onCloseWidget()
    UIManager:unschedule(self._tick)
    UIManager:allowStandby()
    DeskView.restoreScreensaver(self.settings)
    UIManager:setDirty(nil, "full")
end

function DeskView:onClose()
    UIManager:close(self)
    return true
end

function DeskView:onResume()
    self:refresh()
end

---------------------------------------------------------------- ticking ----

-- Next refresh: the regular interval, or sooner when a flashcard is about to flip or change.
function DeskView:scheduleTick()
    UIManager:unschedule(self._tick)
    local mins = self:cfg("refresh_min", 1)
    local delay = mins * 60 + 1 - tonumber(os.date("%S"))
    if CARD_PAGES[self.page_name] and not self.menu_open and self.next_event then
        delay = math.min(delay, math.max(5, self.next_event))
    end
    UIManager:scheduleIn(delay, self._tick)
end

function DeskView:inNight()
    if not self:cfg("night", false) then return false end
    local from, to = self:cfg("night_from", 23), self:cfg("night_to", 6)
    local h = tonumber(os.date("%H"))
    if from > to then return h >= from or h < to end
    return h >= from and h < to
end

function DeskView:onTick()
    -- Keep the device awake: poke KOReader's autosuspend and the Kindle's own idle (T1) timer.
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
    self:refresh()
    self:fetchSleep()
end

function DeskView:sleepNow()
    self:fetchSleep(true)
    UIManager:scheduleIn(1, function() Device:onPowerEvent("Suspend") end)
end

---------------------------------------------------------------- network ----

local function battery()
    local ok, cap = pcall(function() return Device:getPowerDevice():getCapacity() end)
    return ok and cap or nil
end

-- Returns ok, headers.
function DeskView:get(path, dest)
    local f = io.open(dest, "wb")
    if not f then return false end
    local client = self.server:match("^https") and require("ssl.https") or require("socket.http")
    socketutil:set_timeout(10, 30)
    local ok, code, headers = pcall(function()
        return socket.skip(1, client.request{
            url = self.server .. path .. (path:find("?", 1, true) and "&" or "?") .. "k=" .. self.token,
            method = "GET", sink = ltn12.sink.file(f),
        })
    end)
    socketutil:reset_timeout()
    if not ok then
        log("request error: %s", tostring(code))
        return false
    end
    if code ~= 200 then log("http %s for %s", tostring(code), path:match("^[^?]*")) end
    return code == 200, headers or {}
end

function DeskView:geometry()
    return string.format("w=%d&h=%d&rot=%d", self.dimen.w, self.dimen.h, self:rot())
end

function DeskView:menuQuery()
    return string.format("&menu=1&m_rot=%s&m_ref=%d&m_sl=%d&m_ng=%d&m_nf=%d&m_nt=%d",
        self:cfg("rotation", "portrait"), self:cfg("refresh_min", 1), self:cfg("sleep_screen", true) and 1 or 0,
        self:cfg("night", false) and 1 or 0, self:cfg("night_from", 23), self:cfg("night_to", 6))
end

-- The sleep screen is a normal image in a folder KOReader's screensaver picks from. It is refreshed
-- in the background-ish (every SLEEP_MAX_AGE seconds), never on a user tap, so taps stay fast.
function DeskView:fetchSleep(force)
    if not self:cfg("sleep_screen", true) then return end
    if not force and os.time() - self.sleep_at < SLEEP_MAX_AGE then return end
    local bat = battery()
    local t0 = now()
    local path = "/sleep.png?" .. self:geometry() .. (bat and ("&bat=" .. bat) or "")
    if self:get(path, SLEEP_IMG .. ".part") then
        os.rename(SLEEP_IMG .. ".part", SLEEP_IMG)
        self.sleep_at = os.time()
        log("sleep image %.2fs", now() - t0)
    end
end

-- action: optional server action ("next", "mode?m=vi2en", ...) executed in the same request.
function DeskView:refresh(action, first)
    local t0 = now()
    local path = string.format("/screen.png?p=%d&%s", self.page, self:geometry())
    if self.menu_open then path = path .. self:menuQuery() end
    if action then path = path .. "&do=" .. url_lib.escape(action) end

    local ok, headers = self:get(path, IMG .. ".part")
    if not ok then
        -- Probably offline: let KOReader bring Wi-Fi up, then try once more.
        NetworkMgr:runWhenOnline(function()
            local ok2, h2 = self:get(path, IMG .. ".part")
            if ok2 then self:onFetched(h2, t0, action) end
            self:scheduleTick()
        end)
        self:scheduleTick() -- keep ticking (and retrying) even if Wi-Fi never comes back
        return
    end
    self:onFetched(headers, t0, action)
    self:scheduleTick()
end

function DeskView:onFetched(headers, t0, action)
    local t1 = now()
    os.rename(IMG .. ".part", IMG)
    self.npages = tonumber(headers["x-desk-pages"]) or self.npages
    self.page_name = headers["x-desk-page"] or self.page_name
    self.next_event = tonumber(headers["x-desk-next"])
    local zok, zones = pcall(json.decode, headers["x-desk-zones"] or "[]")
    self.zones = zok and zones or {}
    self:showImage()
    log("%s: fetch %.2fs, show %.2fs", action or "refresh", t1 - t0, now() - t1)
end

function DeskView:showImage()
    if self[1] and self[1].free then self[1]:free() end
    self[1] = ImageWidget:new{
        file = IMG, file_do_cache = false,
        width = self.dimen.w, height = self.dimen.h, scale_factor = 0,
    }
    self.draws = self.draws + 1
    local every = self:cfg("full_every", 3)
    local mode = (self.next_full or every <= 1 or self.draws % every == 0) and "full" or "partial"
    self.next_full = false
    UIManager:setDirty(self, function() return mode, self.dimen end)
end

------------------------------------------------------------------ input ----

function DeskView:turnPage(d)
    self.next_full = true
    self.menu_open = false
    self.page = (self.page + d) % math.max(1, self.npages)
    self:refresh()
end

function DeskView:step(d)
    if self.menu_open then return end
    if CARD_PAGES[self.page_name] then
        self:refresh(d > 0 and "next" or "prev")
    else
        self:turnPage(d)
    end
end

function DeskView:onNextCard() self:step(1) return true end
function DeskView:onPrevCard() self:step(-1) return true end

-- "@..." actions are handled here; everything else is sent to the server.
function DeskView:localAction(a)
    local kind, arg = a:match("^@(%a+):?(.*)$")
    if kind == "close" then
        self.menu_open = false
        self.next_full = true
        self:refresh()
    elseif kind == "page" then
        self:turnPage(tonumber(arg) or 1)
    elseif kind == "rot" then
        self:set("rotation", arg)
        self:fetchSleep(true)
        self:refresh()
    elseif kind == "ref" then
        self:set("refresh_min", tonumber(arg) or 1)
        self:refresh()
    elseif kind == "sleepscreen" then
        local on = not self:cfg("sleep_screen", true)
        self:set("sleep_screen", on)
        if on then self:overrideScreensaver() else DeskView.restoreScreensaver(self.settings) end
        self:refresh()
    elseif kind == "sleep" then
        self.menu_open = false
        self:sleepNow()
    elseif kind == "night" then
        self:set("night", not self:cfg("night", false))
        self:refresh()
    elseif kind == "nightpreset" then
        local from, to = self:cfg("night_from", 23), self:cfg("night_to", 6)
        local idx = 0
        for i, p in ipairs(NIGHT_PRESETS) do if p[1] == from and p[2] == to then idx = i end end
        local nxt = NIGHT_PRESETS[idx % #NIGHT_PRESETS + 1]
        self:set("night_from", nxt[1])
        self:set("night_to", nxt[2])
        self:refresh()
    elseif kind == "ui" then
        self:set("ui", arg)
        if arg ~= "image" then
            UIManager:close(self)
            if self.switch_cb then UIManager:nextTick(function() self.switch_cb() end) end
        else
            self.menu_open = false
            self:refresh()
        end
    elseif kind == "refresh" then
        self:refresh()
    elseif kind == "exit" then
        self:onClose()
    end
end

function DeskView:onTap(_, ges)
    local rx, ry = ges.pos.x / self.dimen.w, ges.pos.y / self.dimen.h
    for i = #self.zones, 1, -1 do
        local z = self.zones[i]
        if rx >= z.x and rx <= z.x + z.w and ry >= z.y and ry <= z.y + z.h then
            local a = z.a
            if a:sub(1, 1) == "@" then self:localAction(a)
            elseif a == "menu" then self.menu_open = true self.next_full = true self:refresh()
            elseif a == "refresh" then self:refresh()
            elseif a:match("^page%?d=") then self:turnPage(tonumber(a:match("d=(-?%d+)")) or 1)
            else self:refresh(a) end
            return true
        end
    end
    return true
end

function DeskView:onSwipe(_, ges)
    if self.menu_open then return true end
    local d = (SWIPE[self:rot()] or SWIPE[0])[ges.direction]
    if d == "west" then self:step(1)
    elseif d == "east" then self:step(-1)
    elseif d == "north" then self:turnPage(1)
    elseif d == "south" then self:turnPage(-1) end
    return true
end

return DeskView
