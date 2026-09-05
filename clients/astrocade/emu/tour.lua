-- tour.lua: headless walk of every screen for visual checks.
--
-- Launch, topics (move the bar), Top Stories, move the list bar, open the
-- article, info overlay, back out to the list (cache redraw) and topics.
-- Snapshots at each stop.
--
--   mame ... -autoboot_script emu/tour.lua -video none -sound none \
--        -seconds_to_run 45 -snapshot_directory build
local FPS = 60

local function port_by_suffix(suffix)
    for tag, port in pairs(manager.machine.ioport.ports) do
        if tag:sub(-#suffix) == suffix then return port end
    end
    return nil
end

local function tap(name, field)
    local p = port_by_suffix(name)
    if p then p:field(field):set_value(1) end
end
local function untap(name, field)
    local p = port_by_suffix(name)
    if p then p:field(field):clear_value() end
end

local acts = {}
local function at(sec, fn) acts[#acts + 1] = {math.floor(sec * FPS), fn} end
local function press(sec, name, field)
    at(sec, function() tap(name, field) end)
    at(sec + 0.13, function() untap(name, field) end)
end
local function snap(sec, what)
    at(sec, function()
        emu.print_info("tour.lua: snapshot (" .. what .. ")")
        manager.machine.video:snapshot()
    end)
end

press(3.0,  "KEYPAD3", 0x10)   -- keypad 1: launch from the menu
snap(5.0, "topics")
press(6.0,  "KEYPAD1", 0x01)   -- keypad v: bar to World News
snap(7.0, "topics, bar moved")
press(8.0,  "KEYPAD3", 0x10)   -- keypad 1: Top Stories
snap(14.0, "article list")
press(15.0, "KEYPAD1", 0x01)   -- keypad v: bar to the second article
snap(16.5, "list, bar moved")
press(17.5, "KEYPAD0", 0x20)   -- keypad =: read it
snap(23.0, "reader")
press(24.0, "KEYPAD2", 0x20)   -- keypad 0: info overlay
snap(26.0, "info")
press(27.0, "KEYPAD0", 0x20)   -- any key: back to the body
snap(28.5, "reader again")
press(29.5, "KEYPAD3", 0x20)   -- keypad CE: back to the list (cache)
snap(31.0, "list from cache")
press(32.0, "KEYPAD3", 0x01)   -- keypad C: back to topics
snap(33.5, "topics again")

table.sort(acts, function(a, b) return a[1] < b[1] end)

local i, frame = 1, 0
emu.register_frame(function()
    frame = frame + 1
    while i <= #acts and frame >= acts[i][1] do
        acts[i][2]()
        i = i + 1
    end
end)
