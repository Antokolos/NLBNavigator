#include "PropertyManager.h"

PropertyManager& PropertyManager::getInstance() {
    static PropertyManager instance;
    return instance;
}

const Settings& PropertyManager::getSettings() {
    return getInstance().m_settings;
}