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
    void request_library_refresh();
    void clear_persistent_cache();
    uint64_t persistent_cache_bytes() const;
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
        uint64_t sourceSize = 0;
        int64_t sourceWriteTime = 0;
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

    struct CacheJob {
        std::condition_variable cv;
        bool done = false;
        bool success = false;
        std::shared_ptr<abort_callback_impl> aborter = std::make_shared<abort_callback_impl>();
    };

    struct ClientState {
        SOCKET socket = INVALID_SOCKET;
        std::shared_ptr<abort_callback_impl> aborter;
    };

    struct ArtCache {
        std::string mime;
        std::vector<uint8_t> bytes;
        std::wstring path;
    };

    std::atomic<bool> m_running{false};
    std::atomic<size_t> m_sharedCount{0};
    std::atomic<bool> m_sharingLibrary{false};
    std::atomic<bool> m_libraryRefreshPending{false};
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
    uint32_t m_updateId = 1;
    std::string m_lastHttpRequest;

    std::mutex m_clientMutex;
    std::vector<ClientState> m_clients;
    std::vector<std::thread> m_clientThreads;

    std::mutex m_cacheMutex;
    std::map<std::string, std::shared_ptr<CacheJob>> m_cacheJobs;

    std::mutex m_artMutex;
    std::map<uint32_t, ArtCache> m_artCache;

    std::shared_ptr<class library_tracker> m_libraryTracker;

    std::string m_sdxProtocolInfo;
    std::string m_sdxConnectionManagerControl;
    std::string m_sdxModelNumber;

    void httpLoop();
    void ssdpLoop();
    void clientThread(SOCKET s, std::shared_ptr<abort_callback_impl> aborter);
    void handleClient(SOCKET s, abort_callback_impl& aborter);
    void closeClientState(SOCKET s);

    std::string makeDeviceXml() const;
    std::string makeContentDirectoryScpd() const;
    std::string makeConnectionManagerScpd() const;
    std::string browseDidl(const std::string& objectId,
                           bool metadataOnly,
                           unsigned startingIndex,
                           unsigned requestedCount,
                           unsigned& numberReturned,
                           unsigned& totalMatches) const;
    std::string browseResponse(const std::string& objectId,
                               const std::string& browseFlag,
                               unsigned startingIndex,
                               unsigned requestedCount,
                               const std::string& filter,
                               const std::string& sortCriteria) const;
    std::string systemUpdateIdResponse() const;

    bool serveMedia(SOCKET s, uint32_t itemId, const std::string& requestLine,
                    const std::string& requestHeaders, const std::string& peerIp,
                    abort_callback_impl& aborter);
    bool serveAlbumArt(SOCKET s, uint32_t albumId, const std::string& requestLine,
                       abort_callback_impl& aborter);

    std::string localAddress() const;
    std::wstring cacheFolder() const;
    bool ensureCached(const Item& item, std::wstring& cachePath, DsdTrack& track,
                      abort_callback& abort);
    bool ensureProcessedDsf(const Item& item, std::wstring& cachePath, DsdTrack& track,
                            abort_callback& abort);
    void setConversionStatus(bool active, uint32_t percent);

    void refreshLibraryNow();

    static std::string xmlEscape(const std::string& s);
    static std::string urlPathDecode(const std::string& s);
    static std::string normalizeKey(const std::string& s);
    static std::string mimeForExtension(const std::string& ext);
    static std::string detectImageMime(const void* data, size_t size);
    static std::string didlProtocolInfo(const std::string& mime);
    static std::string firstXmlTagText(const std::string& xml, const std::string& localName);
    static std::string extractSoapArg(const std::string& soap, const char* localName, const std::string& fallback = {});
    static unsigned extractSoapUint(const std::string& soap, const char* localName, unsigned fallback);
    static std::string xmlLocalTagValueCI(const std::string& xml, const char* localName);
    static std::string resolveUrl(const std::string& baseUrl, const std::string& relative);
    static std::string extractConnectionManagerControl(const std::string& deviceXml, const std::string& baseUrl);
    static std::string extractProtocolList(const std::string& soap, const char* tag);
    static bool protocolListSupports(const std::string& protocols, const std::string& mime);
    std::string chooseRendererMime(const std::string& defaultMime) const;
    std::string chooseRendererProtocolInfo(const std::string& defaultMime) const;
    static bool readDsfHeader(const std::wstring& path, uint32_t& rate,
                              uint32_t& channels, uint64_t& samplesPerChannel,
                              uint64_t& fileSize);

    void updateStreamStart(const std::string& peerIp, const std::string& title, uint32_t dsdRate);
    void updateStreamBytes(uint64_t bytes);
    void updateStreamEnd();
    void updateLastHttpRequest(const std::string& request);
    void discoverSdxResponse(const std::string& response, const std::string& peerIp);
    void probeSdxDescription(const std::string& location, const std::string& peerIp);
    void probeSdxConnectionManager(const std::string& controlUrl, const std::string& peerIp);

    mutable std::mutex m_rateMutex;
    uint64_t m_rateBytes = 0;
    mutable uint64_t m_lastRateBytes = 0;
    mutable std::chrono::steady_clock::time_point m_lastRateTick{};
    mutable uint64_t m_currentBps = 0;
    uint64_t m_totalBytes = 0;
    uint64_t m_streamBytes = 0;
    uint64_t m_streamElapsedMs = 0;
    std::chrono::steady_clock::time_point m_streamStartTick{};
    uint64_t m_cacheHits = 0;
    uint64_t m_cacheMisses = 0;
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
