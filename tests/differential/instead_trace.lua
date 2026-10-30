-- Test-only INSTEAD state trace.
-- This file is injected into a disposable checkout after the game's Lua module is loaded.
local trace_path = os.getenv("NLB_TEST_TRACE")
if not trace_path or not iface or not iface.fmt or not nlb then
    return
end

local trace, open_error = io.open(trace_path, "w")
if not trace then
    print("NLB_TEST_TRACE: cannot open trace file: " .. tostring(open_error))
    return
end

local original_format = iface.fmt

local function encode_field(value)
    local text = value == nil and "" or tostring(value)
    return (text:gsub("[%%\t\r\n]", function(character)
        return string.format("%%%02X", string.byte(character))
    end))
end

trace:write("# epoch\tcommand\tpage_id\tpage_name\tbackground\trendered\troom_text\taction_text\tobjects_text\tplayer_text\twolfiness\thp\n")
trace:flush()

iface.fmt = function(self, command, state_changed, moved, room_text, action_text, objects_text, player_text)
    local rendered = original_format(
        self, command, state_changed, moved,
        room_text, action_text, objects_text, player_text
    )

    local location = nlb:curloc()
    if not location then
        location = here()
    end

    local background = ""
    if location and type(location.bgimg) == "function" then
        local ok, result = pcall(location.bgimg, location)
        if ok then
            background = result
        end
    end

    local fields = {
        os.time(),
        command,
        location and location.nlbid or "",
        location and location.nam or "",
        background,
        rendered,
        room_text,
        action_text,
        objects_text,
        player_text,
        _wolfiness,
        _hp
    }

    for index = 1, #fields do
        fields[index] = encode_field(fields[index])
    end
    trace:write(table.concat(fields, "\t") .. "\n")
    trace:flush()
    return rendered
end
