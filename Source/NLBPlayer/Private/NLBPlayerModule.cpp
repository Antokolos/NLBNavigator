#include "Modules/ModuleManager.h"

void NLBPlayer_CparseStartup();  // NLBCparseStartup.cpp

/** Модуль плеера: при загрузке регистрирует встроенные функции выражений NLB (cparse) */
class FNLBPlayerModule : public IModuleInterface {
public:
    virtual void StartupModule() override { NLBPlayer_CparseStartup(); }
};

IMPLEMENT_MODULE(FNLBPlayerModule, NLBPlayer)
