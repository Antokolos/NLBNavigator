#ifndef NON_LINEAR_BOOK_H
#define NON_LINEAR_BOOK_H

#include <string>
#include <map>
#include <vector>
#include <set>
#include <memory>
#include "Page.h"
#include "Theme.h"
#include "MediaExportParameters.h"
#include "NLBExceptions.h"
#include "History.h"

class NLBCORE_API NonLinearBook {
public:
    static constexpr const char* TRUE_VARID = "TRUE";
    static constexpr const char* FALSE_VARID = "FALSE";
    static constexpr const char* LC_VARID_PREFIX = "LC_";
    static constexpr const char* LC_VARID_SEPARATOR_OUT = "_OUT_";

    static constexpr const char* SOUND_DIR_NAME = "sound";
    static constexpr const char* IMAGES_DIR_NAME = "images";
    static constexpr const char* DEFAULT_STARTPOINT = "";
    static constexpr const char* DEFAULT_LANGUAGE = "ru";
    static constexpr const char* DEFAULT_LICENSE = "";
    static constexpr bool DEFAULT_FULL_AUTOWIRE = false;
    static constexpr bool DEFAULT_SUPPRESS_MEDIA = false;
    static constexpr bool DEFAULT_SUPPRESS_SOUND = false;
    static constexpr const char* DEFAULT_TITLE = "";
    static constexpr const char* DEFAULT_AUTHOR = "";
    static constexpr const char* DEFAULT_VERSION = "";
    static constexpr const char* DEFAULT_PERFECT_GAME_ACHIEVEMENT_NAME = "";

    struct ModuleInfo {
        std::string modulePageId;
        std::string moduleName;
        int depth;

        ModuleInfo(const std::string& modulePageId, const std::string& moduleName, int depth)
            : modulePageId(modulePageId), moduleName(moduleName), depth(depth) {}
    };

    virtual ~NonLinearBook() = default;

    virtual std::set<std::string> getAllAchievementNames(bool recursive) const = 0;
    virtual std::string getPerfectGameAchievementName() const = 0;

    virtual bool isEmpty() const = 0;
    virtual std::string getStartPoint() const = 0;
    virtual std::string getLanguage() const = 0;
    virtual std::string getLicense() const = 0;
    virtual Theme getTheme() const = 0;
    virtual bool isFullAutowire() const = 0;
    virtual bool isSuppressMedia() const = 0;
    virtual bool isSuppressSound() const = 0;
    virtual std::string getTitle() const = 0;
    virtual std::string getAuthor() const = 0;
    virtual std::string getVersion() const = 0;

    virtual std::map<std::string, std::shared_ptr<Page>> getPages() const = 0;

    // Добавьте остальные методы интерфейса
};

#endif // NON_LINEAR_BOOK_H
