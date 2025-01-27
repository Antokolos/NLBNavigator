#include "nlb/domain/ObjImpl.h"
#include "nlb/util/StringHelper.h"
#include "nlb/domain/NonLinearBookImpl.h"
#include "nlb/api/Variable.h"
#include "nlb/api/Constants.h"
#include "nlb/api/Page.h"

// Define static string constants
const std::string ObjImpl::TEXT_SUBDIR_NAME = "text";
const std::string ObjImpl::ACT_TEXT_SUBDIR_NAME = "acttext";
const std::string ObjImpl::NOUSE_TEXT_SUBDIR_NAME = "nousetxt";
const std::string ObjImpl::VARID_FILE_NAME = "varid";
const std::string ObjImpl::CONSTRID_FILE_NAME = "constrid";
const std::string ObjImpl::COMMONTOID_FILE_NAME = "commonto";
const std::string ObjImpl::NAME_FILE_NAME = "name";
const std::string ObjImpl::IMAGE_FILE_NAME = "image";
const std::string ObjImpl::SOUND_FILE_NAME = "sound";
const std::string ObjImpl::SOUND_SFX_FILE_NAME = "soundsfx";
const std::string ObjImpl::SUPPRESS_DSC_FILE_NAME = "suppdsc";
const std::string ObjImpl::ANIMATED_FILE_NAME = "animated";
const std::string ObjImpl::DISP_SUBDIR_NAME = "disp";
const std::string ObjImpl::GRAPHICAL_FILE_NAME = "graphical";
const std::string ObjImpl::SHOW_ON_CURSOR_FILE_NAME = "showoncur";
const std::string ObjImpl::PRESERVED_FILE_NAME = "preserved";
const std::string ObjImpl::LOADONCE_FILE_NAME = "loadonce";
const std::string ObjImpl::COLLAPSABLE_FILE_NAME = "collapsable";
const std::string ObjImpl::MVDIRECTION_FILE_NAME = "mvdirection";
const std::string ObjImpl::EFFECT_FILE_NAME = "effect";
const std::string ObjImpl::START_FRAME_FILE_NAME = "startframe";
const std::string ObjImpl::MAX_FRAME_FILE_NAME = "maxframe";
const std::string ObjImpl::PRELOAD_FRAMES_FILE_NAME = "preloadf";
const std::string ObjImpl::PAUSE_FRAMES_FILE_NAME = "pausef";
const std::string ObjImpl::COORDSOR_FILE_NAME = "coordsor";
const std::string ObjImpl::CLEARUTT_FILE_NAME = "clearutt";
const std::string ObjImpl::ACTONKEY_FILE_NAME = "actonkey";
const std::string ObjImpl::CACHETEXT_FILE_NAME = "cachetxt";
const std::string ObjImpl::NO_REDRAW_ON_ACT_FILE_NAME = "noactredraw";
const std::string ObjImpl::LOOPED_FILE_NAME = "looped";
const std::string ObjImpl::MORPH_OVER_FILE_NAME = "morphover";
const std::string ObjImpl::MORPH_OUT_FILE_NAME = "morphout";
const std::string ObjImpl::OFFSET_FILE_NAME = "offset";
const std::string ObjImpl::TAKABLE_FILE_NAME = "takable";
const std::string ObjImpl::CALLBACK_FILE_NAME = "callback";
const std::string ObjImpl::IMAGE_IN_SCENE_FILE_NAME = "imgscene";
const std::string ObjImpl::IMAGE_IN_INVENTORY_FILE_NAME = "imginv";
const std::string ObjImpl::CONTAINERID_FILE_NAME = "containerid";
