-- Test-only INSTEAD trace adapter.
-- Wrap the outer iface.cmd after export modules have installed their hooks.
local trace_path = os.getenv("NLB_TEST_TRACE")
if not trace_path or not iface or type(iface.cmd) ~= "function" or not nlb then
    return
end

-- Адаптер уже оборачивал ЭТУ функцию? При загрузке main.lua может как переиспользовать прежнюю
-- iface.cmd, так и создать новую. Слабая таблица ключей-функций (в Lua 5.1 у функции нет полей)
-- позволяет обернуть новую функцию и не обернуть повторно старую.
local wrapped = rawget(_G, "__nlb_trace_wrapped")
if type(wrapped) ~= "table" then
    wrapped = setmetatable({}, { __mode = "k" })
    _G.__nlb_trace_wrapped = wrapped
end
if wrapped[iface.cmd] then
    return
end

-- Загрузка сохранения (F9) перезапускает main.lua в том же процессе INSTEAD: адаптер
-- выполняется снова. Нельзя открывать трассу на запись — прежние записи терялись бы, а seq
-- начинался заново, и раннер не мог сопоставить состояние до и после загрузки. Поэтому файл
-- дополняется, seq продолжается с последнего записанного, заголовок пишется только один раз.
local last_seq, need_header = 0, true
local existing = io.open(trace_path, "r")
if existing then
    for line in existing:lines() do
        if line:match("^#") then
            need_header = false
        else
            local seq = tonumber(line:match("^(%d+)"))
            if seq and seq > last_seq then last_seq = seq end
        end
    end
    existing:close()
end

local trace, open_error = io.open(trace_path, last_seq > 0 and "a" or "w")
if not trace then
    print("NLB_TEST_TRACE: cannot open trace file: " .. tostring(open_error))
    return
end

local original_cmd = iface.cmd
local original_fmt = iface.fmt
local sequence = last_seq
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

    -- Экран конца («начать заново»): комната v_<id>_choices, все объекты которой — _try_again
    -- (nlb.lua), как is_end в vn.lua экспорта. Обычный экран вариантов — тоже _choices, но с
    -- вариантами-объектами, поэтому одного screen_kind "choices" мало
    local end_screen = false
    if room_is_choices or location_is_choices then
        end_screen = safe_value(function()
            if type(_G._try_again) ~= "table" then return false end
            local any, only = false, true
            for _, v in ipairs(objs()) do
                any = true
                if stead.nameof(v) ~= stead.nameof(_G._try_again) then only = false end
            end
            return any and only
        end) == true
    end

    local page_name = safe_value(function() return location and location.nam end) or ""
    local background = safe_value(function() return vn and vn._bg end) or ""

    local portion_index, portion_count, more = 0, 0, false
    local pg = _G.paginator
    local real_text = _G.game and game._realdisp
    if type(pg) == "table" and type(real_text) == "string" then
        local delimiter = pg.delim
        if type(delimiter) == "string" and delimiter ~= "" then
            local next_page = tonumber(pg._page) or 1
            local starts = {}
            local cursor = 1
            pcall(function()
                while cursor <= #real_text + 1 do
                    local first, last = real_text:find(delimiter, cursor)
                    local finish = first and (first - 1) or #real_text
                    local segment = real_text:sub(cursor, finish)
                    if segment:find("%S") then
                        starts[#starts + 1] = cursor
                    end
                    if not first then break end
                    cursor = last + 1
                end
            end)
            local completed = 0
            for _, start in ipairs(starts) do
                if start < next_page then completed = completed + 1 end
            end
            portion_count = #starts
            -- _page points to the next visible segment, except on the final segment
            -- where text_page() sets _last without moving _page.
            portion_index = pg._last and completed or math.max(0, completed - 1)
        end
        more = not pg._last
    end

    sequence = sequence + 1
    local fields = {
        sequence, kind, command, arguments, rendered, page_id, location_name, room_name,
        screen_kind, page_name, background, portion_index, portion_count, more,
        room_text, action_text, objects_text, player_text, _wolfiness, _hp,
        _barExit, _forestExit, _wolvesTalk, _guardDone, end_screen
    }
    local encoded = {}
    -- Numeric iteration preserves empty fields when a Lua value is nil.
    for index = 1, 25 do encoded[index] = encode_field(fields[index]) end
    trace:write(table.concat(encoded, "\t") .. "\n")
    trace:flush()
end

if need_header then
    trace:write("# seq\tkind\tcommand\targuments\trendered\tpage_id\tlocation_id\troom_id\tscreen_kind\tpage_name\tbackground\tportion_index\tportion_count\tmore\troom_text\taction_text\tobjects_text\tplayer_text\twolfiness\thp\tbarExit\tforestExit\twolvesTalk\tguardDone\tend_screen\n")
    trace:flush()
end

-- Keep iface.fmt as a secondary diagnostic source for STANDARD exports.
if type(original_fmt) == "function" and not wrapped[iface.fmt] then
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
    wrapped[iface.fmt] = true
end

iface.cmd = function(self, command, ...)
    local arguments = pack(...)
    local returns = pack(original_cmd(self, command, ...))
    local command_name = tostring(command)
    if command_name ~= "user_timer" then
        local result = returns[1]
        local kind = command_name == "inv" and "inventory"
            or command_name == "way" and "event"
            or type(result) == "string" and "scene"
            or "event"
        local argument_text = {}
        for index = 1, arguments.n do argument_text[index] = tostring(arguments[index]) end
        snapshot(command_name, kind, table.concat(argument_text, ","), result, nil, nil, nil, nil)
    end
    return unpack_values(returns, 1, returns.n)
end
wrapped[iface.cmd] = true
