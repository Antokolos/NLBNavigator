-- Test-only INSTEAD trace adapter.
-- Wrap the outer iface.cmd after export modules have installed their hooks.
local trace_path = os.getenv("NLB_TEST_TRACE")
if not trace_path or not iface or type(iface.cmd) ~= "function" or not nlb then
    return
end

local trace, open_error = io.open(trace_path, "w")
if not trace then
    print("NLB_TEST_TRACE: cannot open trace file: " .. tostring(open_error))
    return
end

local original_cmd = iface.cmd
local original_fmt = iface.fmt
local sequence = 0
local unpack_values = table.unpack or unpack

local function pack(...)
    return { n = select("#", ...), ... }
end

local function encode_field(value)
    local text = value == nil and "" or tostring(value)
    return (text:gsub("[%%\t\r\n]", function(character)
        return string.format("%%%02X", string.byte(character))
    end))
end

local function safe_value(fn)
    local ok, value = pcall(fn)
    if ok then return value end
    return nil
end

local id_by_object = {}
for name, value in pairs(_G) do
    if type(value) == "table" then
        id_by_object[value] = name
    end
end

local function split_location_id(name)
    if not name then return "", false end
    local choices_id = name:match("^v_(.+)_choices$")
    if choices_id then return (choices_id:gsub("_", "-")), true end
    local page = name:match("^v_(.+)$")
    if page then return (page:gsub("_", "-")), false end
    return "", false
end

local function snapshot(command, kind, arguments, rendered, room_text, action_text, objects_text, player_text)
    local location = safe_value(function() return nlb:curloc() end)
    local room = safe_value(function() return here() end)
    local location_name = location and id_by_object[location] or ""
    local room_name = room and id_by_object[room] or ""
    local page_id, location_is_choices = split_location_id(location_name)
    local room_page_id, room_is_choices = split_location_id(room_name)
    if page_id == "" then page_id = room_page_id end

    local screen_kind = "page"
    if location_is_choices or room_is_choices then
        screen_kind = "choices"
    elseif room ~= nil and location ~= nil and room ~= location then
        screen_kind = "overlay"
    end

    local page_name = safe_value(function() return location and location.nam end) or ""
    local background = safe_value(function() return vn and vn._bg end) or ""

    local portion_index, portion_count, more = 0, 0, false
    local pg = _G.paginator
    local real_text = _G.game and game._realdisp
    if type(pg) == "table" and type(real_text) == "string" then
        local delimiter = pg.delim
        if type(delimiter) == "string" and delimiter ~= "" then
            local total, prefix_count = 0, 0
            local next_page = tonumber(pg._page) or 1
            pcall(function()
                for _ in real_text:gmatch(delimiter) do total = total + 1 end
                local prefix = real_text:sub(1, math.max(0, next_page - 1))
                for _ in prefix:gmatch(delimiter) do prefix_count = prefix_count + 1 end
            end)
            portion_count = total + 1
            portion_index = prefix_count
        end
        more = not pg._last
    end

    sequence = sequence + 1
    local fields = {
        sequence, kind, command, arguments, rendered, page_id, location_name, room_name,
        screen_kind, page_name, background, portion_index, portion_count, more,
        room_text, action_text, objects_text, player_text, _wolfiness, _hp
    }
    local encoded = {}
    for index, value in ipairs(fields) do encoded[index] = encode_field(value) end
    trace:write(table.concat(encoded, "\t") .. "\n")
    trace:flush()
end

trace:write("# seq\tkind\tcommand\targuments\trendered\tpage_id\tlocation_id\troom_id\tscreen_kind\tpage_name\tbackground\tportion_index\tportion_count\tmore\troom_text\taction_text\tobjects_text\tplayer_text\twolfiness\thp\n")
trace:flush()

-- Keep iface.fmt as a secondary diagnostic source for STANDARD exports.
if type(original_fmt) == "function" then
    iface.fmt = function(self, command, state_changed, moved, room_text, action_text, objects_text, player_text, ...)
        local returns = pack(original_fmt(
            self, command, state_changed, moved,
            room_text, action_text, objects_text, player_text, ...
        ))
        if command ~= "inv" and command ~= "user_timer" then
            snapshot(tostring(command), "fmt", "", returns[1], room_text, action_text, objects_text, player_text)
        end
        return unpack_values(returns, 1, returns.n)
    end
end

iface.cmd = function(self, command, ...)
    local arguments = pack(...)
    local returns = pack(original_cmd(self, command, ...))
    local command_name = tostring(command)
    if command_name ~= "user_timer" then
        local result = returns[1]
        local kind = command_name == "inv" and "inventory"
            or type(result) == "string" and "scene"
            or "event"
        local argument_text = {}
        for index = 1, arguments.n do argument_text[index] = tostring(arguments[index]) end
        snapshot(command_name, kind, table.concat(argument_text, ","), result, nil, nil, nil, nil)
    end
    return unpack_values(returns, 1, returns.n)
end
