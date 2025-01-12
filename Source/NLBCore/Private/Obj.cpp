#include "Obj.h"
#include "NonLinearBook.h"
#include "Theme.h"
#include "Constants.h"

// Initialize string constants with empty strings from Constants
const std::string Obj::DEFAULT_IMAGE_FILE_NAME = nlb::Constants::EMPTY_STRING;
const std::string Obj::DEFAULT_SOUND_FILE_NAME = nlb::Constants::EMPTY_STRING;
const std::string Obj::DEFAULT_VARID = nlb::Constants::EMPTY_STRING;
const std::string Obj::DEFAULT_CONSTRID = nlb::Constants::EMPTY_STRING;
const std::string Obj::DEFAULT_COMMON_TO_ID = nlb::Constants::EMPTY_STRING;
const std::string Obj::DEFAULT_NAME = nlb::Constants::EMPTY_STRING;
const std::string Obj::DEFAULT_MORPH_OVER_ID = nlb::Constants::EMPTY_STRING;
const std::string Obj::DEFAULT_MORPH_OUT_ID = nlb::Constants::EMPTY_STRING;
const std::string Obj::DEFAULT_OFFSET = nlb::Constants::EMPTY_STRING;
const std::string Obj::DEFAULT_CONTAINER_ID = nlb::Constants::EMPTY_STRING;

// Initialize MultiLangString constants with empty texts
const MultiLangString Obj::DEFAULT_TEXT = MultiLangString::createEmptyText();
const MultiLangString Obj::DEFAULT_ACT_TEXT = MultiLangString::createEmptyText();
const MultiLangString Obj::DEFAULT_NOUSE_TEXT = MultiLangString::createEmptyText();
const MultiLangString Obj::DEFAULT_DISP = MultiLangString::createEmptyText();

// Note: Boolean and enum constants are already initialized in the header file
// as they are compile-time constants. Same applies to the integer constants.
// For reference, these are:
//
// static const bool DEFAULT_ANIMATED_IMAGE = false;
// static const bool DEFAULT_SOUND_SFX = true;
// static const bool DEFAULT_SUPPRESS_DSC = false;
// static const bool DEFAULT_GRAPHICAL = false;
// static const bool DEFAULT_SHOW_ON_CURSOR = false;
// static const bool DEFAULT_PRESERVED = false;
// static const bool DEFAULT_LOAD_ONCE = false;
// static const bool DEFAULT_COLLAPSABLE = false;
// static const bool DEFAULT_CLEAR_UNDER_TOOLTIP = false;
// static const bool DEFAULT_ACT_ON_KEY = false;
// static const bool DEFAULT_CACHE_TEXT = false;
// static const bool DEFAULT_LOOPED = false;
// static const bool DEFAULT_NO_REDRAW_ON_ACT = false;
// static const bool DEFAULT_TAKABLE = false;
// static const bool DEFAULT_CALLBACK = false;
// static const bool DEFAULT_IMAGE_IN_SCENE = true;
// static const bool DEFAULT_IMAGE_IN_INVENTORY = true;
// static const MovementDirection DEFAULT_MOVEMENT_DIRECTION = MovementDirection::None;
// static const Effect DEFAULT_EFFECT = Effect::None;
// static const int DEFAULT_START_FRAME = 0;
// static const int DEFAULT_MAX_FRAME = 8;
// static const int DEFAULT_PRELOAD_FRAMES = 0;
// static const int DEFAULT_PAUSE_FRAMES = 0;