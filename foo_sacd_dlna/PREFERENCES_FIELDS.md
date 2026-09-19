# Preferences Field Reference

| Field | Purpose | Typical value |
|---|---|---|
| Enable DLNA broadcasting | Starts SSDP discovery and HTTP MediaServer | ON |
| Share DSD content | Publishes DSD items from the foobar2000 Media Library | ON |
| Server name | Name displayed by UPnP players | foobar2000 SACD DSD |
| HTTP port | TCP port for HTTP/XML/media delivery | 8192 |
| Enable stability mode | Separates cache preparation from network delivery | ON |
| Pre-buffer (seconds) | Target DSD read-ahead | 15 |
| Verbose network logging | Adds timestamps/protocol diagnostics to the log | OFF normally |

## Live status fields

| Status | Meaning |
|---|---|
| DLNA | `BROADCASTING / ACTIVE` means the MediaServer/SSDP service is running. |
| foo_input_sacd | Required decoder detection and version. |
| Music Library | Whether the DSD library is shared and item count. |
| Audio stream | `TRANSMITTING` means actual HTTP media delivery. |
| TX speed | Measured TCP transmit rate. |
| T+A SDX | Detected renderer identity and active-stream correlation. |
| DSD cache/buffer | SACD conversion/cache and read-ahead state. |

| Process DLNA audio through DSD Processor | Runs the installed DSD Processor as a private DLNA-only DSP chain. Result must be DSD. | OFF (native DSD) |
| Configure DSD Processor... | Opens the installed DSP Processor configuration and saves its preset for DLNA use. | Configure per desired DSD output |

## Build toolchain note

This repository uses **MSVC v142** with the WTL headers from the SDK tree at:

```text
D:\SDX_SACD_DSF_DLNA\SDK-2025-03-07\WTL\include
```

See `BUILD.md`, `WTL_SETUP.md` and `V142_WTL_FIX.md` for the complete configuration.
