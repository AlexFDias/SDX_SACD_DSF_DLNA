#pragma once

#include "stdafx.h"
#include "dsf_writer.h"
#include "status.h"

class SacdDlnaServer {
public:
    static SacdDlnaServer& instance();

    void start();
    void stop();
    void set_enabled(bool enabled);
    bool is_running() const noexcept { return m_running.load(); }

    void publish(const metadb_handle_list& items);
    void share_music_library();
    void clear_shared_library();
    size_t shared_count() const;
    SacdDlnaStatus get_status() const;

private:
    SacdDlnaServer() = default;
    ~SacdDlnaServer() = default;
    SacdDlnaServer(const SacdDlnaServer&) = delete;
    SacdDlnaServer& operator=(const SacdDlnaServer&) = delete;

    struct Item {
        uint32_t id = 0;
        std::string sourcePath;
        std::string sourceExt;
        t_uint32 subsong = 0;
        metadb_handle_ptr handle;
        DsdTrack track;
        std::wstring cachePath;
        uint32_t artistId = 0;
        uint32_t albumId = 0;
    };

    struct Artist {
        uint32_t id = 0;
        std::string name;
        std::string key;
        std::vector<uint32_t> albumIds;
    };

    struct Album {
        uint32_t id = 0;
        uint32_t artistId = 0;
        std::string title;
        std::string key;
        uint32_t representativeItemId = 0;
        std::vector<uint32_t> itemIds;
    };

    std::atomic<bool> m_running{false};
    std::atomic<size_t> m_sharedCount{0};
    std::atomic<bool> m_sharingLibrary{false};
    std::thread m_httpThread;
    std::thread m_ssdpThread;
    SOCKET m_httpListen = INVALID_SOCKET;
    mutable std::mutex m_mutex;
    std::vector<Item> m_items;
    std::vector<Artist> m_artists;
    std::vector<Album> m_albums;
    uint32_t m_nextId = 1;
    uint32_t m_nextArtistId = 1;
    uint32_t m_nextAlbumId = 1;
    uint16_t m_port = 8192;

    void httpLoop();
    void ssdpLoop();
    void handleClient(SOCKET s);

    std::string makeDeviceXml() const;
    std::string makeContentDirectoryScpd() const;
    std::string makeConnectionManagerScpd() const;
    std::string browseDidl(const std::string& objectId, unsigned& numberReturned, unsigned& totalMatches) const;
    std::string browseResponse(const std::string& objectId) const;

    bool serveMedia(SOCKET s, uint32_t itemId, const std::string& requestLine, const std::string& requestHeaders, const std::string& peerIp);
    bool serveAlbumArt(SOCKET s, uint32_t albumId, const std::string& requestLine);

    std::string localAddress() const;
    std::wstring cacheFolder() const;
    bool ensureCached(Item& item);
    void setConversionStatus(bool active, uint32_t percent);

    static std::string xmlEscape(const std::string& s);
    static std::string urlPathDecode(const std::string& s);
    static std::string normalizeKey(const std::string& s);
    static std::string mimeForExtension(const std::string& ext);
    static std::string detectImageMime(const void* data, size_t size);
    static std::string didlProtocolInfo(const std::string& mime);
    void updateStreamStart(const std::string& peerIp, const std::string& title, uint32_t dsdRate);
    void updateStreamBytes(uint64_t bytes);
    void updateStreamEnd();
    void discoverSdxResponse(const std::string& response, const std::string& peerIp);
    void probeSdxDescription(const std::string& location, const std::string& peerIp);

    mutable std::mutex m_rateMutex;
    uint64_t m_rateBytes = 0;
    uint64_t m_lastRateBytes = 0;
    std::chrono::steady_clock::time_point m_lastRateTick{};
    uint64_t m_currentBps = 0;
    uint64_t m_totalBytes = 0;
    uint64_t m_streamBytes = 0;
    uint32_t m_activeStreams = 0;
    std::string m_clientIp;
    std::string m_clientName;
    std::string m_clientModel;
    std::string m_streamTitle;
    uint32_t m_streamDsdRate = 0;
    std::string m_sdxIp;
    std::string m_sdxName;
    std::string m_sdxModel;
    std::string m_sdxManufacturer;
    bool m_conversionActive = false;
    uint32_t m_conversionPercent = 0;
    uint64_t m_prebufferBytes = 0;
    uint64_t m_prebufferTargetBytes = 0;
};
