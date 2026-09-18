#pragma once

#include "stdafx.h"
#include <SDK/cfg_var.h>

namespace sacd_dlna_cfg {
    extern const GUID guid_cfg_enabled;
    extern const GUID guid_cfg_share_library;
    extern const GUID guid_cfg_port;
    extern const GUID guid_cfg_server_name;
    extern const GUID guid_cfg_stability_mode;
    extern const GUID guid_cfg_prebuffer_seconds;

    extern cfg_bool enabled;
    extern cfg_bool share_library;
    extern cfg_uint port;
    extern cfg_string server_name;
    extern cfg_bool stability_mode;
    extern cfg_uint prebuffer_seconds;
}

bool sacd_plugin_installed(pfc::string_base* versionOut = nullptr);
const char* sacd_plugin_required_name();
