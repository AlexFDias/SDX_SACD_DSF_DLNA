# Changelog

## 0.6-alpha3
- Added detailed in-app Help with practical configuration examples.
- Added HELP.md explanations for every preference and status field.
- Added EXAMPLES.md with SACD ISO, DSF, DSD256 and congestion examples.
- Added NETWORK_REQUIREMENTS and field reference documentation.
- Added contextual tooltips to preferences/status fields.
- Clarified the difference between SSDP broadcasting/discovery and actual HTTP audio transmission.

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
