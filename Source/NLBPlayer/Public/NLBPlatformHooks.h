#pragma once

#include "CoreMinimal.h"

#include <memory>
#include <string>

class IFileSystem;

/**
 * Точка подключения закрытого плагина платформы (консоли) к плееру — без кода SDK в открытом
 * репозитории. Плагин лежит в Platforms/<Платформа>/Plugins закрытого репозитория (docs/consoles.md)
 * и из своего StartupModule:
 *  - оборачивает файловую систему плеера — сохранения и достижения в хранилище платформы
 *    (монтирование, фиксация записи, квоты): SetFileSystemWrapper;
 *  - сообщает, чьи это сохранения, — пользователь платформы: SaveUserId.
 * Достижения и трофеи идут через Online Subsystem движка (FNLBOnlinePlatformServices): на консоли
 * их даёт подсистема платформы, настраивается в Platforms/<Платформа>/Config.
 */
struct NLBPLAYER_API FNLBPlatformHooks
{
	/// Обёртка файловой системы плеера: получает файловую систему Unreal (IPlatformFile), отдаёт
	/// свою — например, пути под SavesRoot() в хранилище платформы, остальное — базовой. Ставится
	/// сразу и сохраняется (если вызвать до загрузки модуля NLBPlayer — применится при загрузке)
	using FWrapper = TFunction<std::shared_ptr<IFileSystem>(std::shared_ptr<IFileSystem>)>;
	static void SetFileSystemWrapper(FWrapper Wrap);

	/// Пользователь платформы, чьи сохранения и достижения: подкаталог в SavesRoot() (пусто —
	/// общие, как на ПК). Спрашивается при каждом открытии книги
	static TFunction<FString()> SaveUserId;

	/// Каталог данных плеера (Unreal-путь): saves/ и achievements.tsv внутри — у каждого
	/// пользователя (SaveUserId) свои
	static FString SavesRoot();

	/// Каталог данных текущего пользователя: SavesRoot() или SavesRoot()/<SaveUserId>
	static FString UserSaveDir();

	/// Путь (UTF-8, как его видит файловая система плеера) — внутри SavesRoot()? Плеер получает
	/// пути и в виде Unreal («../../../…»), и полные — сравниваются оба. Для обёртки файловой
	/// системы: какие пути — сохранения
	static bool IsSavePath(const std::string& Utf8Path);

	/// Файловая система плеера для этой платформы: обёртка над Unreal или сама Unreal
	static void InstallFileSystem();
};
