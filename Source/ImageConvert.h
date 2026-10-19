#pragma once

#include <string>

/**
 * PNG -> JPG для упаковки книги (nlb_main pack), как экспорт NLBB (stb_image, stb_image_write).
 * transparentToMagenta: прозрачные точки — на пурпурном фоне (255, 0, 255), как JPG_BGCOLOR NLBB
 * (пресет COMPRESSED); иначе картинка, где есть прозрачность, не конвертируется (false).
 */
bool convertPngToJpg(const std::string& png, int quality, bool transparentToMagenta, std::string& jpg);
