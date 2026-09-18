#include "stdafx.h"
#include "dlna_server.h"
#include "sacd_decode.h"
#include "config.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <shellapi.h>
#include <sstream>

namespace fs = std::filesystem;

namespace {

constexpr const char* kUuid = "uuid:7e2f2d7e-7d89-4d3c-9f3e-3a7c09fd1234";
constexpr const char* kRootObject = "0";
constexpr const char* kArtistPrefix = "artist-";
constexpr const char* kAlbumPrefix = "album-";
constexpr const char* kTrackPrefix = "track-";

void sendAll(SOCKET s, const char* p, size_t n) {
    while (n) {
        const int chunk = static_cast<int>(std::min<size_t>(n, 1u << 20));
        const int r = send(s, p, chunk, 0);
        if (r <= 0) throw std::runtime_error("socket send failed");
        p += r;
        n -= static_cast<size_t>(r);
    }
}

std::wstring persistentCacheFolder() {
    std::wstring p = pfc::stringcvt::string_wide_from_utf8(core_api::get_profile_path());
    if (!p.empty() && p.back() != L'\\' && p.back() != L'/') p += L'\\';
    p += L"foo_sacd_dlna\\cache";
    CreateDirectoryW((p.substr(0, p.rfind(L"\\"))).c_str(), nullptr);
    CreateDirectoryW(p.c_str(), nullptr);
    return p;
}

uint64_t fnv1a64(const std::string& s) {
    uint64_t h = 14695981039346656037ull;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ull; }
    return h;
}

std::string cacheKeyFor(const std::string& sourcePath, t_uint32 subsong) {
    std::error_code ec;
    const auto ws = pfc::stringcvt::string_wide_from_utf8(sourcePath.c_str());
    const uintmax_t size = fs::file_size(ws, ec);
    std::error_code ec2;
    const auto wt = fs::last_write_time(ws, ec2);
    const auto ticks = ec2 ? 0LL : wt.time_since_epoch().count();
    const std::string material = sourcePath + "#" + std::to_string(subsong) + "#" + std::to_string(size) + "#" + std::to_string(ticks);
    char hex[32]{}; snprintf(hex, sizeof(hex), "%016llX", static_cast<unsigned long long>(fnv1a64(material)));
    return hex;
}

bool getHeaderValue(const std::string& headers, const char* name, std::string& out) {
    const auto p = headers.find(name);
    if (p == std::string::npos) return false;
    const auto e = headers.find("\r\n", p);
    const auto start = p + strlen(name);
    out = headers.substr(start, e == std::string::npos ? std::string::npos : e - start);
    while (!out.empty() && (out.front() == ' ' || out.front() == '\t')) out.erase(out.begin());
    return true;
}

bool parseRange(const std::string& headers, uint64_t size, uint64_t& begin, uint64_t& end, bool& partial) {
    std::string value;
    if (!getHeaderValue(headers, "Range:", value)) return false;
    if (value.rfind("bytes=", 0) != 0 || size == 0) return false;

    const auto dash = value.find('-', 6);
    if (dash == std::string::npos) return false;
    try {
        if (dash == 6) {
            const uint64_t suffix = std::stoull(value.substr(7));
            begin = suffix >= size ? 0 : size - suffix;
            end = size - 1;
        } else {
            begin = std::stoull(value.substr(6, dash - 6));
            if (dash + 1 < value.size()) end = std::stoull(value.substr(dash + 1));
            else end = size - 1;
        }
    } catch (...) {
        return false;
    }
    if (begin >= size || begin > end) return false;
    end = std::min(end, size - 1);
    partial = true;
    return true;
}

std::string xmlAttr(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 16);
    for (char c : value) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        case '\'': out += "&apos;"; break;
        default: out += c; break;
        }
    }
    return out;
}


}

SacdDlnaServer& SacdDlnaServer::instance() {
    static SacdDlnaServer x;
    return x;
}

void SacdDlnaServer::set_enabled(bool enabled) {
    if (enabled) start();
    else stop();
}

void SacdDlnaServer::start() {
    bool expected = false;
    if (!m_running.compare_exchange_strong(expected, true)) return;

    if (!sacd_plugin_installed()) {
        m_running = false;
        console::print("SACD DLNA: Super Audio CD Decoder (foo_input_sacd.dll) is not installed");
        return;
    }

    m_port = static_cast<uint16_t>(std::clamp<uint32_t>(sacd_dlna_cfg::port.get(), 1024, 65535));

    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        m_running = false;
        throw std::runtime_error("WSAStartup failed");
    }

    m_httpThread = std::thread([this] { httpLoop(); });
    m_ssdpThread = std::thread([this] { ssdpLoop(); });
    console::print("SACD DLNA: BROADCASTING / ACTIVE");
}

void SacdDlnaServer::stop() {
    if (!m_running.exchange(false)) return;

    if (m_httpListen != INVALID_SOCKET) {
        closesocket(m_httpListen);
        m_httpListen = INVALID_SOCKET;
    }
    if (m_httpThread.joinable()) m_httpThread.join();
    if (m_ssdpThread.joinable()) m_ssdpThread.join();
    WSACleanup();
    console::print("SACD DLNA: broadcasting stopped");
}

size_t SacdDlnaServer::shared_count() const {
    return m_sharedCount.load();
}

SacdDlnaStatus SacdDlnaServer::get_status() const {
    SacdDlnaStatus s;
    s.broadcasting = is_running();
    s.sacdInstalled = sacd_plugin_installed(&s.sacdVersion);
    s.sharingLibrary = m_sharingLibrary.load();
    s.sharedCount = m_sharedCount.load();
    s.port = m_port;

    {
        std::lock_guard<std::mutex> g(m_rateMutex);
        const auto now = std::chrono::steady_clock::now();
        if (m_lastRateTick.time_since_epoch().count() == 0) {
            m_lastRateTick = now;
        } else {
            const double seconds = std::chrono::duration<double>(now - m_lastRateTick).count();
            if (seconds >= 0.25) {
                const uint64_t delta = m_rateBytes - m_lastRateBytes;
                m_currentBps = static_cast<uint64_t>(static_cast<double>(delta) / seconds);
                m_lastRateBytes = m_rateBytes;
                m_lastRateTick = now;
            }
        }
        s.activeStreams = m_activeStreams;
        s.streamingActive = m_activeStreams != 0;
        s.bytesPerSecond = m_currentBps;
        s.totalBytesSent = m_totalBytes;
        s.streamBytesSent = m_streamBytes;
        s.dsdRate = m_streamDsdRate;
        s.nominalBitrate = m_streamDsdRate ? static_cast<uint64_t>(m_streamDsdRate) * 2ULL : 0ULL;
        s.requiredBytesPerSecond = m_streamDsdRate ? static_cast<uint64_t>(m_streamDsdRate) / 4ULL : 0ULL;
        s.clientIp = m_clientIp.c_str();
        s.clientName = m_clientName.c_str();
        s.clientModel = m_clientModel.c_str();
        s.streamTitle = m_streamTitle.c_str();
        s.sdxIp = m_sdxIp.c_str();
        s.sdxName = m_sdxName.c_str();
        s.sdxModel = m_sdxModel.c_str();
        s.sdxDetected = !m_sdxIp.empty();
        s.sdxStreaming = s.streamingActive && !m_sdxIp.empty() && !m_clientIp.empty() && m_clientIp == m_sdxIp;
    }

    s.stabilityMode = sacd_dlna_cfg::stability_mode;
    s.serverName = sacd_dlna_cfg::server_name;
    {
        std::lock_guard<std::mutex> g(m_rateMutex);
        s.conversionActive = m_conversionActive;
        s.conversionPercent = m_conversionPercent;
        s.prebufferBytes = m_prebufferBytes;
        s.prebufferTargetBytes = m_prebufferTargetBytes;
        if (s.nominalBitrate && s.bytesPerSecond) s.networkHeadroom = static_cast<double>(s.bytesPerSecond) / static_cast<double>(s.nominalBitrate);
    }
    return s;
}

void SacdDlnaServer::updateStreamStart(const std::string& peerIp, const std::string& title, uint32_t dsdRate) {
    std::lock_guard<std::mutex> g(m_rateMutex);
    ++m_activeStreams;
    m_clientIp = peerIp;
    m_streamTitle = title;
    m_streamDsdRate = dsdRate;
    m_streamBytes = 0;
    m_rateBytes = 0;
    m_lastRateBytes = 0;
    m_currentBps = 0;
    m_lastRateTick = std::chrono::steady_clock::now();
}

void SacdDlnaServer::updateStreamBytes(uint64_t bytes) {
    std::lock_guard<std::mutex> g(m_rateMutex);
    m_rateBytes += bytes;
    m_totalBytes += bytes;
    m_streamBytes += bytes;
}

void SacdDlnaServer::updateStreamEnd() {
    std::lock_guard<std::mutex> g(m_rateMutex);
    if (m_activeStreams) --m_activeStreams;
    if (m_activeStreams == 0) {
        m_currentBps = 0;
        m_streamDsdRate = 0;
        m_streamTitle.clear();
        m_prebufferBytes = 0;
        m_prebufferTargetBytes = 0;
    }
}

namespace {
std::string headerValueCI(const std::string& response, const char* header) {
    std::string lower = response;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::string target = header;
    std::transform(target.begin(), target.end(), target.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    target += ':';
    const auto p = lower.find(target);
    if (p == std::string::npos) return {};
    const auto start = p + target.size();
    const auto end = lower.find("\r\n", start);
    std::string value = response.substr(start, end == std::string::npos ? std::string::npos : end - start);
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.erase(value.begin());
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) value.pop_back();
    return value;
}

std::string xmlTagTextCI(const std::string& xml, const char* tag) {
    std::string lower = xml;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::string open = std::string("<") + tag + ">";
    std::string close = std::string("</") + tag + ">";
    std::transform(open.begin(), open.end(), open.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::transform(close.begin(), close.end(), close.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    const auto a = lower.find(open);
    if (a == std::string::npos) return {};
    const auto b = lower.find(close, a + open.size());
    if (b == std::string::npos) return {};
    return xml.substr(a + open.size(), b - (a + open.size()));
}

} // namespace

void SacdDlnaServer::probeSdxDescription(const std::string& location, const std::string& peerIp) {
    if (location.empty()) return;
    std::string url = location;
    const auto scheme = url.find("://");
    if (scheme == std::string::npos) return;
    const auto hostStart = scheme + 3;
    const auto slash = url.find('/', hostStart);
    const std::string authority = url.substr(hostStart, slash == std::string::npos ? std::string::npos : slash - hostStart);
    const std::string path = slash == std::string::npos ? "/" : url.substr(slash);

    std::string host = authority;
    uint16_t port = 80;
    const auto colon = authority.rfind(':');
    if (colon != std::string::npos && authority.find(']') == std::string::npos) {
        host = authority.substr(0, colon);
        try { port = static_cast<uint16_t>(std::stoul(authority.substr(colon + 1))); } catch (...) { port = 80; }
    }

    SOCKET c = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (c == INVALID_SOCKET) return;
    DWORD timeout = 1200;
    setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    setsockopt(c, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        addrinfo hints{}; hints.ai_family = AF_INET;
        addrinfo* res = nullptr;
        if (getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0 || !res) { closesocket(c); return; }
        addr.sin_addr = reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr;
        freeaddrinfo(res);
    }
    if (connect(c, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) { closesocket(c); return; }

    const std::string req = "GET " + path + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
    send(c, req.data(), static_cast<int>(req.size()), 0);
    std::string xml;
    char buf[8192];
    for (;;) {
        const int n = recv(c, buf, sizeof(buf), 0);
        if (n <= 0) break;
        xml.append(buf, buf + n);
        if (xml.size() > 1024 * 1024) break;
    }
    closesocket(c);

    const auto friendly = xmlTagTextCI(xml, "friendlyName");
    const auto model = xmlTagTextCI(xml, "modelName");
    const auto manufacturer = xmlTagTextCI(xml, "manufacturer");
    const auto hay = friendly + " " + model + " " + manufacturer + " " + xml;
    std::string lower = hay;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (lower.find("t+a") == std::string::npos && lower.find("sdx") == std::string::npos && lower.find("sd 3100 hv") == std::string::npos) return;

    std::lock_guard<std::mutex> g(m_rateMutex);
    m_sdxIp = peerIp;
    m_sdxName = friendly.empty() ? "T+A renderer" : friendly;
    m_sdxModel = model;
    m_sdxManufacturer = manufacturer;
}

void SacdDlnaServer::discoverSdxResponse(const std::string& response, const std::string& peerIp) {
    const auto location = headerValueCI(response, "LOCATION");
    const auto server = headerValueCI(response, "SERVER");
    const auto usn = headerValueCI(response, "USN");
    std::string hay = server + " " + usn;
    std::transform(hay.begin(), hay.end(), hay.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    std::string loc = location;
    std::transform(loc.begin(), loc.end(), loc.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (hay.find("t+a") != std::string::npos || hay.find("sdx") != std::string::npos || hay.find("sd 3100") != std::string::npos || loc.find("t+a") != std::string::npos || loc.find("sdx") != std::string::npos) {
        probeSdxDescription(location, peerIp);
    }
}

std::string SacdDlnaServer::xmlEscape(const std::string& s) {
    return xmlAttr(s);
}

std::string SacdDlnaServer::urlPathDecode(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            const auto hex = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return 10 + c - 'a';
                if (c >= 'A' && c <= 'F') return 10 + c - 'A';
                return -1;
            };
            const int a = hex(s[i + 1]), b = hex(s[i + 2]);
            if (a >= 0 && b >= 0) {
                out.push_back(static_cast<char>((a << 4) | b));
                i += 2;
                continue;
            }
        }
        out.push_back(s[i]);
    }
    return out;
}

std::string SacdDlnaServer::normalizeKey(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) out.push_back(static_cast<char>(std::tolower(c)));
    return out;
}

std::string SacdDlnaServer::mimeForExtension(const std::string& ext) {
    if (!_stricmp(ext.c_str(), ".dsf")) return "audio/x-dsf";
    if (!_stricmp(ext.c_str(), ".dff")) return "audio/x-dff";
    return "audio/x-dsf";
}

std::string SacdDlnaServer::detectImageMime(const void* data, size_t size) {
    if (!data || size < 4) return {};
    const auto* p = static_cast<const uint8_t*>(data);
    if (size >= 8 && p[0] == 0x89 && p[1] == 'P' && p[2] == 'N' && p[3] == 'G') return "image/png";
    if (p[0] == 0xFF && p[1] == 0xD8 && p[2] == 0xFF) return "image/jpeg";
    if (size >= 12 && !memcmp(p, "RIFF", 4) && !memcmp(p + 8, "WEBP", 4)) return "image/webp";
    return "application/octet-stream";
}

std::string SacdDlnaServer::didlProtocolInfo(const std::string& mime) {
    // Keep DLNA flags conservative; the endpoint is still HTTP range capable.
    return "http-get:*:" + mime + ":DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01700000000000000000000000000000";
}

std::wstring SacdDlnaServer::cacheFolder() const { return persistentCacheFolder(); }

std::string SacdDlnaServer::localAddress() const {
    char host[256]{};
    if (gethostname(host, sizeof(host)) != 0) return "127.0.0.1";
    addrinfo hints{};
    hints.ai_family = AF_INET;
    addrinfo* res = nullptr;
    if (getaddrinfo(host, nullptr, &hints, &res) != 0 || !res) return "127.0.0.1";

    char ip[INET_ADDRSTRLEN]{};
    const auto* sin = reinterpret_cast<const sockaddr_in*>(res->ai_addr);
    inet_ntop(AF_INET, &sin->sin_addr, ip, sizeof(ip));
    freeaddrinfo(res);
    return ip;
}

std::string SacdDlnaServer::makeDeviceXml() const {
    const std::string name = xmlEscape(sacd_dlna_cfg::server_name.get());
    const std::string base = "http://" + localAddress() + ":" + std::to_string(m_port);
    return
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
        "<specVersion><major>1</major><minor>0</minor></specVersion>"
        "<device>"
        "<deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>"
        "<friendlyName>" + name + "</friendlyName>"
        "<manufacturer>foo_sacd_dlna</manufacturer>"
        "<manufacturerURL>https://www.foobar2000.org/</manufacturerURL>"
        "<modelName>foobar2000 SACD DLNA</modelName>"
        "<modelDescription>Native DSD UPnP Media Server</modelDescription>"
        "<modelNumber>0.5-alpha1</modelNumber>"
        "<serialNumber>foo-sacd-dlna</serialNumber>"
        "<UDN>" + kUuid + "</UDN>"
        "<presentationURL>" + base + "/status</presentationURL>"
        "<serviceList>"
        "<service><serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType>"
        "<serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId>"
        "<SCPDURL>/ContentDirectory.xml</SCPDURL><controlURL>/ctl/ContentDirectory</controlURL><eventSubURL>/evt/ContentDirectory</eventSubURL></service>"
        "<service><serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</serviceType>"
        "<serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId>"
        "<SCPDURL>/ConnectionManager.xml</SCPDURL><controlURL>/ctl/ConnectionManager</controlURL><eventSubURL>/evt/ConnectionManager</eventSubURL></service>"
        "</serviceList>"
        "</device></root>";
}

std::string SacdDlnaServer::makeContentDirectoryScpd() const {
    return
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\"><specVersion><major>1</major><minor>0</minor></specVersion>"
        "<actionList><action><name>Browse</name><argumentList>"
        "<argument><name>ObjectID</name><direction>in</direction><relatedStateVariable>A_ARG_TYPE_ObjectID</relatedStateVariable></argument>"
        "<argument><name>BrowseFlag</name><direction>in</direction><relatedStateVariable>A_ARG_TYPE_BrowseFlag</relatedStateVariable></argument>"
        "<argument><name>Filter</name><direction>in</direction><relatedStateVariable>A_ARG_TYPE_Filter</relatedStateVariable></argument>"
        "<argument><name>StartingIndex</name><direction>in</direction><relatedStateVariable>A_ARG_TYPE_Index</relatedStateVariable></argument>"
        "<argument><name>RequestedCount</name><direction>in</direction><relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable></argument>"
        "<argument><name>SortCriteria</name><direction>in</direction><relatedStateVariable>A_ARG_TYPE_SortCriteria</relatedStateVariable></argument>"
        "<argument><name>Result</name><direction>out</direction><relatedStateVariable>A_ARG_TYPE_Result</relatedStateVariable></argument>"
        "<argument><name>NumberReturned</name><direction>out</direction><relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable></argument>"
        "<argument><name>TotalMatches</name><direction>out</direction><relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable></argument>"
        "<argument><name>UpdateID</name><direction>out</direction><relatedStateVariable>A_ARG_TYPE_UpdateID</relatedStateVariable></argument>"
        "</argumentList></action></actionList><serviceStateTable>"
        "<stateVariable sendEvents=\"no\"><name>A_ARG_TYPE_ObjectID</name><dataType>string</dataType></stateVariable>"
        "<stateVariable sendEvents=\"no\"><name>A_ARG_TYPE_BrowseFlag</name><dataType>string</dataType></stateVariable>"
        "<stateVariable sendEvents=\"no\"><name>A_ARG_TYPE_Filter</name><dataType>string</dataType></stateVariable>"
        "<stateVariable sendEvents=\"no\"><name>A_ARG_TYPE_Index</name><dataType>ui4</dataType></stateVariable>"
        "<stateVariable sendEvents=\"no\"><name>A_ARG_TYPE_Count</name><dataType>ui4</dataType></stateVariable>"
        "<stateVariable sendEvents=\"no\"><name>A_ARG_TYPE_SortCriteria</name><dataType>string</dataType></stateVariable>"
        "<stateVariable sendEvents=\"no\"><name>A_ARG_TYPE_Result</name><dataType>string</dataType></stateVariable>"
        "<stateVariable sendEvents=\"no\"><name>A_ARG_TYPE_UpdateID</name><dataType>ui4</dataType></stateVariable>"
        "</serviceStateTable></scpd>";
}

std::string SacdDlnaServer::makeConnectionManagerScpd() const {
    return
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\"><specVersion><major>1</major><minor>0</minor></specVersion>"
        "<actionList><action><name>GetProtocolInfo</name><argumentList>"
        "<argument><name>Source</name><direction>out</direction><relatedStateVariable>SourceProtocolInfo</relatedStateVariable></argument>"
        "<argument><name>Sink</name><direction>out</direction><relatedStateVariable>SinkProtocolInfo</relatedStateVariable></argument>"
        "</argumentList></action></actionList><serviceStateTable>"
        "<stateVariable sendEvents=\"no\"><name>SourceProtocolInfo</name><dataType>string</dataType></stateVariable>"
        "<stateVariable sendEvents=\"no\"><name>SinkProtocolInfo</name><dataType>string</dataType></stateVariable>"
        "</serviceStateTable></scpd>";
}

std::string SacdDlnaServer::browseDidl(const std::string& objectId, unsigned& numberReturned, unsigned& totalMatches) const {
    std::vector<Item> items;
    std::vector<Artist> artists;
    std::vector<Album> albums;
    {
        std::lock_guard<std::mutex> g(m_mutex);
        items = m_items;
        artists = m_artists;
        albums = m_albums;
    }

    numberReturned = totalMatches = 0;
    const std::string ns =
        " xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
        " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
        " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\""
        " xmlns:dlna=\"urn:schemas-dlna-org:metadata-1-0/\"";
    std::string out = "<DIDL-Lite" + ns + ">";
    const std::string base = "http://" + localAddress() + ":" + std::to_string(m_port);

    auto findArtist = [&](uint32_t id) -> const Artist* {
        for (const auto& x : artists) if (x.id == id) return &x;
        return nullptr;
    };
    auto findAlbum = [&](uint32_t id) -> const Album* {
        for (const auto& x : albums) if (x.id == id) return &x;
        return nullptr;
    };
    auto findItem = [&](uint32_t id) -> const Item* {
        for (const auto& x : items) if (x.id == id) return &x;
        return nullptr;
    };

    if (objectId == kRootObject) {
        out += "<container id=\"artists\" parentID=\"0\" restricted=\"1\" childCount=\"" + std::to_string(artists.size()) + "\">";
        out += "<dc:title>Artists</dc:title><upnp:class>object.container.person.musicArtist</upnp:class></container>";
        numberReturned = totalMatches = 1;
    } else if (objectId == "artists") {
        for (const auto& artist : artists) {
            out += "<container id=\"" + std::string(kArtistPrefix) + std::to_string(artist.id) + "\" parentID=\"artists\" restricted=\"1\" childCount=\"" + std::to_string(artist.albumIds.size()) + "\">";
            out += "<dc:title>" + xmlEscape(artist.name) + "</dc:title><upnp:class>object.container.person.musicArtist</upnp:class></container>";
            ++numberReturned;
        }
        totalMatches = numberReturned;
    } else if (objectId.rfind(kArtistPrefix, 0) == 0) {
        uint32_t id = 0;
        try { id = std::stoul(objectId.substr(strlen(kArtistPrefix))); } catch (...) {}
        const auto* artist = findArtist(id);
        if (artist) {
            for (const auto albumId : artist->albumIds) {
                const auto* album = findAlbum(albumId);
                if (!album) continue;
                out += "<container id=\"" + std::string(kAlbumPrefix) + std::to_string(album->id) + "\" parentID=\"" + objectId + "\" restricted=\"1\" childCount=\"" + std::to_string(album->itemIds.size()) + "\">";
                out += "<dc:title>" + xmlEscape(album->title) + "</dc:title><upnp:class>object.container.album.musicAlbum</upnp:class>";
                out += "<upnp:albumArtURI>" + base + "/art/" + std::to_string(album->id) + ".jpg</upnp:albumArtURI>";
                out += "</container>";
                ++numberReturned;
            }
            totalMatches = numberReturned;
        }
    } else if (objectId.rfind(kAlbumPrefix, 0) == 0) {
        uint32_t id = 0;
        try { id = std::stoul(objectId.substr(strlen(kAlbumPrefix))); } catch (...) {}
        const auto* album = findAlbum(id);
        if (album) {
            for (const auto itemId : album->itemIds) {
                const auto* item = findItem(itemId);
                if (!item) continue;
                const auto title = xmlEscape(item->track.title.empty() ? ("Track " + std::to_string(item->id)) : item->track.title);
                const auto artist = xmlEscape(item->track.artist);
                const auto albumName = xmlEscape(item->track.album);
                const auto mime = (_stricmp(item->sourceExt.c_str(), ".iso") == 0) ? std::string("audio/x-dsf") : mimeForExtension(item->sourceExt);
                out += "<item id=\"" + std::string(kTrackPrefix) + std::to_string(item->id) + "\" parentID=\"" + objectId + "\" restricted=\"1\">";
                out += "<dc:title>" + title + "</dc:title>";
                if (!artist.empty()) out += "<dc:creator>" + artist + "</dc:creator>";
                if (!albumName.empty()) out += "<upnp:album>" + albumName + "</upnp:album>";
                if (!item->track.genre.empty()) out += "<upnp:genre>" + xmlEscape(item->track.genre) + "</upnp:genre>";
                out += "<upnp:class>object.item.audioItem.musicTrack</upnp:class>";
                out += "<upnp:albumArtURI>" + base + "/art/" + std::to_string(album->id) + ".jpg</upnp:albumArtURI>";
                const std::string servedExt = (_stricmp(item->sourceExt.c_str(), ".iso") == 0) ? ".dsf" : item->sourceExt;
                out += "<res protocolInfo=\"" + didlProtocolInfo(mime) + "\">" + base + "/media/" + std::to_string(item->id) + servedExt + "</res>";
                out += "</item>";
                ++numberReturned;
            }
            totalMatches = numberReturned;
        }
    } else if (objectId.rfind(kTrackPrefix, 0) == 0) {
        uint32_t id = 0;
        try { id = std::stoul(objectId.substr(strlen(kTrackPrefix))); } catch (...) {}
        const auto* item = findItem(id);
        if (item) {
            const auto* album = findAlbum(item->albumId);
            const auto mime = (_stricmp(item->sourceExt.c_str(), ".iso") == 0) ? std::string("audio/x-dsf") : mimeForExtension(item->sourceExt);
            out += "<item id=\"" + objectId + "\" parentID=\"" + (album ? std::string(kAlbumPrefix) + std::to_string(album->id) : "artists") + "\" restricted=\"1\">";
            out += "<dc:title>" + xmlEscape(item->track.title) + "</dc:title>";
            if (!item->track.artist.empty()) out += "<dc:creator>" + xmlEscape(item->track.artist) + "</dc:creator>";
            if (!item->track.album.empty()) out += "<upnp:album>" + xmlEscape(item->track.album) + "</upnp:album>";
            out += "<upnp:class>object.item.audioItem.musicTrack</upnp:class>";
            if (album) out += "<upnp:albumArtURI>" + base + "/art/" + std::to_string(album->id) + ".jpg</upnp:albumArtURI>";
            const std::string servedExt = (_stricmp(item->sourceExt.c_str(), ".iso") == 0) ? ".dsf" : item->sourceExt;
            out += "<res protocolInfo=\"" + didlProtocolInfo(mime) + "\">" + base + "/media/" + std::to_string(item->id) + servedExt + "</res></item>";
            numberReturned = totalMatches = 1;
        }
    }

    out += "</DIDL-Lite>";
    return out;
}

std::string SacdDlnaServer::browseResponse(const std::string& objectId) const {
    unsigned numberReturned = 0, totalMatches = 0;
    const std::string didl = browseDidl(objectId, numberReturned, totalMatches);
    return
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
        "<s:Body><u:BrowseResponse xmlns:u=\"urn:schemas-upnp-org:service:ContentDirectory:1\">"
        "<Result>" + xmlEscape(didl) + "</Result>"
        "<NumberReturned>" + std::to_string(numberReturned) + "</NumberReturned>"
        "<TotalMatches>" + std::to_string(totalMatches) + "</TotalMatches>"
        "<UpdateID>1</UpdateID></u:BrowseResponse></s:Body></s:Envelope>";
}

void SacdDlnaServer::setConversionStatus(bool active, uint32_t percent) {
    std::lock_guard<std::mutex> g(m_rateMutex);
    m_conversionActive = active;
    m_conversionPercent = std::min<uint32_t>(percent, 100);
}

bool SacdDlnaServer::ensureCached(Item& item) {
    if (!item.cachePath.empty() && GetFileAttributesW(item.cachePath.c_str()) != INVALID_FILE_ATTRIBUTES) return true;
    if (_stricmp(item.sourceExt.c_str(), ".iso") != 0) return false;

    try {
        abort_callback_dummy abort;
        const auto key = cacheKeyFor(item.sourcePath, item.subsong);
        const auto path = cacheFolder() + L"\\" + pfc::stringcvt::string_wide_from_utf8(key.c_str()) + L".dsf";
        const auto tmp = path + L".partial";

        setConversionStatus(true, 0);
        const auto track = SacdDecoder::decodeToDsf(item.sourcePath.c_str(), item.subsong, tmp, abort,
            [this](uint32_t pct) { setConversionStatus(true, pct); });
        MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
        item.track = track;
        item.cachePath = path;
        setConversionStatus(false, 100);
        return true;
    } catch (std::exception const& e) {
        setConversionStatus(false, 0);
        FB2K_console_formatter() << "SACD DLNA: SACD ISO cache generation failed: " << e;
        return false;
    }
}

bool SacdDlnaServer::serveMedia(SOCKET s, uint32_t itemId, const std::string& requestLine, const std::string& requestHeaders, const std::string& peerIp) {
    std::wstring path;
    std::string ext;
    std::string title;
    uint32_t dsdRate = 0;

    {
        std::lock_guard<std::mutex> g(m_mutex);
        for (auto& item : m_items) {
            if (item.id != itemId) continue;
            if (!item.handle.is_valid()) return false;
            title = item.track.title;
            dsdRate = item.track.dsdRate;
            if (_stricmp(item.sourceExt.c_str(), ".iso") == 0) {
                if (!ensureCached(item)) return false;
                path = item.cachePath;
                ext = ".dsf";
            } else {
                path = pfc::stringcvt::string_wide_from_utf8(item.sourcePath.c_str());
                ext = item.sourceExt;
            }
            break;
        }
    }

    if (path.empty()) return false;

    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    const uint64_t size = static_cast<uint64_t>(f.tellg());

    uint64_t begin = 0, end = size ? size - 1 : 0;
    bool partial = false;
    parseRange(requestHeaders, size, begin, end, partial);
    const uint64_t length = size ? end - begin + 1 : 0;
    f.seekg(static_cast<std::streamoff>(begin));

    if (dsdRate == 0 && _stricmp(ext.c_str(), ".dsf") == 0 && size >= 72) {
        std::ifstream df(path, std::ios::binary);
        if (df) {
            df.seekg(52 + 4 + 4 + 4 + 4);
            uint32_t rate = 0;
            df.read(reinterpret_cast<char*>(&rate), sizeof(rate));
            dsdRate = rate;
        }
    }

    const std::string mime = mimeForExtension(ext);
    std::string hdr = (partial ? "HTTP/1.1 206 Partial Content\r\n" : "HTTP/1.1 200 OK\r\n");
    hdr += "Content-Type: " + mime + "\r\n";
    hdr += "Content-Length: " + std::to_string(length) + "\r\n";
    hdr += "Accept-Ranges: bytes\r\n";
    hdr += "transferMode.dlna.org: Streaming\r\n";
    hdr += "contentFeatures.dlna.org: DLNA.ORG_OP=01;DLNA.ORG_CI=0;DLNA.ORG_FLAGS=01700000000000000000000000000000\r\n";
    if (partial) hdr += "Content-Range: bytes " + std::to_string(begin) + "-" + std::to_string(end) + "/" + std::to_string(size) + "\r\n";
    hdr += "Connection: close\r\n\r\n";

    if (requestLine.rfind("HEAD ", 0) == 0) {
        sendAll(s, hdr.data(), hdr.size());
        return true;
    }

    int sndbuf = 4 * 1024 * 1024;
    if (sacd_dlna_cfg::stability_mode && dsdRate) {
        const uint64_t bytesPerSecond = static_cast<uint64_t>(dsdRate) / 4ULL;
        const uint64_t desired = bytesPerSecond * std::clamp<uint32_t>(sacd_dlna_cfg::prebuffer_seconds.get(), 5, 60);
        sndbuf = static_cast<int>(std::clamp<uint64_t>(desired, 512ull * 1024ull, 32ull * 1024ull * 1024ull));
    }
    setsockopt(s, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&sndbuf), sizeof(sndbuf));

    sendAll(s, hdr.data(), hdr.size());
    updateStreamStart(peerIp, title, dsdRate);

    std::vector<char> prebuffer;
    uint64_t prebufferTarget = 0;
    if (sacd_dlna_cfg::stability_mode && dsdRate) {
        const uint64_t audioBytesPerSecond = static_cast<uint64_t>(dsdRate) / 4ULL;
        prebufferTarget = std::min<uint64_t>(length, audioBytesPerSecond * std::clamp<uint32_t>(sacd_dlna_cfg::prebuffer_seconds.get(), 5, 60));
        prebuffer.resize(static_cast<size_t>(std::min<uint64_t>(prebufferTarget, 256ull * 1024ull * 1024ull)));
        f.read(prebuffer.data(), static_cast<std::streamsize>(prebuffer.size()));
        prebuffer.resize(static_cast<size_t>(f.gcount()));
        f.clear();
        f.seekg(static_cast<std::streamoff>(begin + prebuffer.size()));
        {
            std::lock_guard<std::mutex> g(m_rateMutex);
            m_prebufferTargetBytes = prebufferTarget;
            m_prebufferBytes = prebuffer.size();
        }
    } else {
        std::lock_guard<std::mutex> g(m_rateMutex);
        m_prebufferTargetBytes = 0;
        m_prebufferBytes = 0;
    }

    bool ok = false;
    try {
        if (!prebuffer.empty()) {
            sendAll(s, prebuffer.data(), prebuffer.size());
            updateStreamBytes(prebuffer.size());
            std::lock_guard<std::mutex> g(m_rateMutex);
            m_prebufferBytes = 0;
        }

        char buf[128 * 1024];
        uint64_t remaining = length - static_cast<uint64_t>(prebuffer.size());
        while (remaining) {
            const size_t want = static_cast<size_t>(std::min<uint64_t>(remaining, sizeof(buf)));
            f.read(buf, static_cast<std::streamsize>(want));
            const auto n = f.gcount();
            if (n <= 0) break;
            sendAll(s, buf, static_cast<size_t>(n));
            updateStreamBytes(static_cast<uint64_t>(n));
            remaining -= static_cast<uint64_t>(n);
        }
        ok = remaining == 0;
    } catch (...) {
        updateStreamEnd();
        throw;
    }
    updateStreamEnd();
    return ok;
}
bool SacdDlnaServer::serveAlbumArt(SOCKET s, uint32_t albumId, const std::string& requestLine) {
    metadb_handle_ptr handle;
    {
        std::lock_guard<std::mutex> g(m_mutex);
        for (const auto& album : m_albums) {
            if (album.id != albumId) continue;
            for (const auto& item : m_items) {
                if (item.id == album.representativeItemId) {
                    handle = item.handle;
                    break;
                }
            }
            break;
        }
    }
    if (!handle.is_valid()) return false;

    try {
        abort_callback_dummy abort;
        metadb_handle_list group;
        group += handle;
        pfc::list_t<GUID> ids;
        ids.add_item(album_art_ids::cover_front);
        auto manager = album_art_manager_v2::get();
        auto extractor = manager->open(group, ids, abort);
        auto art = extractor->query(album_art_ids::cover_front, abort);
        if (!art.is_valid() || !art->data() || !art->size()) return false;

        const std::string mime = detectImageMime(art->data(), art->size());
        const std::string hdr = "HTTP/1.1 200 OK\r\nContent-Type: " + mime + "\r\nContent-Length: " + std::to_string(art->size()) + "\r\nCache-Control: max-age=3600\r\nConnection: close\r\n\r\n";
        sendAll(s, hdr.data(), hdr.size());
        if (requestLine.rfind("HEAD ", 0) == 0) return true;
        sendAll(s, static_cast<const char*>(art->data()), art->size());
        return true;
    } catch (...) {
        return false;
    }
}

void SacdDlnaServer::handleClient(SOCKET s) {
    std::string peerIp = "unknown";
    sockaddr_in peer{};
    int peerLen = sizeof(peer);
    if (getpeername(s, reinterpret_cast<sockaddr*>(&peer), &peerLen) == 0) {
        char ip[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
        peerIp = ip;
    }
    char buf[65536]{};
    const int n = recv(s, buf, sizeof(buf) - 1, 0);
    if (n <= 0) return;
    buf[n] = 0;
    const std::string req(buf);
    const auto eol = req.find("\r\n");
    if (eol == std::string::npos) return;
    const std::string line = req.substr(0, eol);

    std::string userAgent;
    getHeaderValue(req, "User-Agent:", userAgent);
    std::string uaLower = userAgent;
    std::transform(uaLower.begin(), uaLower.end(), uaLower.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (uaLower.find("t+a") != std::string::npos || uaLower.find("sdx") != std::string::npos) {
        std::lock_guard<std::mutex> g(m_rateMutex);
        m_clientName = userAgent.empty() ? "T+A renderer" : userAgent;
        m_sdxIp = peerIp;
        m_sdxName = "T+A renderer";
        if (uaLower.find("sdx") != std::string::npos) m_sdxName = "T+A SDX renderer";
    }

    auto sendXml = [&](const std::string& body) {
        const std::string hdr = "HTTP/1.1 200 OK\r\nContent-Type: text/xml; charset=utf-8\r\nContent-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n";
        sendAll(s, hdr.data(), hdr.size());
        sendAll(s, body.data(), body.size());
    };

    if (line.rfind("GET /device.xml", 0) == 0 || line.rfind("GET /device.xml", 0) == 0) {
        sendXml(makeDeviceXml());
        return;
    }
    if (line.rfind("GET /ContentDirectory.xml", 0) == 0) { sendXml(makeContentDirectoryScpd()); return; }
    if (line.rfind("GET /ConnectionManager.xml", 0) == 0) { sendXml(makeConnectionManagerScpd()); return; }
    if (line.rfind("GET /status", 0) == 0) {
        const auto st = get_status();
        auto mbps = [](uint64_t bps) { return static_cast<double>(bps) * 8.0 / 1000000.0; };
        std::string body = "<!doctype html><html><head><meta charset=\"utf-8\"><title>SACD DLNA</title></head><body>";
        body += "<h1>foo_sacd_dlna</h1>";
        body += "<p>DLNA discovery: <b>" + std::string(st.broadcasting ? "BROADCASTING / ACTIVE" : "STOPPED") + "</b></p>";
        body += "<p>Audio stream: <b>" + std::string(st.streamingActive ? "ACTIVE / TRANSMITTING" : "IDLE") + "</b></p>";
        body += "<p>TX speed: <b>" + std::to_string(mbps(st.bytesPerSecond)) + " Mbit/s</b> (" + std::to_string(st.bytesPerSecond / 1024) + " KiB/s)</p>";
        body += "<p>DSD: " + (st.dsdRate ? std::to_string(st.dsdRate / 2822400) + "x (" + std::to_string(st.dsdRate) + " Hz)" : "-") + "</p>";
        body += "<p>DLNA client: <b>" + std::string(st.clientName.is_empty() ? st.clientIp.c_str() : st.clientName.c_str()) + "</b>";
        if (!st.clientModel.is_empty()) body += " — " + st.clientModel.c_str();
        body += "</p>";
        body += "<p>T+A SDX: <b>" + std::string(st.sdxDetected ? (st.sdxStreaming ? "DETECTED / STREAMING" : "DETECTED / IDLE") : "NOT DETECTED") + "</b>";
        if (!st.sdxIp.is_empty()) body += " — " + st.sdxIp.c_str();
        if (!st.sdxName.is_empty()) body += " — " + st.sdxName.c_str();
        body += "</p>";
        body += "<p>foo_input_sacd: " + std::string(st.sacdInstalled ? "INSTALLED" : "NOT INSTALLED") + " " + st.sacdVersion.c_str() + "</p>";
        body += "<p>Music Library: " + std::string(st.sharingLibrary ? "SHARING" : "NOT SHARING") + " (" + std::to_string(st.sharedCount) + " DSD tracks)</p></body></html>";
        const auto hdr = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: " + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n";
        sendAll(s, hdr.data(), hdr.size()); sendAll(s, body.data(), body.size());
        return;
    }
    if (line.rfind("POST /ctl/ContentDirectory", 0) == 0) {
        const auto bodyStart = req.find("\r\n\r\n");
        const std::string soap = bodyStart == std::string::npos ? std::string{} : req.substr(bodyStart + 4);
        std::string objectId = "0";
        const auto p = soap.find("<ObjectID>");
        const auto q = soap.find("</ObjectID>");
        if (p != std::string::npos && q != std::string::npos && q > p + 10) objectId = soap.substr(p + 10, q - (p + 10));
        const auto body = browseResponse(urlPathDecode(objectId));
        const auto hdr = "HTTP/1.1 200 OK\r\nContent-Type: text/xml; charset=\"utf-8\"\r\nContent-Length: " + std::to_string(body.size()) + "\r\nEXT:\r\nConnection: close\r\n\r\n";
        sendAll(s, hdr.data(), hdr.size()); sendAll(s, body.data(), body.size());
        return;
    }
    if (line.rfind("POST /ctl/ConnectionManager", 0) == 0) {
        const std::string body = "<?xml version=\"1.0\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body><u:GetProtocolInfoResponse xmlns:u=\"urn:schemas-upnp-org:service:ConnectionManager:1\"><Source>http-get:*:audio/x-dsf:*,http-get:*:audio/x-dff:*</Source><Sink></Sink></u:GetProtocolInfoResponse></s:Body></s:Envelope>";
        const auto hdr = "HTTP/1.1 200 OK\r\nContent-Type: text/xml; charset=\"utf-8\"\r\nContent-Length: " + std::to_string(body.size()) + "\r\nEXT:\r\nConnection: close\r\n\r\n";
        sendAll(s, hdr.data(), hdr.size()); sendAll(s, body.data(), body.size());
        return;
    }

    const bool isHead = line.rfind("HEAD ", 0) == 0;
    const auto mediaPos = line.find("/media/");
    if (mediaPos != std::string::npos && (line.rfind("GET ", 0) == 0 || isHead)) {
        const auto pathStart = mediaPos + 7;
        const auto pathEnd = line.find('.', pathStart);
        if (pathEnd != std::string::npos) {
            try {
                const uint32_t id = std::stoul(line.substr(pathStart, pathEnd - pathStart));
                if (serveMedia(s, id, line, req, peerIp)) return;
            } catch (...) {}
        }
    }

    const auto artPos = line.find("/art/");
    if (artPos != std::string::npos && (line.rfind("GET ", 0) == 0 || isHead)) {
        const auto pathStart = artPos + 5;
        const auto pathEnd = line.find('.', pathStart);
        if (pathEnd != std::string::npos) {
            try {
                const uint32_t id = std::stoul(line.substr(pathStart, pathEnd - pathStart));
                if (serveAlbumArt(s, id, line)) return;
            } catch (...) {}
        }
    }

    const char* body = "Not Found";
    const std::string hdr = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\nConnection: close\r\n\r\n";
    sendAll(s, hdr.data(), hdr.size()); sendAll(s, body, 9);
}

void SacdDlnaServer::httpLoop() {
    m_httpListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_httpListen == INVALID_SOCKET) return;

    BOOL reuse = TRUE;
    setsockopt(m_httpListen, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(m_port);

    if (bind(m_httpListen, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR || listen(m_httpListen, 8) == SOCKET_ERROR) {
        closesocket(m_httpListen);
        m_httpListen = INVALID_SOCKET;
        console::print("SACD DLNA: HTTP bind/listen failed");
        m_running = false;
        return;
    }

    while (m_running) {
        sockaddr_in client{};
        int len = sizeof(client);
        SOCKET clientSocket = accept(m_httpListen, reinterpret_cast<sockaddr*>(&client), &len);
        if (clientSocket == INVALID_SOCKET) {
            if (m_running) continue;
            break;
        }
        try { handleClient(clientSocket); } catch (...) {}
        closesocket(clientSocket);
    }
}

void SacdDlnaServer::ssdpLoop() {
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return;

    BOOL reuse = TRUE;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(1900);
    bind(s, reinterpret_cast<sockaddr*>(&local), sizeof(local));

    ip_mreq mreq{};
    mreq.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<const char*>(&mreq), sizeof(mreq));

    sockaddr_in multicast{};
    multicast.sin_family = AF_INET;
    multicast.sin_addr.s_addr = inet_addr("239.255.255.250");
    multicast.sin_port = htons(1900);

    const std::string location = "http://" + localAddress() + ":" + std::to_string(m_port) + "/device.xml";
    const std::string uuidUsn = std::string(kUuid) + "::upnp:rootdevice";
    const std::string deviceUsn = std::string(kUuid) + "::urn:schemas-upnp-org:device:MediaServer:1";

    auto notifyAlive = [&]() {
        const std::string common =
            "HOST: 239.255.255.250:1900\r\nCACHE-CONTROL: max-age=1800\r\nLOCATION: " + location +
            "\r\nNT: %s\r\nNTS: ssdp:alive\r\nSERVER: Windows/10 UPnP/1.1 foo_sacd_dlna/0.5-alpha1\r\nUSN: %s\r\n\r\n";
        char msg[2048]{};
        const char* formats[] = {
            "upnp:rootdevice", "uuid:7e2f2d7e-7d89-4d3c-9f3e-3a7c09fd1234", "urn:schemas-upnp-org:device:MediaServer:1"
        };
        const char* usns[] = { uuidUsn.c_str(), kUuid, deviceUsn.c_str() };
        for (int i = 0; i < 3; ++i) {
            const int n = snprintf(msg, sizeof(msg), common.c_str(), formats[i], usns[i]);
            if (n > 0) sendto(s, msg, n, 0, reinterpret_cast<sockaddr*>(&multicast), sizeof(multicast));
        }
    };

    notifyAlive();
    auto nextAlive = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    auto nextDiscover = std::chrono::steady_clock::now() + std::chrono::seconds(2);

    auto discover = [&]() {
        const std::string search =
            "M-SEARCH * HTTP/1.1\r\n"
            "HOST: 239.255.255.250:1900\r\n"
            "MAN: \"ssdp:discover\"\r\n"
            "MX: 2\r\n"
            "ST: urn:schemas-upnp-org:device:MediaRenderer:1\r\n\r\n";
        sendto(s, search.data(), static_cast<int>(search.size()), 0, reinterpret_cast<sockaddr*>(&multicast), sizeof(multicast));
        const std::string all =
            "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: \"ssdp:discover\"\r\nMX: 2\r\nST: ssdp:all\r\n\r\n";
        sendto(s, all.data(), static_cast<int>(all.size()), 0, reinterpret_cast<sockaddr*>(&multicast), sizeof(multicast));
    };

    discover();

    while (m_running) {
        if (std::chrono::steady_clock::now() >= nextAlive) {
            notifyAlive();
            nextAlive = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        }
        if (std::chrono::steady_clock::now() >= nextDiscover) {
            discover();
            nextDiscover = std::chrono::steady_clock::now() + std::chrono::seconds(20);
        }

        fd_set set;
        FD_ZERO(&set);
        FD_SET(s, &set);
        timeval tv{1, 0};
        if (select(0, &set, nullptr, nullptr, &tv) <= 0) continue;

        char buf[4096]{};
        sockaddr_in from{};
        int fromLen = sizeof(from);
        const int n = recvfrom(s, buf, sizeof(buf) - 1, 0, reinterpret_cast<sockaddr*>(&from), &fromLen);
        if (n <= 0) continue;
        buf[n] = 0;
        const std::string msg(buf);
        if (msg.rfind("HTTP/1.1 200", 0) == 0) {
            char peerIp[INET_ADDRSTRLEN]{};
            inet_ntop(AF_INET, &from.sin_addr, peerIp, sizeof(peerIp));
            discoverSdxResponse(msg, peerIp);
            continue;
        }
        if (msg.find("M-SEARCH") == std::string::npos) continue;

        std::string st = "ssdp:all";
        const auto stPos = msg.find("ST:");
        if (stPos != std::string::npos) {
            const auto e = msg.find("\r\n", stPos);
            st = msg.substr(stPos + 3, e == std::string::npos ? std::string::npos : e - stPos - 3);
            while (!st.empty() && (st.front() == ' ' || st.front() == '\t')) st.erase(st.begin());
        }

        if (st != "ssdp:all" && st != "upnp:rootdevice" && st != kUuid && st != "urn:schemas-upnp-org:device:MediaServer:1") continue;

        const std::string usn = (st == "upnp:rootdevice") ? uuidUsn : (st == kUuid ? std::string(kUuid) : deviceUsn);
        const std::string response =
            "HTTP/1.1 200 OK\r\nCACHE-CONTROL: max-age=1800\r\nEXT:\r\nLOCATION: " + location +
            "\r\nSERVER: Windows/10 UPnP/1.1 foo_sacd_dlna/0.5-alpha1\r\nST: " + st + "\r\nUSN: " + usn + "\r\n\r\n";
        sendto(s, response.data(), static_cast<int>(response.size()), 0, reinterpret_cast<sockaddr*>(&from), fromLen);
    }

    closesocket(s);
}

void SacdDlnaServer::clear_shared_library() {
    std::lock_guard<std::mutex> g(m_mutex);
    m_items.clear();
    m_artists.clear();
    m_albums.clear();
    m_sharedCount = 0;
    m_sharingLibrary = false;
}

void SacdDlnaServer::publish(const metadb_handle_list& items) {
    std::vector<Item> newItems;
    newItems.reserve(items.get_count());

    for (size_t i = 0; i < items.get_count(); ++i) {
        const auto& handle = items[i];
        const char* path = handle->get_path();
        if (!path || !*path) continue;

        const char* ext = strrchr(path, '.');
        if (!ext) continue;
        const std::string extLower = ext;
        if (_stricmp(ext, ".iso") != 0 && _stricmp(ext, ".dsf") != 0 && _stricmp(ext, ".dff") != 0) continue;

        Item x;
        x.id = m_nextId++;
        x.sourcePath = path;
        x.sourceExt = extLower;
        x.subsong = handle->get_subsong_index();
        x.handle = handle;

        try {
            abort_callback_dummy abort;
            service_ptr_t<input_info_reader> infoReader;
            input_entry::g_open_for_info_read(infoReader, nullptr, path, abort);
            file_info info;
            infoReader->get_info(x.subsong, info, abort);

            auto getMeta = [&](const char* key) -> std::string {
                const char* v = info.meta_get(key, 0);
                return v ? v : "";
            };

            x.track.title = getMeta("title");
            x.track.artist = getMeta("artist");
            x.track.album = getMeta("album");
            x.track.genre = getMeta("genre");

            if (x.track.title.empty()) x.track.title = "Track " + std::to_string(x.subsong + 1);
            if (x.track.artist.empty()) x.track.artist = "Unknown Artist";
            if (x.track.album.empty()) x.track.album = "Unknown Album";

            x.track.path = x.sourceExt == ".iso" ? std::wstring{} : std::wstring(x.sourcePath.begin(), x.sourcePath.end());
            newItems.push_back(std::move(x));
        } catch (std::exception const& e) {
            FB2K_console_formatter() << "SACD DLNA: unable to index " << path << ": " << e;
        }
    }

    std::vector<Artist> newArtists;
    std::vector<Album> newAlbums;

    auto findArtist = [&](const std::string& key) -> Artist* {
        for (auto& x : newArtists) if (x.key == key) return &x;
        return nullptr;
    };
    auto findAlbum = [&](uint32_t artistId, const std::string& key) -> Album* {
        for (auto& x : newAlbums) if (x.artistId == artistId && x.key == key) return &x;
        return nullptr;
    };

    for (auto& item : newItems) {
        const std::string artistKey = normalizeKey(item.track.artist);
        Artist* artist = findArtist(artistKey);
        if (!artist) {
            Artist a;
            a.id = m_nextArtistId++;
            a.name = item.track.artist;
            a.key = artistKey;
            newArtists.push_back(std::move(a));
            artist = &newArtists.back();
        }
        item.artistId = artist->id;

        const std::string albumKey = normalizeKey(item.track.album);
        Album* album = findAlbum(artist->id, albumKey);
        if (!album) {
            Album a;
            a.id = m_nextAlbumId++;
            a.artistId = artist->id;
            a.title = item.track.album;
            a.key = albumKey;
            a.representativeItemId = item.id;
            newAlbums.push_back(std::move(a));
            album = &newAlbums.back();
            artist->albumIds.push_back(album->id);
        }
        item.albumId = album->id;
        album->itemIds.push_back(item.id);
    }

    auto ciLess = [](const std::string& a, const std::string& b) {
        return _stricmp(a.c_str(), b.c_str()) < 0;
    };
    std::sort(newArtists.begin(), newArtists.end(), [&](const Artist& a, const Artist& b) { return ciLess(a.name, b.name); });
    std::sort(newAlbums.begin(), newAlbums.end(), [&](const Album& a, const Album& b) {
        const auto* aa = std::find_if(newArtists.begin(), newArtists.end(), [&](const Artist& x) { return x.id == a.artistId; });
        const auto* bb = std::find_if(newArtists.begin(), newArtists.end(), [&](const Artist& x) { return x.id == b.artistId; });
        const std::string an = aa ? aa->name : "";
        const std::string bn = bb ? bb->name : "";
        if (_stricmp(an.c_str(), bn.c_str()) != 0) return ciLess(an, bn);
        return ciLess(a.title, b.title);
    });

    for (auto& album : newAlbums) {
        std::sort(album.itemIds.begin(), album.itemIds.end(), [&](uint32_t a, uint32_t b) {
            const auto* ia = std::find_if(newItems.begin(), newItems.end(), [&](const Item& x) { return x.id == a; });
            const auto* ib = std::find_if(newItems.begin(), newItems.end(), [&](const Item& x) { return x.id == b; });
            if (!ia || !ib) return a < b;
            return ia->track.title < ib->track.title;
        });
    }

    {
        std::lock_guard<std::mutex> g(m_mutex);
        m_items = std::move(newItems);
        m_artists = std::move(newArtists);
        m_albums = std::move(newAlbums);
        m_sharedCount = m_items.size();
    }

    FB2K_console_formatter() << "SACD DLNA: indexed " << static_cast<unsigned>(m_sharedCount.load()) << " DSD tracks";
}

void SacdDlnaServer::share_music_library() {
    if (!sacd_plugin_installed()) {
        popup_message::g_show("The Super Audio CD Decoder (foo_input_sacd.dll) is required before the DSD Music Library can be shared.", "SACD DLNA");
        return;
    }
    if (!library_manager::get()->is_library_enabled()) {
        library_manager::get()->show_preferences();
        console::print("SACD DLNA: Media Library is not enabled; opened foobar2000 Music Library preferences");
        return;
    }

    pfc::list_t<metadb_handle_ptr> all;
    library_manager::get()->get_all_items(all);

    metadb_handle_list dsd;
    for (size_t i = 0; i < all.get_count(); ++i) {
        const char* path = all[i]->get_path();
        if (!path) continue;
        const char* ext = strrchr(path, '.');
        if (!ext) continue;
        if (_stricmp(ext, ".iso") == 0 || _stricmp(ext, ".dsf") == 0 || _stricmp(ext, ".dff") == 0) dsd += all[i];
    }

    publish(dsd);
    m_sharingLibrary = true;
    console::printf("SACD DLNA: Music Library SHARING / %u DSD tracks", static_cast<unsigned>(m_sharedCount.load()));
}
