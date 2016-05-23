#pragma once

#include "Utils.h"

namespace Ls {
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

    typedef int WindowStateUpdateFlags;

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
            assert(error == ERROR_SUCCESS);
            error = RegCloseKey(key);
            assert(error == ERROR_SUCCESS);
        }

        HKEY key;
        int error = RegOpenKeyEx(HKEY_CURRENT_USER,
                                 "Software\\Lustrious Paint",
                                 0,
                                 KEY_SET_VALUE,
                                 &key);
        assert(error == ERROR_SUCCESS);

        if(Utils::CheckBit(stateUpdateMask, WINDOW_STATE_UPDATE_X_FLAG)) {
            error = RegSetKeyValue(key, nullptr, "X", REG_DWORD, &state.x, 4);
            assert(error == ERROR_SUCCESS);
        }

        if(Utils::CheckBit(stateUpdateMask, WINDOW_STATE_UPDATE_Y_FLAG)) {
            error = RegSetKeyValue(key, nullptr, "Y", REG_DWORD, &state.y, 4);
            assert(error == ERROR_SUCCESS);
        }

        if(Utils::CheckBit(stateUpdateMask, WINDOW_STATE_UPDATE_WIDTH_FLAG)) {
            error = RegSetKeyValue(key, nullptr, "Width", REG_DWORD, &state.width, 4);
            assert(error == ERROR_SUCCESS);
        }

        if(Utils::CheckBit(stateUpdateMask, WINDOW_STATE_UPDATE_HEIGHT_FLAG)) {
            error = RegSetKeyValue(key, nullptr, "Height", REG_DWORD, &state.height, 4);
            assert(error == ERROR_SUCCESS);
        }

        if(Utils::CheckBit(stateUpdateMask, WINDOW_STATE_UPDATE_MODE_FLAG)) {
            error = RegSetKeyValue(key, nullptr, "Mode", REG_DWORD, &state.mode, 4);
            assert(error == ERROR_SUCCESS);
        }

        error = RegCloseKey(key);
        assert(error == ERROR_SUCCESS);
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
        assert(size == 4);

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
        assert(size == 4);

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
        assert(size == 4);

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
        assert(size == 4);

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
        assert(size == 4);

        RegCloseKey(key);
        assert(error == ERROR_SUCCESS);
        return true;
    }
}