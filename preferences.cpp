#include "stdafx.h"
#include "config.h"
#include "dlna_server.h"
#include "tooltips.h"
#include "resource.h"
#include <helpers/atl-misc.h>
#include <helpers/DarkMode.h>

namespace {
class preferences_impl : public CDialogImpl<preferences_impl>, public preferences_page_instance {
public:
    enum { IDD = IDD_SACD_DLNA_PREFERENCES };
    preferences_impl(preferences_page_callback::ptr callback) : m_callback(callback) {}

    BEGIN_MSG_MAP_EX(preferences_impl)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_HANDLER_EX(IDC_ENABLE, BN_CLICKED, OnChangedCommand)
        COMMAND_HANDLER_EX(IDC_SHARE_LIBRARY, BN_CLICKED, OnChangedCommand)
        COMMAND_HANDLER_EX(IDC_SERVER_NAME, EN_CHANGE, OnChangedCommand)
        COMMAND_HANDLER_EX(IDC_PORT, EN_CHANGE, OnChangedCommand)
        COMMAND_HANDLER_EX(IDC_STABILITY_MODE, BN_CLICKED, OnChangedCommand)
        COMMAND_HANDLER_EX(IDC_PREBUFFER_SECONDS, EN_CHANGE, OnChangedCommand)
        COMMAND_HANDLER_EX(IDC_REFRESH_LIBRARY, BN_CLICKED, OnRefreshLibrary)
        COMMAND_HANDLER_EX(IDC_OPEN_LIBRARY, BN_CLICKED, OnOpenLibrary)
        COMMAND_HANDLER_EX(IDC_CLEAR_LIBRARY, BN_CLICKED, OnClearLibrary)
        COMMAND_HANDLER_EX(IDC_HELP, BN_CLICKED, OnHelp)
        MSG_WM_TIMER(OnTimer)
    END_MSG_MAP()

    t_uint32 get_state() override {
        t_uint32 s = preferences_state::resettable | preferences_state::dark_mode_supported;
        if (HasChanged()) s |= preferences_state::changed;
        return s;
    }

    void reset() override {
        CheckDlgButton(IDC_ENABLE, sacd_dlna_cfg::enabled ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(IDC_SHARE_LIBRARY, sacd_dlna_cfg::share_library ? BST_CHECKED : BST_UNCHECKED);
        SetDlgItemTextA(IDC_SERVER_NAME, sacd_dlna_cfg::server_name.get());
        SetDlgItemInt(IDC_PORT, static_cast<UINT>(sacd_dlna_cfg::port.get()), FALSE);
        CheckDlgButton(IDC_STABILITY_MODE, sacd_dlna_cfg::stability_mode ? BST_CHECKED : BST_UNCHECKED);
        SetDlgItemInt(IDC_PREBUFFER_SECONDS, static_cast<UINT>(sacd_dlna_cfg::prebuffer_seconds.get()), FALSE);
        OnChanged();
    }

    void apply() override {
        sacd_dlna_cfg::enabled = IsDlgButtonChecked(IDC_ENABLE) == BST_CHECKED;
        sacd_dlna_cfg::share_library = IsDlgButtonChecked(IDC_SHARE_LIBRARY) == BST_CHECKED;
        char name[256]{}; GetDlgItemTextA(IDC_SERVER_NAME, name, sizeof(name)); sacd_dlna_cfg::server_name = name;
        BOOL ok = FALSE; const UINT p = GetDlgItemInt(IDC_PORT, &ok, FALSE); if (ok) sacd_dlna_cfg::port = std::clamp<UINT>(p, 1024, 65535);
        sacd_dlna_cfg::stability_mode = IsDlgButtonChecked(IDC_STABILITY_MODE) == BST_CHECKED;
        BOOL bok = FALSE; const UINT b = GetDlgItemInt(IDC_PREBUFFER_SECONDS, &bok, FALSE); if (bok) sacd_dlna_cfg::prebuffer_seconds = std::clamp<UINT>(b, 5, 60);

        if (sacd_dlna_cfg::enabled) {
            if (!sacd_plugin_installed()) {
                sacd_dlna_cfg::enabled = false;
                CheckDlgButton(IDC_ENABLE, BST_UNCHECKED);
                popup_message::g_show("The Super Audio CD Decoder (foo_input_sacd.dll) is not installed. Install it before enabling SACD DLNA.", "SACD DLNA");
            } else {
                SacdDlnaServer::instance().set_enabled(true);
                if (sacd_dlna_cfg::share_library) SacdDlnaServer::instance().share_music_library();
            }
        } else {
            SacdDlnaServer::instance().set_enabled(false);
        }
        OnChanged();
    }

private:
    preferences_page_callback::ptr m_callback;
    fb2k::CDarkModeHooks m_dark;
    sacd_tooltips m_tips;

    BOOL OnInitDialog(CWindow, LPARAM) {
        m_dark.AddDialogWithControls(*this);
        m_tips.create(*this);
        m_tips.add(*this, IDC_ENABLE,
            "Enable the UPnP/DLNA server and SSDP discovery. BROADCASTING means the server is discoverable; it does not mean audio is currently being transmitted.");
        m_tips.add(*this, IDC_SHARE_LIBRARY,
            "Expose DSD-capable tracks from the foobar2000 Music Library. The DLNA tree is presented as Artist > Album > Track.");
        m_tips.add(*this, IDC_SERVER_NAME,
            "Friendly name shown to UPnP/DLNA players. Example: foobar2000 SACD DSD.");
        m_tips.add(*this, IDC_PORT,
            "TCP port used for HTTP media delivery and UPnP XML. Default: 8192. If changed, make sure the Windows firewall permits the new port.");
        m_tips.add(*this, IDC_STABILITY_MODE,
            "Separate SACD/DSD preparation from network delivery. Useful for short disk or network fluctuations. It cannot compensate for a link that is permanently too slow.");
        m_tips.add(*this, IDC_PREBUFFER_SECONDS,
            "Read-ahead target in seconds. Allowed range: 5-60 seconds. Default: 15. DSD256 uses about 2.82 MB/s of raw stereo DSD, so 15 seconds is about 42.3 MB.");
        m_tips.add(*this, IDC_STATUS_DLNA,
            "BROADCASTING / ACTIVE = the DLNA server and SSDP discovery are running.");
        m_tips.add(*this, IDC_STATUS_SACD,
            "Shows whether the required Super Audio CD Decoder (foo_input_sacd) is installed and, when available, its detected version.");
        m_tips.add(*this, IDC_STATUS_LIBRARY,
            "Shows whether the foobar2000 Music Library is being shared and how many DSD-capable items are currently exposed.");
        m_tips.add(*this, IDC_STATUS_STREAM,
            "ACTIVE / TRANSMITTING = a renderer is downloading audio over HTTP. TX is the measured server-to-renderer TCP rate.");
        m_tips.add(*this, IDC_STATUS_SDX,
            "Shows whether a T+A SDX renderer has been identified via UPnP/SSDP, including its IP and streaming state when available.");
        m_tips.add(*this, IDC_STATUS_BUFFER,
            "Shows SACD-to-DSD preparation, Stability Mode, read-ahead size and network headroom when available.");
        m_tips.add(*this, IDC_REFRESH_LIBRARY,
            "Re-scan the configured foobar2000 Music Library and rebuild the DSD items exposed through DLNA.");
        m_tips.add(*this, IDC_OPEN_LIBRARY,
            "Open foobar2000 Media Library preferences so you can add or remove music folders.");
        m_tips.add(*this, IDC_CLEAR_LIBRARY,
            "Stop exposing the current shared DSD library. It does not delete your music files.");
        m_tips.add(*this, IDC_HELP,
            "Open detailed help with setup examples, explanations of every field, network guidance and troubleshooting.");
        reset();
        SetTimer(1, 1000);
        UpdateStatus();
        return FALSE;
    }

    bool HasChanged() const {
        char name[256]{}; GetDlgItemTextA(IDC_SERVER_NAME, name, sizeof(name));
        BOOL ok = FALSE; const UINT p = GetDlgItemInt(IDC_PORT, &ok, FALSE);
        BOOL bok = FALSE; const UINT b = GetDlgItemInt(IDC_PREBUFFER_SECONDS, &bok, FALSE);
        return (IsDlgButtonChecked(IDC_ENABLE) == BST_CHECKED) != static_cast<bool>(sacd_dlna_cfg::enabled) ||
            (IsDlgButtonChecked(IDC_SHARE_LIBRARY) == BST_CHECKED) != static_cast<bool>(sacd_dlna_cfg::share_library) ||
            strcmp(name, sacd_dlna_cfg::server_name.get()) != 0 ||
            (ok && p != static_cast<UINT>(sacd_dlna_cfg::port.get())) ||
            (IsDlgButtonChecked(IDC_STABILITY_MODE) == BST_CHECKED) != static_cast<bool>(sacd_dlna_cfg::stability_mode) ||
            (bok && b != static_cast<UINT>(sacd_dlna_cfg::prebuffer_seconds.get()));
    }

    void OnTimer(UINT_PTR) { UpdateStatus(); }

    void OnChangedCommand(UINT, int, CWindow) { UpdateStatus(); OnChanged(); }

    void OnChanged() { m_callback->on_state_changed(); }

    void UpdateStatus() {
        const auto st = SacdDlnaServer::instance().get_status();
        std::string line1 = std::string("DLNA: ") + (st.broadcasting ? "BROADCASTING / ACTIVE" : "STOPPED");
        SetDlgItemTextA(IDC_STATUS_DLNA, line1.c_str());
        std::string line2 = std::string("foo_input_sacd: ") + (st.sacdInstalled ? "INSTALLED" : "NOT INSTALLED");
        if (st.sacdInstalled && !st.sacdVersion.is_empty()) { line2 += " ("; line2 += st.sacdVersion; line2 += ")"; }
        SetDlgItemTextA(IDC_STATUS_SACD, line2.c_str());
        std::string line3 = "Music Library: " + std::string(st.sharingLibrary ? "SHARING" : "NOT SHARING") + " (" + std::to_string(st.sharedCount) + " DSD tracks)";
        SetDlgItemTextA(IDC_STATUS_LIBRARY, line3.c_str());

        const double mbps = static_cast<double>(st.bytesPerSecond) * 8.0 / 1000000.0;
        std::string line4 = std::string("Audio stream: ") + (st.streamingActive ? "ACTIVE / TRANSMITTING" : "IDLE");
        if (st.streamingActive) line4 += "   TX " + std::to_string(mbps) + " Mbit/s";
        SetDlgItemTextA(IDC_STATUS_STREAM, line4.c_str());

        std::string line5 = "T+A SDX: " + std::string(st.sdxDetected ? (st.sdxStreaming ? "DETECTED / STREAMING" : "DETECTED / IDLE") : "NOT DETECTED");
        if (!st.sdxIp.is_empty()) line5 += "  " + std::string(st.sdxIp.c_str());
        if (!st.sdxName.is_empty()) line5 += "  " + std::string(st.sdxName.c_str());
        SetDlgItemTextA(IDC_STATUS_SDX, line5.c_str());
        std::string line6 = "DSD cache/buffer: ";
        if (st.conversionActive) line6 += "SACD→DSD CONVERTING " + std::to_string(st.conversionPercent) + "%";
        else if (st.stabilityMode && st.streamingActive && st.prebufferTargetBytes) line6 += "READ-AHEAD " + std::to_string(st.prebufferTargetBytes / 1048576.0) + " MB";
        else line6 += st.stabilityMode ? "READY" : "OFF";
        if (st.streamingActive && st.nominalBitrate) {
            const double req = static_cast<double>(st.nominalBitrate) / 1000000.0;
            line6 += " | required " + std::to_string(req) + " Mbit/s";
            if (st.networkHeadroom > 0) line6 += " | TX/required " + std::to_string(st.networkHeadroom) + "x";
        }
        SetDlgItemTextA(IDC_STATUS_BUFFER, line6.c_str());
    }

    void OnRefreshLibrary(UINT, int, CWindow) {
        if (!sacd_plugin_installed()) { popup_message::g_show("foo_input_sacd.dll is required before the DSD library can be shared.", "SACD DLNA"); return; }
        SacdDlnaServer::instance().share_music_library();
        CheckDlgButton(IDC_SHARE_LIBRARY, BST_CHECKED); UpdateStatus(); OnChanged();
    }

    void OnOpenLibrary(UINT, int, CWindow) {
        library_manager::get()->show_preferences();
    }

    void OnClearLibrary(UINT, int, CWindow) {
        SacdDlnaServer::instance().clear_shared_library();
        sacd_dlna_cfg::share_library = false;
        CheckDlgButton(IDC_SHARE_LIBRARY, BST_UNCHECKED);
        UpdateStatus(); OnChanged();
    }

    void OnHelp(UINT, int, CWindow) {
        const char* msg =
            "foo_sacd_dlna Help\n\n"
            "Purpose:\n"
            "Expose native DSD music from foobar2000 through UPnP/DLNA to compatible network players such as the T+A SDX 3100 HV.\n\n"
            "DSD policy:\n"
            "SACD ISO -> foo_input_sacd -> DSD -> DSF cache -> DLNA -> renderer. No DSD-to-PCM conversion is performed for network delivery.\n\n"
            "Key status meanings:\n"
            "BROADCASTING = server discovery is running.\n"
            "TRANSMITTING = a renderer is actively downloading audio bytes.\n"
            "T+A SDX = renderer identity/state when enough UPnP/SSDP information is available.\n"
            "TX = measured TCP transmit rate to the active client.\n\n"
            "Example 1 - normal Gigabit Ethernet:\n"
            "Enable DLNA, Share DSD Library, Stability Mode ON, Pre-buffer 15 s.\n\n"
            "Example 2 - busy network:\n"
            "Use wired Gigabit Ethernet where possible and increase Pre-buffer to 20-30 s. This absorbs short fluctuations but cannot fix a sustained throughput deficit.\n\n"
            "Example 3 - SACD ISO:\n"
            "The installed foo_input_sacd decoder is required. The ISO is decoded as DSD and prepared as DSF for network delivery; the ISO itself is not modified.\n\n"
            "Why not PCM?\n"
            "The project is designed for a native-DSD signal path. Converting to PCM would change that signal path and is therefore intentionally outside the network mode.\n\n"
            "For complete field-by-field explanations and troubleshooting, see HELP.md and EXAMPLES.md in the project repository.";
        popup_message::g_show(msg, "foo_sacd_dlna Help");
    }
};

class preferences_page_impl_sacd : public preferences_page_impl<preferences_impl> {
public:
    const char* get_name() { return "SACD DLNA"; }
    GUID get_guid() { return GUID{ 0x5fbb3c34, 0x8f75, 0x4e2d, { 0xb6, 0x7f, 0x1d, 0x37, 0x78, 0x8c, 0x0e, 0x29 } }; }
    GUID get_parent_guid() { return guid_tools; }
};
static preferences_page_factory_t<preferences_page_impl_sacd> g_preferences_factory;
}
