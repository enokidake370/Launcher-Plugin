#include "main.h"
#include "utils/WUPSConfigItemButtonCombo.h"
#include "utils/logger.h"
#include <nn/ccr/sys.h>
#include <string_view>
#include <vpad/input.h>
#include <wups.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include "sysapp/switch.h"

extern "C" void OSSendAppSwitchRequest(uint32_t,uint32_t,uint32_t);
extern "C" void OSShutdown(uint32_t,uint32_t,uint32_t);

WUPS_PLUGIN_NAME("Launcher plugin");
WUPS_PLUGIN_DESCRIPTION("Launcher plugin.");
WUPS_PLUGIN_VERSION("V1.0");
WUPS_PLUGIN_AUTHOR("Yuta and Jokyusha");
WUPS_PLUGIN_LICENSE("GPLv3");

WUPS_USE_STORAGE("Launcher Plugin");

#define BUTTON_COMBO_HOME_MENU_CONFIG_STRING "buttonComboHomeMenu"
#define BUTTON_COMBO_TV_MENU_CONFIG_STRING   "buttonComboPowerOff"
#define ENABLED_CONFIG_STRING                     "enabled"

bool enabled = true;
uint32_t buttonComboHomeMenu = (VPAD_BUTTON_L | VPAD_BUTTON_R);
uint32_t buttonComboPowerOff = (VPAD_BUTTON_L | VPAD_BUTTON_Y);

INITIALIZE_PLUGIN() {
    initLogging();
    DEBUG_FUNCTION_LINE("init plugin");

    WUPSStorageError storageRes = WUPS_OpenStorage();
    if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to open storage %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
    } else {
        if ((storageRes = WUPS_GetBool(nullptr, ENABLED_CONFIG_STRING, &enabled)) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            if (WUPS_StoreBool(nullptr, ENABLED_CONFIG_STRING, enabled) != WUPS_STORAGE_ERROR_SUCCESS) {
                DEBUG_FUNCTION_LINE_ERR("Failed to store value");
            }
        } else if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to get value %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
        }

        if ((storageRes = WUPS_GetInt(nullptr, BUTTON_COMBO_HOME_MENU_CONFIG_STRING, reinterpret_cast<int32_t *>(&buttonComboHomeMenu))) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            if (WUPS_StoreInt(nullptr, BUTTON_COMBO_HOME_MENU_CONFIG_STRING, (int32_t)buttonComboHomeMenu) != WUPS_STORAGE_ERROR_SUCCESS) {
                DEBUG_FUNCTION_LINE_ERR("Failed to store value");
            }
        } else if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to get value %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
        }

        
        if ((storageRes = WUPS_GetInt(nullptr, BUTTON_COMBO_TV_MENU_CONFIG_STRING, reinterpret_cast<int32_t *>(&buttonComboPowerOff))) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            if (WUPS_StoreInt(nullptr, BUTTON_COMBO_TV_MENU_CONFIG_STRING, (int32_t)buttonComboPowerOff) != WUPS_STORAGE_ERROR_SUCCESS) {
                DEBUG_FUNCTION_LINE_ERR("Failed to store value");
            }
        } else if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to get value %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
        }

        // Close storage
        if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE_ERR("Failed to close storage");
        }
    }

    deinitLogging();
}

void buttonComboItemChanged(ConfigItemButtonCombo *item, uint32_t newValue) {
    if (item && item->configId) {
        DEBUG_FUNCTION_LINE("New value in %s changed: %d", item->configId, newValue);
        if (std::string_view(item->configId) == BUTTON_COMBO_HOME_MENU_CONFIG_STRING) {
            buttonComboHomeMenu = newValue;
            WUPS_StoreInt(nullptr, BUTTON_COMBO_HOME_MENU_CONFIG_STRING, (int32_t)buttonComboHomeMenu);
        } else if (std::string_view(item->configId) == BUTTON_COMBO_TV_MENU_CONFIG_STRING) {
            buttonComboPowerOff = newValue;
            WUPS_StoreInt(nullptr, BUTTON_COMBO_TV_MENU_CONFIG_STRING, (int32_t)buttonComboPowerOff);
        }
    }
}

void boolItemCallback(ConfigItemBoolean *item, bool newValue) {
    if (item && item->configId) {
        DEBUG_FUNCTION_LINE("New value in %s changed: %d", item->configId, newValue);
        if (std::string_view(item->configId) == ENABLED_CONFIG_STRING) {
            enabled = newValue;
            WUPS_StoreBool(nullptr, ENABLED_CONFIG_STRING, enabled);
        }
    }
}

WUPS_CONFIG_CLOSED() {
    
    if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to close storage");
    }
}

ON_APPLICATION_START() {
    initLogging();
}

ON_APPLICATION_ENDS() {
    deinitLogging();
}

WUPS_GET_CONFIG() {
    
    if (WUPS_OpenStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to open storage");
        return 0;
    }

    WUPSConfigHandle config;
    WUPSConfig_CreateHandled(&config, "Launcher plugin.");

    WUPSConfigCategoryHandle cat;
    WUPSConfig_AddCategoryByNameHandled(config, "Settings", &cat);

    WUPSConfigItemBoolean_AddToCategoryHandled(
        config, cat, ENABLED_CONFIG_STRING, "Enable plugin", enabled, &boolItemCallback);

    WUPSConfigItemButtonCombo_AddToCategoryHandled(
        config, cat, BUTTON_COMBO_HOME_MENU_CONFIG_STRING, "FriendList", buttonComboHomeMenu, &buttonComboItemChanged);

    WUPSConfigItemButtonCombo_AddToCategoryHandled(
        config, cat, BUTTON_COMBO_TV_MENU_CONFIG_STRING, "TVii", buttonComboPowerOff, &buttonComboItemChanged);

    return config;
}

DECL_FUNCTION(int32_t, VPADRead, VPADChan chan, VPADStatus *buffer, uint32_t buffer_size, VPADReadError *error) {
    int32_t result = real_VPADRead(chan, buffer, buffer_size, error);

if (!enabled) {
        // If the plugin is disabled, simply call the original VPADRead function without any further processing.
        
        return result;
    
    }
    // Check for button combo to open Home Menu
    if ((buffer->hold & buttonComboHomeMenu) == buttonComboHomeMenu) {
        _SYSSwitchTo(SYSAPP_PFID_FRIENDLIST);
    }
    // Check for button combo to power off
    if ((buffer->hold & buttonComboPowerOff) == buttonComboPowerOff) {
        _SYSSwitchTo(SYSAPP_PFID_TVII);
    }

    return result;
}

WUPS_MUST_REPLACE(VPADRead, WUPS_LOADER_LIBRARY_VPAD, VPADRead);
