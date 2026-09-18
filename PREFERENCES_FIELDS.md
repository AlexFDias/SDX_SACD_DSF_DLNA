# Preferences Field Reference

| Field | Meaning | Typical value |
|---|---|---|
| Enable DLNA broadcasting | Starts SSDP + HTTP/DLNA server | ON |
| Share DSD content | Publishes foobar2000 DSD library | ON |
| Server name | Friendly UPnP server name | foobar2000 SACD DSD |
| HTTP port | HTTP/media server port | 8192 |
| Enable stability mode | Decouples DSD preparation from network delivery | ON |
| Pre-buffer seconds | Read-ahead target | 15 s |

## Status fields

| Status | Meaning |
|---|---|
| DLNA | Discovery/server status |
| foo_input_sacd | Required decoder detection |
| Music Library | Shared DSD item count |
| Audio stream | Actual HTTP media transfer |
| T+A SDX | Detected renderer state |
| DSD cache/buffer | Conversion/read-ahead state |
