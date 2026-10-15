// Встроенные функции выражений NLB (cparse). Отдельный файл без заголовков Unreal: в
// builtin-features.inc есть имена INT, BOOL и т.п., которые конфликтуют с типами Windows,
// попадающими сюда через заголовки движка
#include "shunting-yard.h"
#include "builtin-features.inc"

void NLBPlayer_CparseStartup()
{
    cparse_startup();
}
