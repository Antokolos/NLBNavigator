#include "Modules/ModuleManager.h"
#include "shunting-yard.h"
#include "builtin-features.inc"

/** Модуль плеера: при загрузке регистрирует встроенные функции выражений NLB (cparse) */
class FNLBPlayerModule : public IModuleInterface {
public:
    virtual void StartupModule() override { cparse_startup(); }
};

IMPLEMENT_MODULE(FNLBPlayerModule, NLBPlayer)
