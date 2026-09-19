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
