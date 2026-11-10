
-- Дополнительные заглушки для VN-экспортов (vn.lua, vnspr.lua): добавлены при восстановлении,
-- в исходной сверке («Слуга Ра», Frontier) их не было.
instead_sprite_alpha = function(...) return 'spr' end
instead_sprite_pixels = function(...) return 'spr' end
instead_sprite_colorkey = function(...) return nil end
if debug == nil then debug = { traceback = function() return '' end } end
