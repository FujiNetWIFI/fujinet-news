-- tour2.lua: goto-page entry and server-side paging, headless.
--
-- Launch, Top Stories, goto page 2 (digit + commit), then stick-left back
-- to page 1. Snapshots at each stop.
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
        emu.print_info("tour2.lua: snapshot (" .. what .. ")")
        manager.machine.video:snapshot()
    end)
end

press(3.0,  "KEYPAD3", 0x10)   -- keypad 1: launch from the menu
press(8.0,  "KEYPAD3", 0x10)   -- keypad 1: Top Stories
press(14.0, "KEYPAD2", 0x10)   -- keypad 2: goto-page entry, seeded 2
snap(15.0, "goto prompt")
press(16.0, "KEYPAD0", 0x20)   -- keypad =: jump to page 2
snap(22.0, "page 2")
press(23.0, "HANDLE", 0x04)  -- stick left: back to page 1
snap(29.0, "page 1")
press(30.0, "HANDLE", 0x10)  -- trigger: read the first article
snap(36.0, "reader, lighter paper")

table.sort(acts, function(a, b) return a[1] < b[1] end)

local i, frame = 1, 0
emu.register_frame(function()
    frame = frame + 1
    while i <= #acts and frame >= acts[i][1] do
        acts[i][2]()
        i = i + 1
    end
end)
