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
        encode_field(os.time()),
        encode_field(command),
        encode_field(location and location.nlbid or ""),
        encode_field(location and location.nam or ""),
        encode_field(background),
        encode_field(rendered),
        encode_field(room_text),
        encode_field(action_text),
        encode_field(objects_text),
        encode_field(player_text),
        encode_field(_wolfiness),
        encode_field(_hp)
    }
    trace:write(table.concat(fields, "\t") .. "\n")
    trace:flush()
    return rendered
end
