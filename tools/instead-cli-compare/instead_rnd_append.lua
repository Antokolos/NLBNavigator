
-- NLBNavigator: детерминированная случайность для сверки с плеером (пара к nlb_sdl --deterministic).
-- Нужна только книгам с RND/SHUFFLE (Frontier, «Варвар»). Дописывать ПОСЛЕ instead_main_append.lua.
-- k-й вызов rnd(n) даёт (k mod n) + 1; shuffled — тождество (список в nlb.shuffle разворачивается).
-- ВОССТАНОВЛЕНО ПО ОПИСАНИЮ: формат журнала /rndlog в оригинале мог отличаться.
local nlbnav_k = 0
nlbnav_rndlog = {}
function rnd(n)
    local v = (nlbnav_k % n) + 1
    nlbnav_k = nlbnav_k + 1
    table.insert(nlbnav_rndlog, tostring(n) .. ':' .. tostring(v))
    return v
end
shuffled = function(tab) return tab end

-- "/rndlog" — журнал вызовов rnd; "/rndstart N" — начало последовательности
local nlbnav_cmd_rnd = iface.cmd
function iface:cmd(inp)
    if inp == 'rndlog' then return 'NLBRND ' .. table.concat(nlbnav_rndlog, ' ') end
    local k = inp:match('^rndstart%s+(%d+)$')
    if k then nlbnav_k = tonumber(k); return 'NLBRNDSTART ' .. k end
    return nlbnav_cmd_rnd(self, inp)
end
