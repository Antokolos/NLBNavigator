#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "nlb_player.h"
#include "nlb_screen.h"
#include "shunting-yard.h"
#include "builtin-features.inc"

using namespace godot;

static void initialize(ModuleInitializationLevel level) {
    if (level != MODULE_INITIALIZATION_LEVEL_SCENE) return;
    cparse_startup();  // встроенные функции выражений NLB (cparse)
    GDREGISTER_CLASS(NLBPlayer);
    GDREGISTER_CLASS(NLBScreen);
}

static void uninitialize(ModuleInitializationLevel) {}

extern "C" GDExtensionBool GDE_EXPORT nlbnavigator_init(GDExtensionInterfaceGetProcAddress get_proc_address,
                                                        GDExtensionClassLibraryPtr library,
                                                        GDExtensionInitialization* initialization) {
    GDExtensionBinding::InitObject init(get_proc_address, library, initialization);
    init.register_initializer(initialize);
    init.register_terminator(uninitialize);
    init.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    return init.init();
}
