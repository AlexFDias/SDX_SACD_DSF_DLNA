#include "stdafx.h"
#include "config.h"
#include <SDK/componentversion.h>

namespace sacd_dlna_cfg {
    const GUID guid_cfg_enabled = { 0x3c2e89aa, 0x76df, 0x4c3a, { 0x9a, 0x18, 0x7d, 0x6a, 0x34, 0x75, 0x2b, 0x10 } };
    const GUID guid_cfg_share_library = { 0x4bcad0b8, 0xe0bb, 0x43b4, { 0x8d, 0x8f, 0x7a, 0x38, 0x6d, 0xc0, 0x19, 0xf8 } };
    const GUID guid_cfg_port = { 0x515d5aa5, 0x6b42, 0x47f1, { 0xa2, 0x2f, 0x78, 0x0b, 0xf7, 0x0d, 0x34, 0x2a } };
    const GUID guid_cfg_server_name = { 0x0f0a5dc7, 0xc2c8, 0x4cf6, { 0x93, 0x6f, 0x1a, 0x5a, 0xef, 0xe8, 0x56, 0x01 } };
    const GUID guid_cfg_stability_mode = { 0x1a0ef9f2, 0x9d9f, 0x45ec, { 0x83, 0x5f, 0xb9, 0x5f, 0xa9, 0x2d, 0x4d, 0x17 } };
    const GUID guid_cfg_prebuffer_seconds = { 0x25b7c5fd, 0x6d6f, 0x4a43, { 0x91, 0x0f, 0xc1, 0xa5, 0x77, 0x58, 0x3c, 0x22 } };

    cfg_bool enabled(guid_cfg_enabled, false);
    cfg_bool share_library(guid_cfg_share_library, false);
    cfg_uint port(guid_cfg_port, 8192);
    cfg_string server_name(guid_cfg_server_name, "foobar2000 SACD DSD");
    cfg_bool stability_mode(guid_cfg_stability_mode, true);
    cfg_uint prebuffer_seconds(guid_cfg_prebuffer_seconds, 15);
}

const char* sacd_plugin_required_name() {
    return "foo_input_sacd.dll (Super Audio CD Decoder)";
}

bool sacd_plugin_installed(pfc::string_base* versionOut) {
    service_enum_t<componentversion> e;
    componentversion::ptr ptr;
    while (e.next(ptr)) {
        pfc::string8 fileName, componentName, version;
        ptr->get_file_name(fileName);
        ptr->get_component_name(componentName);
        ptr->get_component_version(version);

        const bool fileMatch = !_stricmp(fileName, "foo_input_sacd.dll");
        const bool nameMatch = !_stricmp(componentName, "Super Audio CD Decoder") ||
            (strstr(componentName.c_str(), "SACD") != nullptr && strstr(componentName.c_str(), "Decoder") != nullptr);

        if (fileMatch || nameMatch) {
            if (versionOut) *versionOut = version;
            return true;
        }
    }
    if (versionOut) versionOut->reset();
    return false;
}
