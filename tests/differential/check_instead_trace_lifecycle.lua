-- Стенд для Lua 5.1/LuaJIT. Каждый сценарий получает собственный каталог.
local adapter, directory, scenario = arg[1], arg[2], arg[3]
local path = directory .. "/trace.tsv"
local original_open, original_getenv = io.open, os.getenv
local open_files = 0

os.getenv = function(name)
    if name == "NLB_TEST_TRACE" then return path end
    return original_getenv(name)
end

-- Счётчик учитывает только явно закрытые дескрипторы, независимо от сборки мусора.
io.open = function(...)
    local file, err = original_open(...)
    if not file then return nil, err end
    open_files = open_files + 1
    return {
        lines = function() return file:lines() end,
        write = function(_, ...) return file:write(...) end,
        flush = function() return file:flush() end,
        close = function()
            open_files = open_files - 1
            return file:close()
        end,
    }
end

local function reset_cmd()
    iface.cmd = function(self, command)
        self:fmt(command)
        return "текст команды", nil, 7
    end
end

local function reset_fmt()
    iface.fmt = function() return "текст формата" end
end

iface = {}
v_room_one = {nam = "Первая комната"}
local room = v_room_one
nlb = {curloc = function() return room end}
here = function() return room end
reset_cmd()
reset_fmt()
local expected = {}

local function emit(command, page)
    local a, b, c = iface:cmd(command)
    assert(a == "текст команды" and b == nil and c == 7, "результат iface.cmd изменился")
    expected[#expected + 1] = {command, page}
end

local function verify(file_path, events, offset)
    local file = assert(original_open(file_path, "r"))
    local headers, index = 0, 0
    for line in file:lines() do
        if line:match("^#") then
            assert(index == 0, "заголовок после первой записи")
            headers = headers + 1
        else
            index = index + 1
            local event = assert(events[math.floor((index + 1) / 2)], "лишняя запись")
            local seq, kind, command, page = line:match("^(%d+)\t([^\t]+)\t([^\t]+)\t[^\t]*\t[^\t]*\t([^\t]+)")
            assert(tonumber(seq) == index + offset, ("seq=%s, ожидался %d"):format(tostring(seq), index + offset))
            assert(kind == (index % 2 == 1 and "fmt" or "scene"), "лишняя или пропущенная обёртка")
            assert(command == event[1] and page == event[2], "команда или UUID сцены не совпали")
        end
    end
    file:close()
    assert(headers == 1, "должен быть один заголовок")
    assert(index == 2 * #events, "пропущено событие")
    print(("%s: %d записей, заголовков %d, seq %d..%d"):format(file_path, index, headers, offset + 1, offset + index))
end

if scenario == "append" then
    local file = assert(original_open(path, "w"))
    file:write("# seq\n40\tfmt\told\t\ttext\troom-one\n41\tscene\told\t\ttext\troom-one\n")
    file:close()
    dofile(adapter)
    emit("продолжение", "room-one")
    table.insert(expected, 1, {"old", "room-one"})
    verify(path, expected, 39)
else
    dofile(adapter)
    emit("начало", "room-one")
    local cmd, fmt = iface.cmd, iface.fmt
    if scenario == "cmd" or scenario == "both" then reset_cmd() end
    if scenario == "fmt" or scenario == "both" then reset_fmt() end
    if scenario == "nested" then
        iface.cmd = function(self, command) return cmd(self, command) end
    end
    if scenario == "global-reset" then _G.__nlb_trace_state = nil end
    if scenario == "inflight" then
        iface.cmd = function(self, command)
            reset_cmd()
            reset_fmt()
            dofile(adapter)
            self:fmt(command)
            return "текст команды", nil, 7
        end
    end
    if scenario == "scene" then
        v_room_two = {nam = "Вторая комната"}
        room = v_room_two
    end
    dofile(adapter)
    emit("после", scenario == "scene" and "room-two" or "room-one")
    verify(path, expected, 0)
    if scenario == "repeat" then
        for i = 1, 64 do
            reset_cmd()
            if i % 2 == 0 then reset_fmt() end
            dofile(adapter)
            emit("повтор" .. i, "room-one")
        end
        verify(path, expected, 0)
    elseif scenario == "new-file" then
        path = directory .. "/next.tsv"
        dofile(adapter)
        expected = {}
        emit("новый файл", "room-one")
        verify(path, expected, 0)
    elseif scenario == "unchanged" then
        assert(iface.cmd == cmd and iface.fmt == fmt, "обёртка установлена повторно")
    end
end
assert(open_files == 0, "остались открытые файлы: " .. open_files)
print(scenario .. ": OK, открытых файлов 0")
