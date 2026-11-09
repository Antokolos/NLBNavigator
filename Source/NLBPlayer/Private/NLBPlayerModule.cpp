#include "Modules/ModuleManager.h"

#include "NLBPlatformHooks.h"
#include "nlb/util/FileUtils.h"

void NLBPlayer_CparseStartup();  // NLBCparseStartup.cpp

/** Модуль плеера: при загрузке регистрирует встроенные функции выражений NLB (cparse) и ставит
 *  файловую систему плеера через Unreal (книга, сохранения, журналы — файловым слоем движка) */
class FNLBPlayerModule : public IModuleInterface {
public:
    virtual void StartupModule() override {
        NLBPlayer_CparseStartup();
        // Unreal, или обёртка закрытого плагина платформы над ней (FNLBPlatformHooks)
        FNLBPlatformHooks::InstallFileSystem();
    }
    virtual void ShutdownModule() override {
        FileUtils::setFileSystem(nullptr);  // снова обычная
    }
};

IMPLEMENT_MODULE(FNLBPlayerModule, NLBPlayer)
