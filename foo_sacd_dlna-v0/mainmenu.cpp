#include "stdafx.h"
#include "config.h"
#include "dlna_server.h"

namespace {
static const GUID guid_group = { 0x8b0c48f0, 0x2f77, 0x4f32, { 0x9b, 0x42, 0x7e, 0x0c, 0x61, 0x49, 0x8c, 0x22 } };
static const GUID guid_toggle = { 0x5a1a8d35, 0x5f5c, 0x4d6a, { 0x9a, 0x2b, 0x03, 0x87, 0x6e, 0x76, 0x84, 0x11 } };
static const GUID guid_share = { 0x3d8f32b4, 0xaec4, 0x4d6d, { 0x8f, 0x9c, 0x92, 0x55, 0x45, 0x39, 0x72, 0xe1 } };
static const GUID guid_preferences = { 0x62a9bf62, 0x6d0b, 0x4d2b, { 0xb3, 0x0e, 0x2a, 0xdc, 0xbb, 0x1d, 0x80, 0x72 } };
static const GUID guid_help = { 0x0c747d24, 0x8c3c, 0x45d1, { 0x81, 0x2d, 0x0a, 0x6c, 0x11, 0x49, 0xe7, 0xc2 } };
static const GUID guid_dsp = { 0x1aa39fe6, 0x7716, 0x4c6c, { 0x87, 0xa8, 0x9e, 0x2c, 0x39, 0x55, 0x62, 0x0d } };

static mainmenu_group_popup_factory g_group(guid_group, mainmenu_groups::tools, mainmenu_commands::sort_priority_dontcare, "SACD DLNA");

class commands : public mainmenu_commands {
public:
    enum { toggle, share, dsp, preferences, help, total };
    t_uint32 get_command_count() override { return total; }
    GUID get_command(t_uint32 i) override {
        switch (i) {
        case toggle: return guid_toggle; case share: return guid_share; case dsp: return guid_dsp; case preferences: return guid_preferences; case help: return guid_help; default: uBugCheck();
        }
    }
    void get_name(t_uint32 i, pfc::string_base& out) override {
        switch (i) {
        case toggle: out = "Enable DLNA broadcasting"; break;
        case share: out = "Share DSD Music Library"; break;
        case dsp: out = "Enable DLNA DSD Processor"; break;
        case preferences: out = "Open Preferences"; break;
        case help: out = "Help"; break;
        default: uBugCheck();
        }
    }
    bool get_description(t_uint32 i, pfc::string_base& out) override {
        switch (i) {
        case toggle: out = "Starts or stops the SACD DLNA Media Server."; return true;
        case share: out = "Publishes DSD-capable content from foobar2000's Media Library."; return true;
        case dsp: out = "Routes DLNA audio through the installed DSD Processor DSP."; return true;
        case preferences: out = "Open the dedicated SACD DLNA preferences page."; return true;
        case help: out = "Show SACD DLNA requirements and usage notes."; return true;
        default: return false;
        }
    }
    GUID get_parent() override { return guid_group; }
    void execute(t_uint32 i, service_ptr_t<service_base>) override {
        switch (i) {
        case toggle: {
            const bool on = !SacdDlnaServer::instance().is_running();
            if (on && !sacd_plugin_installed()) { popup_message::g_show("foo_input_sacd.dll (Super Audio CD Decoder) is required.", "SACD DLNA"); return; }
            sacd_dlna_cfg::enabled = on; SacdDlnaServer::instance().set_enabled(on);
            if (on && sacd_dlna_cfg::share_library) SacdDlnaServer::instance().share_music_library();
            break;
        }
        case dsp:
            if (!dsd_processor_installed()) { popup_message::g_show("foo_dsd_processor.dll is required.", "SACD DLNA"); return; }
            sacd_dlna_cfg::dsd_processor_enabled = !static_cast<bool>(sacd_dlna_cfg::dsd_processor_enabled);
            if (!sacd_dlna_cfg::dsd_processor_enabled) SacdDlnaServer::instance().share_music_library();
            else SacdDlnaServer::instance().share_music_library();
            break;
        case share:
            if (!sacd_plugin_installed()) { popup_message::g_show("foo_input_sacd.dll (Super Audio CD Decoder) is required.", "SACD DLNA"); return; }
            sacd_dlna_cfg::share_library = true;
            if (!SacdDlnaServer::instance().is_running()) { sacd_dlna_cfg::enabled = true; SacdDlnaServer::instance().set_enabled(true); }
            SacdDlnaServer::instance().share_music_library();
            break;
        case preferences: standard_commands::main_preferences(); break;
        case help: popup_message::g_show("Enable DLNA broadcasting, then share the DSD portion of foobar2000's Music Library. The server keeps DSD native and exposes DSF over UPnP/DLNA. foo_input_sacd.dll is required for SACD/DSD decoding.", "foo_sacd_dlna Help"); break;
        default: uBugCheck();
        }
    }
    bool get_display(t_uint32 i, pfc::string_base& text, t_uint32& flags) override {
        const bool rv = mainmenu_commands::get_display(i, text, flags);
        if (rv && i == toggle && SacdDlnaServer::instance().is_running()) flags |= flag_checked;
        if (rv && i == share && SacdDlnaServer::instance().get_status().sharingLibrary) flags |= flag_checked;
        if (rv && i == dsp && static_cast<bool>(sacd_dlna_cfg::dsd_processor_enabled)) flags |= flag_checked;
        return rv;
    }
};
static mainmenu_commands_factory_t<commands> g_commands;
}
