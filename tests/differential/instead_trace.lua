-- Адаптер тестовой трассировки. Устанавливается после загрузки модулей экспорта.
local trace_path = os.getenv("NLB_TEST_TRACE")
if not trace_path or not iface or type(iface.cmd) ~= "function" or not nlb then
    return
end

-- main.lua исполняется повторно при загрузке. Обе обёртки читают одно состояние
-- из _G, а при создании новой Lua VM счётчик восстанавливается из TSV.
local state = rawget(_G, "__nlb_trace_state")
if not state then
    state = {
        wrapped = setmetatable({}, { __mode = "k" }),
        active = {},
        -- STEAD сериализует глобальные таблицы с подчёркиванием. Этот хук
        -- оставляет состояние адаптера вне игрового сохранения.
        save = function() end,
    }
    _G.__nlb_trace_state = state
end

local function append_line(path, line)
    local file, err = io.open(path, "a")
    if not file then
        print("NLB_TEST_TRACE: не удалось открыть файл: " .. tostring(err))
        return false
    end
    local written, write_error = file:write(line .. "\n")
    local closed, close_error = file:close()
    if not written or not closed then
        print("NLB_TEST_TRACE: не удалось записать строку: " .. tostring(write_error or close_error))
        return false
    end
    return true
end

if state.path ~= trace_path then
    local last_seq, need_header = 0, true
    local existing = io.open(trace_path, "r")
    if existing then
        for line in existing:lines() do
            need_header = false
            local seq = tonumber(line:match("^(%d+)\t"))
            if seq and seq > last_seq then last_seq = seq end
        end
        existing:close()
    end
    local header = "# seq\tkind\tcommand\targuments\trendered\tpage_id\tlocation_id\troom_id\tscreen_kind\tpage_name\tbackground\tportion_index\tportion_count\tmore\troom_text\taction_text\tobjects_text\tplayer_text\twolfiness\thp\tbarExit\tforestExit\twolvesTalk\tguardDone\tend_screen"
    if need_header and not append_line(trace_path, header) then
        state.snapshot = nil
        return
    end
    state.path, state.sequence = trace_path, last_seq
end

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

    local sequence = state.sequence + 1
    local fields = {
        sequence, kind, command, arguments, rendered, page_id, location_name, room_name,
        screen_kind, page_name, background, portion_index, portion_count, more,
        room_text, action_text, objects_text, player_text, _wolfiness, _hp,
        _barExit, _forestExit, _wolvesTalk, _guardDone, end_screen
    }
    local encoded = {}
    -- Numeric iteration preserves empty fields when a Lua value is nil.
    for index = 1, 25 do encoded[index] = encode_field(fields[index]) end
    if append_line(state.path, table.concat(encoded, "\t")) then
        state.sequence = sequence
    end
end

state.snapshot = snapshot

-- Старый wrapper может оставаться внутри нового хука экспорта. В цепочке
-- пишет только активная обёртка. Проверяем её до вызова оригинала: load
-- может заменить хук, пока старая команда ещё выполняется. Результат
-- записываем через актуальный snapshot, уже с объектами загруженной сцены.
local function is_active(key, wrapper)
    local current = rawget(_G, "__nlb_trace_state")
    return current and current.active[key] == wrapper
end

local function record(...)
    local current = rawget(_G, "__nlb_trace_state")
    if current and current.snapshot then current.snapshot(...) end
end

local function install(key, make_wrapper)
    local original = iface[key]
    if type(original) ~= "function" then
        state.active[key] = nil
        return
    end
    if not state.wrapped[original] then
        local wrapper
        wrapper = make_wrapper(original, function() return is_active(key, wrapper) end)
        state.wrapped[wrapper] = true
        iface[key] = wrapper
    end
    state.active[key] = iface[key]
end

-- iface.fmt остаётся диагностическим источником для STANDARD.
install("fmt", function(original, active)
    return function(self, command, state_changed, moved, room_text, action_text, objects_text, player_text, ...)
        local log = active()
        local returns = pack(original(
            self, command, state_changed, moved,
            room_text, action_text, objects_text, player_text, ...
        ))
        if log and command ~= "inv" and command ~= "user_timer" then
            record(tostring(command), "fmt", "", returns[1], room_text, action_text, objects_text, player_text)
        end
        return unpack_values(returns, 1, returns.n)
    end
end)

install("cmd", function(original, active)
    return function(self, command, ...)
        local log = active()
        local arguments = pack(...)
        local returns = pack(original(self, command, ...))
        local command_name = tostring(command)
        if log and command_name ~= "user_timer" then
            local result = returns[1]
            local kind = command_name == "inv" and "inventory"
                or command_name == "way" and "event"
                or type(result) == "string" and "scene"
                or "event"
            local argument_text = {}
            for index = 1, arguments.n do argument_text[index] = tostring(arguments[index]) end
            record(command_name, kind, table.concat(argument_text, ","), result, nil, nil, nil, nil)
        end
        return unpack_values(returns, 1, returns.n)
    end
end)
