
-- Заглушки графического API INSTEAD для консольного прогона (NLBNavigator: сверка с экспортом)
local function stub(...) return nil end
for _, n in ipairs({'instead_theme_var', 'instead_menu_toggle', 'instead_sprites_free',
    'instead_sprite_text_size', 'instead_sprite_text', 'instead_sprite_size', 'instead_sprite_scale',
    'instead_sprite_rotate', 'instead_sprite_pixel', 'instead_sprite_load', 'instead_sprite_free',
    'instead_sprite_fill', 'instead_sprite_dup', 'instead_sprite_draw', 'instead_sprite_copy',
    'instead_sprite_compose', 'instead_sprite_alpha', 'instead_font_scaled_size', 'instead_font_load',
    'instead_font_free', 'instead_sprite_pixels', 'instead_mouse_pos', 'instead_mouse_filter',
    'instead_ticks', 'instead_timer', 'instead_busy', 'instead_direct'}) do
    if _G[n] == nil then _G[n] = stub end
end
local theme_vals = { ['scr.w'] = '800', ['scr.h'] = '600', ['win.w'] = '600', ['win.h'] = '500',
    ['inv.w'] = '200', ['inv.h'] = '500', ['win.fnt.size'] = '16', ['inv.fnt.size'] = '16' }
instead_theme_var = function(name, val) if val ~= nil then theme_vals[name] = tostring(val) end; return theme_vals[name] or '0' end
instead_sprite_load = function(...) return 'spr' end
instead_sprite_size = function(...) return 1, 1 end
instead_sprite_text_size = function(...) return 1, 1 end
instead_font_load = function(...) return 'fnt' end
instead_sprite_dup = function(...) return 'spr' end
instead_sprite_text = function(...) return 'spr' end
instead_sprite_scale = function(...) return 'spr' end
instead_sprite_rotate = function(...) return 'spr' end
instead_font_scaled_size = function(s) return s end
local fake_ticks = 0
instead_ticks = function() fake_ticks = fake_ticks + 200; return fake_ticks end
if stead then stead.ticks = instead_ticks; stead.mouse_pos = function() return 0, 0 end end
get_ticks = function() return instead_ticks() end
