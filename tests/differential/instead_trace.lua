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
local last_story_rendered = ""

local function page_id(location)
    if location and location.nlbid then
        return tostring(location.nlbid)
    end
    -- STEAD2 exports UUID page names as globals such as v_f124...; the room
    -- object itself does not necessarily expose the UUID as an nlbid field.
    for name, value in pairs(_G) do
        if value == location then
            local id = tostring(name):match("^v_([%x]+_[%x]+_[%x]+_[%x]+_[%x]+)$")
            if id then
                return (id:gsub("_", "-"))
            end
        end
    end
    return ""
end

local function encode_field(value)
    local text = value == nil and "" or tostring(value)
    return (text:gsub("[%%\t\r\n]", function(character)
        return string.format("%%%02X", string.byte(character))
    end))
end

trace:write("# epoch\tcommand\tpage_id\tpage_name\tbackground\trendered\troom_text\taction_text\tobjects_text\tplayer_text\twolfiness\thp\n")
trace:flush()

iface.fmt = function(self, command, state_changed, moved, room_text, action_text, objects_text, player_text)
    -- Preserve the exact return value from INSTEAD. The trace adapter must
    -- never alter the text that the game displays.
    local rendered, extra = original_format(
        self, command, state_changed, moved,
        room_text, action_text, objects_text, player_text
    )

    -- Inventory refreshes happen after the VN text was rendered. Keep the
    -- last non-empty story output so an inventory update cannot replace it.
    if command ~= "inv" and type(rendered) == "string" and rendered:match("%S") then
        last_story_rendered = rendered
    end

    local trace_rendered = rendered
    if command == "inv" and last_story_rendered ~= "" then
        trace_rendered = last_story_rendered
    end

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
        encode_field(page_id(location)),
        encode_field(location and location.nam or ""),
        encode_field(background),
        encode_field(trace_rendered),
        encode_field(room_text),
        encode_field(action_text),
        encode_field(objects_text),
        encode_field(player_text),
        encode_field(_wolfiness),
        encode_field(_hp)
    }
    trace:write(table.concat(fields, "\t") .. "\n")
    trace:flush()
    return rendered, extra
end
