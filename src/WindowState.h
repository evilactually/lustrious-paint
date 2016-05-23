#pragma once

#include "Flags.h"

enum class WindowMode
{
    Normal    = SW_RESTORE,
    Minimized = SW_MINIMIZE,
    Maximized = SW_MAXIMIZE
};

struct WindowState {
    int x;
    int y;
    int width;
    int height;
    WindowMode mode;
};

enum class WindowStateUpdateFlagBits
{
    X      = 1,
    Y      = 2,
    Width  = 4,
    Height = 16,
    Mode   = 32
};

using WindowStateUpdateFlags = Flags<WindowStateUpdateFlagBits, int>;

inline WindowStateUpdateFlags operator|(WindowStateUpdateFlagBits a, WindowStateUpdateFlagBits b)
{
    return WindowStateUpdateFlags(a) | b;
}

bool RegKeyExist(_In_     HKEY    hKey,
                 _In_opt_ LPCTSTR lpSubKey) {
    HKEY key;
    bool exists = RegOpenKeyEx(hKey, lpSubKey, 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS;
    RegCloseKey(key);
    return exists;
}

void StoreWindowState(WindowState state, WindowStateUpdateFlags stateUpdateMask) {
	if(!RegKeyExist(HKEY_CURRENT_USER, "Software\\Lustrious Paint")){
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

    if(CheckBit(stateUpdateMask, WindowStateUpdateFlagBits::X)) {
        error = RegSetKeyValue(key, nullptr, "X", REG_DWORD, &state.x, 4);
        assert(error == ERROR_SUCCESS);
    }

    if(CheckBit(stateUpdateMask, WindowStateUpdateFlagBits::Y)) {
        error = RegSetKeyValue(key, nullptr, "Y", REG_DWORD, &state.y, 4);
        assert(error == ERROR_SUCCESS);
    }

    if(CheckBit(stateUpdateMask, WindowStateUpdateFlagBits::Width)) {
        error = RegSetKeyValue(key, nullptr, "Width", REG_DWORD, &state.width, 4);
        assert(error == ERROR_SUCCESS);
    }

    if(CheckBit(stateUpdateMask, WindowStateUpdateFlagBits::Height)) {
        error = RegSetKeyValue(key, nullptr, "Height", REG_DWORD, &state.height, 4);
        assert(error == ERROR_SUCCESS);
    }

    if(CheckBit(stateUpdateMask, WindowStateUpdateFlagBits::Mode)) {
        error = RegSetKeyValue(key, nullptr, "Mode", REG_DWORD, &state.mode, 4);
        assert(error == ERROR_SUCCESS);
    }

    error = RegCloseKey(key);
    assert(error == ERROR_SUCCESS);
}

bool LoadWindowState(WindowState* state) {
    if(!RegKeyExist(HKEY_CURRENT_USER, "Software\\Lustrious Paint")) {
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