#include "Modules/ModuleManager.h"
#include "shunting-yard.h"
#include "builtin-features.inc"

/** Модуль плагина: при загрузке регистрирует встроенные функции выражений NLB (cparse) */
class FNLBNavigatorRuntimeModule : public IModuleInterface {
public:
    virtual void StartupModule() override { cparse_startup(); }
};

IMPLEMENT_MODULE(FNLBNavigatorRuntimeModule, NLBNavigatorRuntime)
