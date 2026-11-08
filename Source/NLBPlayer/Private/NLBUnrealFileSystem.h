#pragma once

#include "nlb/util/FileUtils.h"

#include <memory>

/**
 * Файловая система плеера через файловый слой Unreal (IPlatformFile): ядро NLB читает книгу и
 * пишет сохранения, настройки и журналы так же, как сам движок — на ПК, Android (APK/OBB), Quest
 * и консолях (данные игры в образе, сохранения в хранилище платформы). Ставится при загрузке
 * модуля NLBPlayer (FileUtils::setFileSystem), снимается при выгрузке.
 */
std::shared_ptr<IFileSystem> MakeNLBUnrealFileSystem();
