#pragma once

#include "stdafx.h"

struct SacdDlnaStatus {
    bool broadcasting = false;
    bool sacdInstalled = false;
    bool sharingLibrary = false;
    bool streamingActive = false;
    bool sdxDetected = false;
    bool sdxStreaming = false;
    bool sdxProtocolNegotiated = false;
    bool stabilityMode = true;
    bool conversionActive = false;
    bool networkLogging = false;
    bool dsdProcessorInstalled = false;
    bool dsdProcessorEnabled = false;
    size_t sharedCount = 0;
    uint32_t activeStreams = 0;
    uint16_t port = 8192;
    uint64_t bytesPerSecond = 0;
    uint64_t totalBytesSent = 0;
    uint64_t streamElapsedMs = 0;
    uint64_t cacheBytes = 0;
    uint64_t streamBytesSent = 0;
    uint64_t cacheHits = 0;
    uint64_t cacheMisses = 0;
    uint32_t dsdRate = 0;
    uint64_t nominalBitrate = 0;
    uint64_t requiredBytesPerSecond = 0;
    uint64_t prebufferBytes = 0;
    uint64_t prebufferTargetBytes = 0;
    uint32_t conversionPercent = 0;
    double networkHeadroom = 0.0;
    uint32_t updateId = 1;
    pfc::string8 serverName;
    pfc::string8 sacdVersion;
    pfc::string8 dsdProcessorVersion;
    pfc::string8 clientIp;
    pfc::string8 clientName;
    pfc::string8 clientModel;
    pfc::string8 streamTitle;
    pfc::string8 sdxIp;
    pfc::string8 sdxName;
    pfc::string8 sdxModel;
    pfc::string8 sdxModelNumber;
    pfc::string8 sdxProtocolInfo;
    pfc::string8 lastError;
    pfc::string8 lastHttpRequest;
};
