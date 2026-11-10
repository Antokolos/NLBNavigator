
-- NLBNavigator: служебные команды для сверки с плеером. Дописывается в конец main.lua КОПИИ
-- экспорта (overlay); исходный экспорт не менять. В instead-cli команды вводятся с косой чертой:
-- /click, /var, /actid, /invid, /useid, /nlbgo; nlbnoop — без косой черты.

-- "/click <имя объекта>" — щелчок по вложенному предмету инвентаря
-- (instead-cli нумерует только предметы верхнего уровня)
local nlbnav_cmd = iface.cmd
function iface:cmd(inp)
    local name = inp:match('^click%s+(.+)$')
    if name then
        local o = _G['_' .. name]
        if not o then return 'NLBNAV: нет объекта ' .. name end
        -- временно поднимаем объект на верхний уровень инвентаря, чтобы INSTEAD его нашёл
        local was = inv():srch(o)
        if not was then inv():add(o) end
        local r, v = nlbnav_cmd(self, 'use,' .. stead.deref(o))
        if not was then inv():del(o) end
        return r, v
    end
    return nlbnav_cmd(self, inp)
end

-- "/var a b c" — значения переменных игры для сверки
local nlbnav_cmd2 = iface.cmd
function iface:cmd(inp)
    local names = inp:match('^var%s+(.+)$')
    if names then
        local out = {}
        for n in names:gmatch('%S+') do
            local v = _G['_' .. n]
            if type(v) == 'table' then v = v.nam end
            table.insert(out, n .. '=' .. tostring(v))
        end
        return 'NLBVARS ' .. table.concat(out, ' ')
    end
    return nlbnav_cmd2(self, inp)
end

-- действия по id объекта книги (для сверки сценариев): /actid, /invid, /useid
local function nlbnav_obj(id) return _G['v_' .. id:gsub('-', '_')] end
local nlbnav_cmd3 = iface.cmd
function iface:cmd(inp)
    local kind, rest = inp:match('^(%a+id)%s+(.+)$')
    if kind then
        local a, b = rest:match('^(%S+)%s*(%S*)$')
        local oa, ob = nlbnav_obj(a), (b ~= '' and nlbnav_obj(b) or nil)
        if not oa or (b ~= '' and not ob) then return 'NLBNAV: нет объекта ' .. rest end
        -- объекты могут быть вложенными: временно поднимаем их, чтобы INSTEAD их нашёл
        local tmp = {}
        local function lift(o, list)
            if not list:srch(o) then list:add(o); table.insert(tmp, {o, list}) end
        end
        local r, v
        if kind == 'actid' then lift(oa, here().obj); r, v = nlbnav_cmd3(self, 'act,' .. stead.deref(oa))
        elseif kind == 'invid' then lift(oa, inv()); r, v = nlbnav_cmd3(self, 'use,' .. stead.deref(oa))
        elseif kind == 'useid' then
            lift(oa, inv())
            if not inv():srch(ob) then lift(ob, here().obj) end
            r, v = nlbnav_cmd3(self, 'use,' .. stead.deref(oa) .. ',' .. stead.deref(ob))
        end
        for _, t in ipairs(tmp) do t[2]:del(t[1]) end
        return r, v
    end
    return nlbnav_cmd3(self, inp)
end

-- "/nlbgo <текст ссылки>" — переход по пути (way) комнаты с таким текстом
local nlbnav_cmd4 = iface.cmd
function iface:cmd(inp)
    local text = inp:match('^nlbgo%s+(.+)$')
    if text then
        for _, w in stead.opairs(ways()) do
            local o = stead.ref(w)
            local d = o and stead.call(o, 'disp') or nil
            if type(d) == 'string' and d:gsub('^%s+', ''):gsub('%s+$', '') == text then
                return nlbnav_cmd4(self, 'go,' .. stead.deref(o))
            end
        end
        return 'NLBNAV: нет пути ' .. text
    end
    return nlbnav_cmd4(self, inp)
end

-- "nlbnoop" (без косой черты) — ничего не делает, но instead-cli печатает
-- после ответа пути и инвентарь с номерами, которые действуют для следующей команды
local nlbnav_cmd5 = iface.cmd
function iface:cmd(inp)
    if inp:find('nlbnoop', 1, true) then return 'NOOP', true end
    return nlbnav_cmd5(self, inp)
end
