#pragma once

#include "Utils.h"

namespace ls {
    enum WindowMode
    {
        WINDOW_MODE_NORMAL    = SW_SHOW,
        WINDOW_MODE_MINIMIZED = SW_MINIMIZE,
        WINDOW_MODE_MAXIMIZED = SW_MAXIMIZE
    };

    struct WindowState {
        int x;
        int y;
        int width;
        int height;
        WindowMode mode;
    };

    enum WindowStateUpdateFlagBits
    {
        WINDOW_STATE_UPDATE_X_FLAG      = 1,
        WINDOW_STATE_UPDATE_Y_FLAG      = 2,
        WINDOW_STATE_UPDATE_WIDTH_FLAG  = 4,
        WINDOW_STATE_UPDATE_HEIGHT_FLAG = 16,
        WINDOW_STATE_UPDATE_MODE_FLAG   = 32
    };

    using WindowStateUpdateFlags = int;

    void StoreWindowState(WindowState state, WindowStateUpdateFlags stateUpdateMask) {
        if(!Utils::RegKeyExist(HKEY_CURRENT_USER, "Software\\Lustrious Paint")){
            HKEY key;
            int error = RegCreateKeyEx(HKEY_CURRENT_USER,
                           "Software\\Lustrious Paint",
                           0,
                           nullptr,
                           REG_OPTION_NON_VOLATILE,
                           0,
                           nullptr,
                           &key,
                           nullptr);
            Assert(error == ERROR_SUCCESS, "RegCreateKeyEx");
            error = RegCloseKey(key);
            Assert(error == ERROR_SUCCESS, "RegCloseKey");
        }

        HKEY key;
        int error = RegOpenKeyEx(HKEY_CURRENT_USER,
                                 "Software\\Lustrious Paint",
                                 0,
                                 KEY_SET_VALUE,
                                 &key);
        Assert(error == ERROR_SUCCESS, "RegOpenKeyEx");

        if(Utils::CheckBit(stateUpdateMask, WINDOW_STATE_UPDATE_X_FLAG)) {
            error = RegSetKeyValue(key, nullptr, "X", REG_DWORD, &state.x, 4);
            Assert(error == ERROR_SUCCESS, "RegSetKeyValue");
        }

        if(Utils::CheckBit(stateUpdateMask, WINDOW_STATE_UPDATE_Y_FLAG)) {
            error = RegSetKeyValue(key, nullptr, "Y", REG_DWORD, &state.y, 4);
            Assert(error == ERROR_SUCCESS, "RegSetKeyValue");
        }

        if(Utils::CheckBit(stateUpdateMask, WINDOW_STATE_UPDATE_WIDTH_FLAG)) {
            error = RegSetKeyValue(key, nullptr, "Width", REG_DWORD, &state.width, 4);
            Assert(error == ERROR_SUCCESS, "RegSetKeyValue");
        }

        if(Utils::CheckBit(stateUpdateMask, WINDOW_STATE_UPDATE_HEIGHT_FLAG)) {
            error = RegSetKeyValue(key, nullptr, "Height", REG_DWORD, &state.height, 4);
            Assert(error == ERROR_SUCCESS, "RegSetKeyValue");
        }

        if(Utils::CheckBit(stateUpdateMask, WINDOW_STATE_UPDATE_MODE_FLAG)) {
            error = RegSetKeyValue(key, nullptr, "Mode", REG_DWORD, &state.mode, 4);
            Assert(error == ERROR_SUCCESS, "RegSetKeyValue");
        }

        error = RegCloseKey(key);
        Assert(error == ERROR_SUCCESS, "RegCloseKey");
    }

    bool LoadWindowState(WindowState* state) {
        if(!Utils::RegKeyExist(HKEY_CURRENT_USER, "Software\\Lustrious Paint")) {
            return false;
        }

        HKEY key;
        int error = RegOpenKeyEx(HKEY_CURRENT_USER,
                                 "Software\\Lustrious Paint",
                                 0,
                                 KEY_QUERY_VALUE ,
                                 &key);
        if(error != ERROR_SUCCESS) {
            return false;
        }
        
        DWORD size = 4;
        error = RegGetValue(key,
                    nullptr,
                    "X",
                    RRF_RT_REG_DWORD,
                    nullptr,
                    &(state->x),
                    &size);
        if(error != ERROR_SUCCESS) {
            return false;
        }
        Assert(size == 4, "RegGetValue");

        error = RegGetValue(key,
                    nullptr,
                    "Y",
                    RRF_RT_REG_DWORD,
                    nullptr,
                    &(state->y),
                    &size);
        if(error != ERROR_SUCCESS) {
            return false;
        }
        Assert(size == 4, "RegGetValue");

        error = RegGetValue(key,
                    nullptr,
                    "Width",
                    RRF_RT_REG_DWORD,
                    nullptr,
                    &(state->width),
                    &size);
        if(error != ERROR_SUCCESS) {
            return false;
        }
        Assert(size == 4, "RegGetValue");

        error = RegGetValue(key,
                    nullptr,
                    "Height",
                    RRF_RT_REG_DWORD,
                    nullptr,
                    &(state->height),
                    &size);
        if(error != ERROR_SUCCESS) {
            return false;
        }
        Assert(size == 4, "RegGetValue");

        error = RegGetValue(key,
                    nullptr,
                    "Mode",
                    RRF_RT_REG_DWORD,
                    nullptr,
                    &(state->mode),
                    &size);
        if(error != ERROR_SUCCESS) {
            return false;
        }
        Assert(size == 4, "RegGetValue");

        RegCloseKey(key);
        Assert(error == ERROR_SUCCESS, "RegCloseKey");
        return true;
    }
}