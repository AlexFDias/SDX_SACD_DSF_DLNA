#include "stdafx.h"
#include "dlna_server.h"
#include "config.h"
#include <libPPUI/win32_op.h>

namespace {
static const GUID guid_sacd_dlna_element = { 0x8c0fe7e9, 0x5f8f, 0x4d33, { 0x9d, 0x9c, 0x2b, 0x64, 0xa8, 0x3d, 0x20, 0x11 } };

class CSacdDlnaWindow : public ui_element_instance, public CWindowImpl<CSacdDlnaWindow> {
public:
    DECLARE_WND_CLASS_EX(TEXT("{5A6C02E7-8D2E-4D5D-8FB7-5F1BD2E11F01}"), CS_VREDRAW | CS_HREDRAW, -1);

    CSacdDlnaWindow(ui_element_config::ptr config, ui_element_instance_callback_ptr cb) : m_config(config), m_callback(cb) {}
    ~CSacdDlnaWindow() { KillTimer(1); }
    void initialize_window(HWND parent) { WIN32_OP(Create(parent) != NULL); SetTimer(1, 1000); }
    HWND get_wnd() { return *this; }
    void set_configuration(ui_element_config::ptr config) { m_config = config; }
    ui_element_config::ptr get_configuration() { return m_config; }
    static GUID g_get_guid() { return guid_sacd_dlna_element; }
    static GUID g_get_subclass() { return ui_element_subclass_utility; }
    static void g_get_name(pfc::string_base& out) { out = "SACD DLNA Status"; }
    static ui_element_config::ptr g_get_default_configuration() { return ui_element_config::g_create_empty(g_get_guid()); }
    static const char* g_get_description() { return "Shows SACD DLNA broadcasting status, decoder availability and Music Library sharing."; }

    BEGIN_MSG_MAP_EX(CSacdDlnaWindow)
        MSG_WM_PAINT(OnPaint)
        MSG_WM_ERASEBKGND(OnEraseBkgnd)
        MSG_WM_LBUTTONUP(OnClick)
        MSG_WM_LBUTTONDBLCLK(OnDoubleClick)
        MSG_WM_TIMER(OnTimer)
    END_MSG_MAP()

    void notify(const GUID& what, t_size, const void*, t_size) {
        if (what == ui_element_notify_colors_changed || what == ui_element_notify_font_changed) Invalidate();
    }

private:
    ui_element_config::ptr m_config;
    const ui_element_instance_callback_ptr m_callback;

    BOOL OnEraseBkgnd(CDCHandle dc) {
        CRect rc; WIN32_OP_D(GetClientRect(&rc));
        CBrush brush; WIN32_OP_D(brush.CreateSolidBrush(m_callback->query_std_color(ui_color_background)) != NULL);
        dc.FillRect(&rc, brush); return TRUE;
    }

    void OnTimer(UINT_PTR) { Invalidate(); }

    void drawLine(CDCHandle dc, int y, const char* text) {
        CRect rc; GetClientRect(&rc); rc.left += 10; rc.right -= 10; rc.top = y; rc.bottom = y + 18;
        dc.SetTextColor(m_callback->query_std_color(ui_color_text)); dc.SetBkMode(TRANSPARENT);
        SelectObjectScope fs(dc, (HGDIOBJ)m_callback->query_font_ex(ui_font_default));
        dc.DrawTextA(text, -1, &rc, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
    }

    void OnPaint(CDCHandle) {
        CPaintDC dc(*this);
        const auto st = SacdDlnaServer::instance().get_status();
        drawLine(dc, 8, st.broadcasting ? "DLNA discovery:  BROADCASTING / ACTIVE" : "DLNA discovery:  STOPPED");
        std::string sacd = std::string("foo_input_sacd:  ") + (st.sacdInstalled ? "INSTALLED" : "NOT INSTALLED");
        if (st.sacdInstalled && !st.sacdVersion.is_empty()) sacd += "  " + st.sacdVersion;
        drawLine(dc, 28, sacd.c_str());
        const std::string lib = std::string("Music Library:  ") + (st.sharingLibrary ? "SHARING" : "NOT SHARING") + "  (" + std::to_string(st.sharedCount) + " DSD tracks)";
        drawLine(dc, 48, lib.c_str());
        const std::string server = "Server: " + std::string(st.serverName.c_str()) + "   HTTP port: " + std::to_string(st.port);
        drawLine(dc, 68, server.c_str());

        std::string stream = std::string("Audio stream:  ") + (st.streamingActive ? "ACTIVE / TRANSMITTING" : "IDLE");
        if (st.streamingActive) {
            const double mbps = static_cast<double>(st.bytesPerSecond) * 8.0 / 1000000.0;
            stream += "   TX " + std::to_string(mbps) + " Mbit/s";
            if (st.dsdRate) stream += "   DSD " + std::to_string(st.dsdRate / 2822400) + "x";
        }
        drawLine(dc, 88, stream.c_str());

        std::string client = "DLNA client:  " + std::string(st.clientIp.is_empty() ? "-" : st.clientIp.c_str());
        if (!st.clientName.is_empty()) client += "  " + std::string(st.clientName.c_str());
        drawLine(dc, 108, client.c_str());

        std::string sdx = "T+A SDX:  " + std::string(st.sdxDetected ? (st.sdxStreaming ? "DETECTED / STREAMING" : "DETECTED / IDLE") : "NOT DETECTED");
        if (!st.sdxIp.is_empty()) sdx += "  " + std::string(st.sdxIp.c_str());
        if (!st.sdxModel.is_empty()) sdx += "  " + std::string(st.sdxModel.c_str());
        drawLine(dc, 128, sdx.c_str());

        std::string buffer = "Stability:  " + std::string(st.stabilityMode ? "ON" : "OFF");
        if (st.conversionActive) buffer += "   SACD→DSD " + std::to_string(st.conversionPercent) + "%";
        else if (st.streamingActive && st.prebufferTargetBytes) buffer += "   read-ahead " + std::to_string(st.prebufferTargetBytes / 1048576.0) + " MB";
        drawLine(dc, 148, buffer.c_str());

        std::string title = "Track:  " + std::string(st.streamTitle.is_empty() ? "-" : st.streamTitle.c_str());
        drawLine(dc, 168, title.c_str());
        drawLine(dc, 188, "Click: toggle DLNA   |   Double-click: open SACD DLNA Preferences");
    }

    void OnClick(UINT, CPoint) {
        const bool now = !SacdDlnaServer::instance().is_running();
        if (now && !sacd_plugin_installed()) {
            popup_message::g_show("foo_input_sacd.dll (Super Audio CD Decoder) is required.", "SACD DLNA");
            return;
        }
        sacd_dlna_cfg::enabled = now;
        SacdDlnaServer::instance().set_enabled(now);
        if (now && sacd_dlna_cfg::share_library) SacdDlnaServer::instance().share_music_library();
        Invalidate();
    }

    void OnDoubleClick(UINT, CPoint) { standard_commands::main_preferences(); }
};

class ui_element_sacd_impl : public ui_element_impl_withpopup<CSacdDlnaWindow> {};
static service_factory_single_t<ui_element_sacd_impl> g_ui_element_factory;
}
