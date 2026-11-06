local DataStorage = require("datastorage")
local Dispatcher = require("dispatcher")
local LuaSettings = require("luasettings")
local UIManager = require("ui/uimanager")
local WidgetContainer = require("ui/widget/container/widgetcontainer")
local lfs = require("libs/libkoreader-lfs")
local _ = require("gettext")

-- Resolved while the plugin loads, when its directory is on package.path.
local DeskView = require("kindledesk_view")     -- server-rendered images (fallback)
local Native = require("kindledesk_native")     -- drawn on the Kindle itself
local config = require("kindledesk_config")

local KindleDesk = WidgetContainer:extend{
    name = "kindledesk",
    is_doc_only = false,
}

function KindleDesk:init()
    self.settings = LuaSettings:open(DataStorage:getSettingsDir() .. "/kindle_desk.lua")
    -- If KOReader died while the desk was open, put the user's screensaver settings back.
    if not KindleDesk.checked_leftovers then
        KindleDesk.checked_leftovers = true
        DeskView.restoreScreensaver(self.settings)
    end
    Dispatcher:registerAction("kindledesk_open", { category = "none", event = "OpenKindleDesk", title = _("Kindle Desk"), general = true })
    self.ui.menu:registerToMainMenu(self)

    -- "Kindle Desk.sh" in the library drops this flag, then starts KOReader.
    local flag = DataStorage:getSettingsDir() .. "/kindle_desk.autostart"
    local flagged = lfs.attributes(flag, "mode") == "file"
    if flagged then os.remove(flag) end
    if (flagged or self.settings:isTrue("autostart")) and not KindleDesk.autostarted then
        KindleDesk.autostarted = true
        UIManager:scheduleIn(1.5, function() self:open() end)
    end
end

function KindleDesk:open()
    local class = (self.settings:readSetting("ui") == "image") and DeskView or Native
    UIManager:show(class:new{
        server = config.server, token = config.token, settings = self.settings,
        switch_cb = function() self:open() end,
    })
end

function KindleDesk:onOpenKindleDesk()
    self:open()
    return true
end

function KindleDesk:addToMainMenu(menu_items)
    menu_items.kindle_desk = {
        text = _("Kindle Desk"),
        sorting_hint = "more_tools",
        sub_item_table = {
            {
                text = _("Open desk"),
                callback = function() self:open() end,
            },
            {
                text = _("Open desk when KOReader starts"),
                checked_func = function() return self.settings:isTrue("autostart") end,
                callback = function()
                    self.settings:toggle("autostart")
                    self.settings:flush()
                end,
            },
            {
                text = _("Server: ") .. config.server,
                enabled_func = function() return false end,
            },
        },
    }
end

return KindleDesk
