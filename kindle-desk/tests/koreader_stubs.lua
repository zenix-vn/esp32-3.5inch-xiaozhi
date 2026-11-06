-- Minimal stand-ins for the KOReader widgets used by kindledesk_native.lua.
-- Sizes and paint offsets follow the real implementations (checked against KOReader v2026.07.1), but
-- instead of drawing pixels, paintTo() records draw operations into `OPS` so a preview can be rendered.
-- Text is measured by the Python driver (PIL + the bundled Noto fonts) through MEASURE(text, font, size).

OPS = {}
local function op(t) OPS[#OPS + 1] = t end

local function class(parent)
    local C = {}
    C.__index = C
    if parent then setmetatable(C, { __index = parent }) end
    function C:extend(t)
        local sub = class(self)
        for k, v in pairs(t or {}) do sub[k] = v end
        return sub
    end
    function C:new(o)
        o = o or {}
        setmetatable(o, self)
        o.key_events, o.ges_events = o.key_events or {}, o.ges_events or {} -- InputContainer does this in init
        if o.init then o:init() end
        return o
    end
    return C
end

local Geom = class()
function Geom:new(o) o = o or {} o.x = o.x or 0 o.y = o.y or 0 setmetatable(o, Geom) return o end

local Widget = class()
function Widget:free() end

local WidgetContainer = Widget:extend{}
function WidgetContainer:getSize()
    if self.dimen then return self.dimen end
    if self[1] then return self[1]:getSize() end
    return { w = 0, h = 0 }
end
function WidgetContainer:paintTo(bb, x, y) if self[1] then self[1]:paintTo(bb, x, y) end end

local function lineHeight(face) return math.floor(face.size * 1.36) + 4 end

local TextWidget = Widget:extend{}
function TextWidget:getSize()
    local w = MEASURE(self.text, self.face.name, self.face.size)
    if self.max_width and w > self.max_width then w = self.max_width end
    return { w = w, h = lineHeight(self.face) }
end
function TextWidget:paintTo(bb, x, y)
    local s = self:getSize()
    op{ k = "text", x = x, y = y, w = s.w, h = s.h, text = self.text, font = self.face.name, size = self.face.size, color = self.fgcolor }
end

local TextBoxWidget = Widget:extend{}
function TextBoxWidget:lines()
    local lines, cur = {}, ""
    for word in tostring(self.text):gmatch("%S+") do
        local try = cur == "" and word or (cur .. " " .. word)
        if cur ~= "" and MEASURE(try, self.face.name, self.face.size) > self.width then
            lines[#lines + 1] = cur
            cur = word
        else
            cur = try
        end
    end
    lines[#lines + 1] = cur
    return lines
end
function TextBoxWidget:getSize() return { w = self.width, h = #self:lines() * math.floor(lineHeight(self.face) * 1.1) } end
function TextBoxWidget:paintTo(bb, x, y)
    local lh = math.floor(lineHeight(self.face) * 1.1)
    for i, line in ipairs(self:lines()) do
        local lw = MEASURE(line, self.face.name, self.face.size)
        local ox = self.alignment == "center" and math.floor((self.width - lw) / 2) or 0
        op{ k = "text", x = x + ox, y = y + (i - 1) * lh, w = lw, h = lh, text = line, font = self.face.name, size = self.face.size, color = self.fgcolor }
    end
end

local VerticalSpan = Widget:extend{ width = 0 }
function VerticalSpan:getSize() return { w = 0, h = self.width } end
function VerticalSpan:paintTo() end
local HorizontalSpan = Widget:extend{ width = 0 }
function HorizontalSpan:getSize() return { w = self.width, h = 0 } end
function HorizontalSpan:paintTo() end

local VerticalGroup = Widget:extend{ align = "center" }
function VerticalGroup:getSize()
    if not self._size then
        self._size = { w = 0, h = 0 }
        self._offsets = {}
        for i, c in ipairs(self) do
            local s = c:getSize()
            self._offsets[i] = { x = s.w, y = self._size.h }
            self._size.h = self._size.h + s.h
            if s.w > self._size.w then self._size.w = s.w end
        end
    end
    return self._size
end
function VerticalGroup:paintTo(bb, x, y)
    local size = self:getSize()
    for i, c in ipairs(self) do
        local cs = c:getSize()
        local ox = 0
        if self.align == "center" then ox = math.floor((size.w - cs.w) / 2)
        elseif self.align == "right" then ox = size.w - cs.w end
        c:paintTo(bb, x + ox, y + self._offsets[i].y)
    end
end

local HorizontalGroup = Widget:extend{ align = "center" }
function HorizontalGroup:getSize()
    if not self._size then
        self._size = { w = 0, h = 0 }
        self._offsets = {}
        for i, c in ipairs(self) do
            local s = c:getSize()
            self._offsets[i] = { x = self._size.w, h = s.h }
            self._size.w = self._size.w + s.w
            if s.h > self._size.h then self._size.h = s.h end
        end
    end
    return self._size
end
function HorizontalGroup:paintTo(bb, x, y)
    local size = self:getSize()
    for i, c in ipairs(self) do
        local cs = c:getSize()
        local oy = 0
        if self.align == "center" then oy = math.floor((size.h - cs.h) / 2)
        elseif self.align == "bottom" then oy = size.h - cs.h end
        c:paintTo(bb, x + self._offsets[i].x, y + oy)
    end
end

local FrameContainer = WidgetContainer:extend{ margin = 0, bordersize = 3, padding = 4, color = 0 }
function FrameContainer:getSize()
    local cs = self[1]:getSize()
    self._pt, self._pr = self.padding_top or self.padding, self.padding_right or self.padding
    self._pb, self._pl = self.padding_bottom or self.padding, self.padding_left or self.padding
    return { w = cs.w + (self.margin + self.bordersize) * 2 + self._pl + self._pr,
             h = cs.h + (self.margin + self.bordersize) * 2 + self._pt + self._pb }
end
function FrameContainer:paintTo(bb, x, y)
    local ms = self:getSize()
    if not self.dimen then self.dimen = Geom:new{ x = x, y = y, w = ms.w, h = ms.h }
    else self.dimen.x, self.dimen.y = x, y end
    local w, h = self.width or ms.w, self.height or ms.h
    if self.background or self.bordersize > 0 then
        op{ k = "rect", x = x + self.margin, y = y + self.margin, w = w - 2 * self.margin, h = h - 2 * self.margin,
            radius = self.radius or 0, border = self.bordersize, bg = self.background, color = self.color }
    end
    if self[1] then
        self[1]:paintTo(bb, x + self.margin + self.bordersize + self._pl, y + self.margin + self.bordersize + self._pt)
    end
end

local CenterContainer = WidgetContainer:extend{}
function CenterContainer:paintTo(bb, x, y)
    local cs = self[1]:getSize()
    self[1]:paintTo(bb, x + math.floor((self.dimen.w - cs.w) / 2), y + math.floor((self.dimen.h - cs.h) / 2))
end
local LeftContainer = WidgetContainer:extend{}
function LeftContainer:paintTo(bb, x, y)
    local cs = self[1]:getSize()
    self.dimen.x, self.dimen.y = x, y + math.floor((self.dimen.h - cs.h) / 2)
    self[1]:paintTo(bb, self.dimen.x, self.dimen.y)
end
local RightContainer = WidgetContainer:extend{}
function RightContainer:paintTo(bb, x, y)
    local cs = self[1]:getSize()
    self.dimen.x, self.dimen.y = x + (self.dimen.w - cs.w), y + math.floor((self.dimen.h - cs.h) / 2)
    self[1]:paintTo(bb, self.dimen.x, self.dimen.y)
end

local OverlapGroup = WidgetContainer:extend{}
function OverlapGroup:getSize() return self.dimen end
function OverlapGroup:paintTo(bb, x, y) for _, c in ipairs(self) do c:paintTo(bb, x, y) end end

local LineWidget = Widget:extend{ background = 0 }
function LineWidget:getSize() return self.dimen end
function LineWidget:paintTo(bb, x, y)
    op{ k = "line", x = x, y = y, w = self.dimen.w, h = self.dimen.h, color = self.background, style = self.style }
end

local ImageWidget = Widget:extend{}
function ImageWidget:getSize()
    if self.image then return { w = self.image.w, h = self.image.h } end
    return { w = self.width, h = self.height }
end
function ImageWidget:paintTo(bb, x, y) op{ k = "image", x = x, y = y, w = self.width, h = self.height, file = self.file } end

local InputContainer = WidgetContainer:extend{}

-- Off-screen buffers: painting records ops (through OPS), rotatedCopy only remembers the angle.
local BB = { COLOR_BLACK = 0, COLOR_WHITE = 255, COLOR_GRAY_5 = 0x55, COLOR_LIGHT_GRAY = 0xCC, COLOR_GRAY_D = 0xDD }
local BBobj = {}
BBobj.__index = BBobj
function BB.new(w, h) return setmetatable({ w = w, h = h, rot = 0, pix = {} }, BBobj) end
function BBobj:fill() end
function BBobj:free() end
function BBobj:getWidth() return self.w end
function BBobj:getHeight() return self.h end
function BBobj:setPixel(x, y, c) self.pix[y * 100 + x] = c end
function BBobj:getPixel(x, y)
    local c = self.pix[y * 100 + x] or 255
    return { getColor8 = function() return { a = c } end }
end
function BBobj:rotatedCopy(deg)
    local r = BB.new((deg == 90 or deg == 270) and self.h or self.w, (deg == 90 or deg == 270) and self.w or self.h)
    r.rot = (self.rot + deg) % 360
    if deg == 90 then       -- clockwise: top-left pixel ends up top-right
        for key, c in pairs(self.pix) do
            local y, x = math.floor(key / 100), key % 100
            r.pix[x * 100 + (self.h - 1 - y)] = c
        end
    else r.pix = self.pix end
    return r
end

local timers = {}
local screen = { w = 1264, h = 1680, mode = 0 }
SCREEN = screen
local Screen = {
    bb = { getType = function() return 1 end },
    getWidth = function() return screen.w end, getHeight = function() return screen.h end,
    scaleBySize = function(_, n) return n end, getRotationMode = function() return screen.mode end,
    setRotationMode = function(_, m) screen.mode = m end,
}
local settings = { data = {} }
function settings:readSetting(k) return self.data[k] end
function settings:saveSetting(k, v) self.data[k] = v end
function settings:delSetting(k) self.data[k] = nil end
function settings:has(k) return self.data[k] ~= nil end
function settings:flush() end
SETTINGS_STUB = settings
G_reader_settings = { readSetting = function() end, saveSetting = function() end, delSetting = function() end }

-- tiny JSON decoder (the real code uses KOReader's json module)
local function decode(s)
    local i = 1
    local function ws() i = s:find("%S", i) or #s + 1 end
    local val
    local function str()
        local out = {}
        i = i + 1
        while true do
            local c = s:sub(i, i)
            if c == '"' then i = i + 1 break
            elseif c == "\\" then
                local n = s:sub(i + 1, i + 1)
                if n == "u" then
                    local cp = tonumber(s:sub(i + 2, i + 5), 16)
                    i = i + 6
                    if cp < 0x80 then out[#out + 1] = string.char(cp)
                    elseif cp < 0x800 then out[#out + 1] = string.char(0xC0 + math.floor(cp / 64), 0x80 + cp % 64)
                    else out[#out + 1] = string.char(0xE0 + math.floor(cp / 4096), 0x80 + math.floor(cp / 64) % 64, 0x80 + cp % 64) end
                else
                    local map = { n = "\n", t = "\t", r = "\r", b = "\b", f = "\f" }
                    out[#out + 1] = map[n] or n
                    i = i + 2
                end
            else out[#out + 1] = c i = i + 1 end
        end
        return table.concat(out)
    end
    function val()
        ws()
        local c = s:sub(i, i)
        if c == "{" then
            local t = {}
            i = i + 1 ws()
            if s:sub(i, i) == "}" then i = i + 1 return t end
            while true do
                ws() local k = str() ws() i = i + 1
                t[k] = val() ws()
                local d = s:sub(i, i) i = i + 1
                if d == "}" then return t end
            end
        elseif c == "[" then
            local t = {}
            i = i + 1 ws()
            if s:sub(i, i) == "]" then i = i + 1 return t end
            while true do
                t[#t + 1] = val() ws()
                local d = s:sub(i, i) i = i + 1
                if d == "]" then return t end
            end
        elseif c == '"' then return str()
        elseif s:sub(i, i + 3) == "true" then i = i + 4 return true
        elseif s:sub(i, i + 4) == "false" then i = i + 5 return false
        elseif s:sub(i, i + 3) == "null" then i = i + 4 return nil
        else
            local num = s:match("^-?%d+%.?%d*[eE]?[+-]?%d*", i)
            i = i + #num
            return tonumber(num)
        end
    end
    return val()
end

local mods = {
    ["ffi/blitbuffer"] = BB,
    ["ui/widget/container/centercontainer"] = CenterContainer,
    ["datastorage"] = { getSettingsDir = function() return "/tmp" end },
    ["device"] = { screen = Screen, hasKeys = function() return false end, isTouchDevice = function() return true end,
                   input = { group = {} }, getPowerDevice = function() return { getCapacity = function() return 76 end } end },
    ["ui/event"] = { new = function() return {} end },
    ["ui/font"] = { getFace = function(_, name, size) return { name = name, size = size } end },
    ["ui/widget/container/framecontainer"] = FrameContainer,
    ["ui/geometry"] = Geom,
    ["ui/gesturerange"] = { new = function(_, o) return o end },
    ["ui/widget/horizontalgroup"] = HorizontalGroup,
    ["ui/widget/horizontalspan"] = HorizontalSpan,
    ["ui/widget/imagewidget"] = ImageWidget,
    ["ui/widget/container/inputcontainer"] = InputContainer,
    ["ui/widget/container/leftcontainer"] = LeftContainer,
    ["ui/widget/linewidget"] = LineWidget,
    ["ui/network/manager"] = { runWhenOnline = function(_, cb) cb() end },
    ["ui/widget/overlapgroup"] = OverlapGroup,
    ["ui/widget/container/rightcontainer"] = RightContainer,
    ["ui/widget/textboxwidget"] = TextBoxWidget,
    ["ui/widget/textwidget"] = TextWidget,
    ["ui/uimanager"] = {
        show = function() end, close = function() CLOSED = true end, setDirty = function() end, preventStandby = function() end,
        allowStandby = function() end, nextTick = function(_, f) f() end, broadcastEvent = function() end,
        scheduleIn = function(_, d, f) timers[#timers + 1] = { d, f } end, unschedule = function() end,
        onRotation = function() end,
    },
    ["ui/widget/verticalgroup"] = VerticalGroup,
    ["ui/widget/verticalspan"] = VerticalSpan,
    ["json"] = { decode = decode },
    ["libs/libkoreader-lfs"] = { attributes = function() return nil end, mkdir = function() end },
    ["ltn12"] = { sink = { file = function() end } },
    ["socket"] = { gettime = function() return os.clock() end, skip = function(_, ...) return ... end },
    ["socketutil"] = { set_timeout = function() end, reset_timeout = function() end },
}
for name, m in pairs(mods) do package.preload[name] = function() return m end end
TIMERS = timers
