# foo_sacd_dlna Help — V0.6 Alpha 3

## 1. What is foo_sacd_dlna?

`foo_sacd_dlna` is a foobar2000 component intended to make DSD music available to a compatible UPnP/DLNA network renderer such as the T+A SDX 3100 HV.

The design deliberately separates three jobs:

```text
foobar2000 Music Library
        │
        ├── DSF / DFF ───────────────┐
        │                            │
        └── SACD ISO                 │
              │                      │
              ▼                      │
       foo_input_sacd                │
              │                      │
              ▼                      │
            Native DSD               │
              │                      │
              ▼                      │
         DSF cache                   │
              │                      │
              └──────────┬───────────┘
                         ▼
                    foo_sacd_dlna
                         │
                    UPnP / DLNA
                         │
                         ▼
                 T+A SDX 3100 HV
```

The component is deliberately **DSD-only**. It does not convert DSD to PCM for network delivery and does not send DoP to the network renderer.

---

## 2. Why require `foo_input_sacd`?

SACD ISO decoding belongs to the established SACD decoder component.

`foo_sacd_dlna` therefore does not copy the SACD decoder into itself. For SACD ISO sources, the installed `foo_input_sacd` remains the decoder.

The intended benefit is maintenance:

- you can update `foo_input_sacd` independently;
- `foo_sacd_dlna` does not depend on private functions exported by the SACD DLL;
- the component checks that the SACD decoder is installed before SACD functionality is enabled.

This is preferable to loading undocumented/private DLL entry points, which could break when the SACD decoder changes.

---

## 3. Why DSF/DSD instead of PCM?

SACD contains DSD. Converting it to PCM before network playback would change the signal path and defeat the purpose of a native-DSD workflow.

The network path is therefore:

```text
SACD ISO → DSD → DSF → HTTP/DLNA → T+A
```

instead of:

```text
SACD ISO → DSD → PCM → FLAC/WAV → T+A
```

`foo_sacd_dlna` supports the intended SACD streaming range:

- DSD64
- DSD128
- DSD256

Higher DSD rates may exist in the source/decoder ecosystem, but network renderer support must be treated separately and should never be assumed.

---

## 4. Preferences page

Open:

`File → Preferences → Tools → SACD DLNA`

### Enable DLNA broadcasting

**Tooltip:** Starts the UPnP/DLNA server and SSDP discovery.

When enabled, the server advertises itself so network players can find it.

Important distinction:

- **BROADCASTING / ACTIVE** = SSDP/server discovery is running.
- **TRANSMITTING** = an audio renderer is actually downloading media.

The word "broadcasting" therefore refers to **service discovery**, not the audio payload.

### Share DSD content from foobar2000 Music Library

**Tooltip:** Exposes DSD-capable music from the foobar2000 Media Library through UPnP/DLNA.

The library is presented conceptually as:

```text
SACD / DSD
└── Artist
    └── Album
        └── Track
```

The original files are not moved. The component keeps references to the foobar2000 media items.

### Server name

**Tooltip:** Friendly name visible to UPnP/DLNA players.

Example:

```text
foobar2000 SACD DSD
```

A descriptive name is useful when several servers exist on the same LAN.

### HTTP port

**Tooltip:** TCP port used to deliver the HTTP media stream and UPnP XML.

Default:

```text
8192
```

Use another port only when it conflicts with another service.

If you change the port, the Windows firewall may need a corresponding rule.

### Enable stability mode

**Tooltip:** Separates SACD decoding/cache work from network delivery to absorb short disk or network fluctuations.

The design intentionally avoids using PCM as a "compression" shortcut.

Stability Mode is useful when:

- another computer is copying files;
- Wi-Fi briefly slows down;
- a NAS/HDD has occasional latency spikes;
- the network has short congestion bursts.

It cannot fix a link whose sustained throughput is below the required DSD bitrate.

### Pre-buffer (seconds)

**Tooltip:** Amount of DSD read-ahead to prepare before/while delivering a track.

Allowed range in the current alpha UI:

```text
5–60 seconds
```

Default:

```text
15 seconds
```

For DSD256, 15 seconds corresponds to roughly 42.3 MB of raw stereo DSD payload.

Practical examples:

**Stable Gigabit Ethernet**

```text
Stability Mode: ON
Pre-buffer: 10–15 s
```

**Busy home network**

```text
Stability Mode: ON
Pre-buffer: 20–30 s
```

**Very unstable wireless network**

```text
Stability Mode: ON
Pre-buffer: 30–60 s
```

A larger buffer consumes more storage/RAM/cache and does not compensate for a permanently insufficient connection.

---

## 5. Live status

The status area is intentionally split into discovery, decoder, library and real audio transfer.

### DLNA

Example:

```text
DLNA: BROADCASTING / ACTIVE
```

Means the server is running and advertising itself.

### foo_input_sacd

Example:

```text
foo_input_sacd: INSTALLED (2.0.25)
```

Means the SACD decoder component has been detected.

If it says:

```text
NOT INSTALLED
```

SACD ISO playback through `foo_sacd_dlna` cannot be prepared through the intended decoder path.

### Music Library

Example:

```text
Music Library: SHARING (245 DSD tracks)
```

This means the configured library is being exposed through the DLNA ContentDirectory.

### Audio stream

Example:

```text
Audio stream: ACTIVE / TRANSMITTING   TX 22.8 Mbit/s
```

This is the most important indicator when you want to know whether the SDX is actually receiving audio.

`IDLE` means the server is available but no active media transfer is detected.

### T+A SDX

Possible states:

```text
T+A SDX: NOT DETECTED
T+A SDX: DETECTED / IDLE
T+A SDX: DETECTED / STREAMING
```

The detection comes from UPnP/SSDP identity information when the renderer exposes enough information to identify itself.

When the active client IP matches the detected T+A renderer, the state can be shown as `DETECTED / STREAMING`.

The component cannot reproduce the physical OLED/GUI of the T+A; it can show network identity and streaming state.

### DSD cache/buffer

Examples:

```text
DSD cache/buffer: READY
```

```text
DSD cache/buffer: READ-AHEAD 42.3 MB
```

```text
DSD cache/buffer: SACD→DSD CONVERTING 63%
```

This status is intended to make it clear whether the component is preparing DSD or already delivering it.

---

## 6. Examples

### Example A — local DSF library

You already have:

```text
D:\Music\SACD\Dire Straits\Brothers In Arms\01 - So Far Away.dsf
```

In foobar2000:

1. Add the folder to Media Library.
2. Enable **Share DSD content from foobar2000 Music Library**.
3. Enable DLNA.
4. On the T+A, open the UPnP/DLNA server.
5. Browse Artist → Album → Track.
6. Play the DSF.

No SACD ISO conversion is necessary because the source is already DSF.

---

### Example B — SACD ISO

You have:

```text
D:\SACD\Album.iso
```

and `foo_input_sacd` is installed.

The intended flow is:

```text
Album.iso
   ↓
foo_input_sacd
   ↓
DSD
   ↓
DSF cache
   ↓
DLNA
   ↓
SDX 3100 HV
```

The first request can take longer because the DSD cache needs to be prepared.

After the DSF is cached, subsequent playback of the same cached item does not need the same initial decode work.

---

### Example C — checking whether the SDX is actually streaming

Do not use only:

```text
DLNA: BROADCASTING / ACTIVE
```

That only proves that the server is available.

Instead look for:

```text
Audio stream: ACTIVE / TRANSMITTING
T+A SDX: DETECTED / STREAMING
TX 22.x Mbit/s
DSD256
```

That combination means the server has an active HTTP media transfer and the detected T+A IP is the active renderer.

---

### Example D — congested network

Suppose another computer is copying a large file over Wi-Fi.

For DSD256:

```text
Stability Mode: ON
Pre-buffer: 30 s
```

The component can prepare enough DSF/cache data to survive short throughput drops.

The preferred network remains wired Gigabit Ethernet:

```text
PC → Gigabit switch → T+A SDX
```

The buffer is an additional protection mechanism, not a replacement for sufficient network capacity.

---

## 7. Understanding the TX speed

The `TX` value is the measured server-to-client TCP transmit rate.

For stereo DSD, approximate payload requirements are:

| Format | Approx. payload |
|---|---:|
| DSD64 | 5.64 Mbit/s |
| DSD128 | 11.29 Mbit/s |
| DSD256 | 22.58 Mbit/s |

Protocol overhead is additional.

A healthy DSD256 transfer might therefore show a TX rate around the low-to-mid 20 Mbit/s range depending on implementation and measurement window.

The important comparison is:

```text
actual TX capacity
        vs.
required DSD bitrate
```

The status view also exposes network headroom where available.

---

## 8. Network recommendations

### Minimum practical

- Windows 64-bit PC
- foobar2000 x64
- 4 GB RAM
- Ethernet 100 Mbps
- SSD preferred for DSF cache

### Recommended

- 8 GB RAM or more
- SSD
- Gigabit Ethernet
- PC and SDX on the same LAN/switch
- wired connection for both whenever possible

For DSD256, Gigabit Ethernet is recommended because it leaves substantial headroom for normal LAN traffic.

---

## 9. Troubleshooting

### The SDX does not see the server

Check:

1. DLNA is enabled.
2. Windows Firewall permits the foobar2000 process.
3. UDP SSDP multicast is not blocked.
4. PC and SDX are on the same LAN/VLAN.
5. The configured HTTP port is not already in use.

### The server appears but tracks do not play

Check:

1. `Audio stream` changes to `ACTIVE / TRANSMITTING`.
2. The T+A IP shown by the status panel is the expected renderer.
3. The track is DSF/DFF or a supported SACD ISO.
4. `foo_input_sacd` is installed for ISO content.
5. The T+A firmware accepts the advertised DSF/DFF profile.

### Playback stops on DSD256

Check the TX rate and the network.

If the network cannot sustain more than the required bitrate for extended periods, increasing the buffer alone cannot guarantee continuous playback.

Prefer:

```text
Gigabit Ethernet
+
Stability Mode
+
15–30 s pre-buffer
```

### `foo_input_sacd` says NOT INSTALLED

Install the Super Audio CD Decoder component, restart foobar2000 if necessary, and reopen Preferences.

---

## 10. Why the component has separate "broadcasting" and "transmitting" states

UPnP/DLNA uses SSDP for discovery and HTTP for media delivery.

That means:

```text
SSDP
  ↓
"Here is my Media Server"
```

is not the same operation as:

```text
HTTP
  ↓
"Here are the DSD audio bytes you requested"
```

Keeping these states separate makes the UI useful for diagnostics rather than merely showing that the plugin is switched on.

---

## 11. Alpha status

This project is an alpha development build.

The following still requires testing against the exact T+A SDX 3100 HV firmware and a real network:

- renderer-specific DLNA profile requirements;
- exact DSF/DFF MIME/protocolInfo acceptance;
- gapless behaviour;
- album-art behaviour;
- seeking/range behaviour;
- long-duration DSD256 playback;
- network congestion recovery.

No claim of guaranteed compatibility should be made until those tests have been completed.
