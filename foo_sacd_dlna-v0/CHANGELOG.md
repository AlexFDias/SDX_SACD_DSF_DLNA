# Changelog

## 0.8-alpha1
- Added optional private DLNA DSP processing through installed foo_dsd_processor.
- Added DSD Processor detection/version status and configuration button.
- Added PCM source sharing when DLNA DSD Processor mode is enabled.
- Added DSP preset fingerprint to persistent DSF cache invalidation.
- Added native-DSD output validation after DSP processing.

## 0.7-alpha4
- Added `BUILD.md` with complete Windows compilation/setup guide.
- Added build-machine hardware requirements and runtime requirements.
- Documented Visual Studio 2022, MSVC, Windows SDK and foobar2000 SDK setup.
- Added command-line build, runtime test, DLNA diagnostics, firewall, CI and release checklists.
- Added references to official Microsoft and foobar2000 documentation.

## 0.7-alpha3
- Improved BrowseMetadata and DIDL-Lite metadata/artist roles.
- Renderer-aware protocolInfo selection using ConnectionManager Sink capabilities.
- Added persistent artwork cache with source invalidation.
- Added persistent DSF cache manifest with source/decoder/version invalidation.
- Added cache cleanup UI.
- Added bounded concurrent stream handling and improved request diagnostics.
- Added stream elapsed time and persistent cache size to status.
- Improved Media Library change tracking documentation.
- Added T+A renderer probe and hardware validation plan.
- Added expanded real-DLNA diagnostic documentation.

## 0.7-alpha2
- Real MediaServer browse path with separate BrowseMetadata handling.
- Renderer-aware DSD MIME/protocolInfo negotiation through ConnectionManager.
- Leaf-track BrowseDirectChildren corrected to return zero children.
- Concurrent HTTP client handling and cancellation retained for live renderer requests.
- Persistent/invalidation-aware SACD→DSF cache and duplicate decode coalescing.
- Media Library callback tracking with SystemUpdateID.
- Console + timestamped `network.log` diagnostics.
- Added Windows GitHub Actions build workflow and dependency-free DLNA smoke test.
- Added exact-firmware T+A hardware validation checklist.
- Updated in-app help to explain diagnostics and renderer-dependent gapless behavior.


## 0.6-alpha2
- Added minimum/recommended hardware and network requirements to README.
- Added DSD64/128/256 bandwidth reference.
- Added recommended Gigabit Ethernet topology and Wi-Fi guidance.
- Added NETWORK_REQUIREMENTS.md.

## 0.6 Alpha 1
- Added Stability Mode with persistent SACD→DSD DSF cache.
- Added configurable 5–120 s delivery read-ahead and TCP send-buffer sizing.
- Added conversion progress and buffer/read-ahead status reporting.
- Added required DSD bitrate and network headroom monitoring.


## 0.5-alpha1

- Added live DLNA discovery/broadcasting state.
- Added active audio transmission state.
- Added measured TCP transmit speed in Mbit/s and KiB/s.
- Added total and current-stream byte counters.
- Added active DLNA client IP indication.
- Added DSD rate indication when known.
- Added SSDP/UPnP discovery of T+A renderers.
- Added `T+A SDX: DETECTED / STREAMING` state when the active client matches the detected renderer.
- Added User-Agent based T+A renderer identification when available.
- Added live monitoring to the Preferences page and `SACD DLNA Status` UI element.

## Stability Mode (V0.6)

Stability Mode decouples SACD ISO → DSD conversion from the network delivery path. ISO tracks are converted to a persistent DSF cache and transmission starts only after the DSD file is ready. A configurable 5–120 second read-ahead and a larger TCP send buffer can be used before transmission.

This is useful for short disk/network fluctuations. No server-side buffer can guarantee uninterrupted playback when sustained network throughput is below the bitrate required by the selected DSD rate.

Approximate stereo payload rates: DSD64 = 5.64 Mbit/s; DSD128 = 11.29 Mbit/s; DSD256 = 22.58 Mbit/s.
