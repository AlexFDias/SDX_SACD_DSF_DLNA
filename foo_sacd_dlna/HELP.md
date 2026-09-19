# foo_sacd_dlna v0.7 Alpha 3 Help

## Purpose

`foo_sacd_dlna` exposes the DSD part of the foobar2000 Music Library as a UPnP/DLNA Media Server for compatible network players such as the T+A SDX 3100 HV.

## Requirements

- foobar2000 x64
- Super Audio CD Decoder (`foo_input_sacd.dll`)
- A network connection shared with the DLNA player

## DSD policy

The component is DSD-only. It never converts DSD to PCM and does not send DoP to the network player.

Supported native DSD families for the SACD path:

- DSD64
- DSD128
- DSD256

For an SACD ISO, the installed SACD decoder is used through foobar2000's public decoder interface with `input_flag_dop`. The DoP container is removed and the resulting DSD bits are stored in DSF without PCM conversion.

## Music Library

Enable **Share DSD content from foobar2000 Music Library**. The server creates this tree and supports both `BrowseDirectChildren` and `BrowseMetadata`:

`Artists > Artist > Album > Track`

The source files are not copied into the Music Library. The component keeps a reference to the foobar2000 media item. Album art is requested through foobar2000's album-art manager when the player asks for it.

## Broadcasting status

`BROADCASTING / ACTIVE` means the HTTP server and SSDP discovery threads are running. The same state is visible in Preferences and the optional `SACD DLNA Status` UI element.

## SACD ISO behaviour

SACD ISO tracks are decoded through the installed `foo_input_sacd` decoder and stored in the persistent DSF cache on first HTTP request. This alpha release therefore provides transparent on-demand playback rather than a permanent conversion of the library.

Native `.dsf` and `.dff` files are served directly and support HTTP Range requests.

## T+A SDX 3100 HV

The current T+A documentation describes UPnP/DLNA streaming and DSD operation; exact renderer acceptance of DSF/DFF MIME/profile combinations should be validated on the firmware installed on the target unit.

## Live monitoring

The status panel distinguishes discovery from actual audio transfer:

- `DLNA discovery: BROADCASTING / ACTIVE` means the server is advertising itself through SSDP.
- `Audio stream: ACTIVE / TRANSMITTING` means a renderer is actively downloading the audio payload over HTTP.
- `TX` is the measured TCP transmit rate from the server to the renderer.
- The client IP identifies the machine/device currently requesting the media.
- The T+A SDX indicator is populated from UPnP/SSDP discovery when the renderer identifies itself as T+A/SDX. The active stream is shown as `DETECTED / STREAMING` when its IP matches the active renderer.

The actual audio stream is unicast HTTP; SSDP is only for UPnP/DLNA discovery and announcement.

## Stability Mode (V0.7)

Stability Mode decouples SACD ISO → DSD conversion from the network delivery path. ISO tracks are converted to a persistent DSF cache and transmission starts only after the DSD file is ready. A configurable 5–60 second read-ahead and a larger TCP send buffer can be used before transmission.

This is useful for short disk/network fluctuations. No server-side buffer can guarantee uninterrupted playback when sustained network throughput is below the bitrate required by the selected DSD rate.

Approximate stereo payload rates: DSD64 = 5.64 Mbit/s; DSD128 = 11.29 Mbit/s; DSD256 = 22.58 Mbit/s.


## 12. Network log file

When **Verbose network logging** is enabled, the component writes the same diagnostic events shown in the foobar2000 Console to:

```text
<foobar2000 profile>\foo_sacd_dlna\network.log
```

The log includes UTC timestamps and is useful when diagnosing renderer discovery, `Browse`/`BrowseMetadata`, protocol negotiation, HTTP range requests, cache generation and stream termination.

Disable verbose logging during normal playback if no diagnostics are needed.


## 12. Renderer protocol negotiation

The server does not blindly assume one MIME type for every renderer.

When a T+A renderer is discovered, the component reads its UPnP `ConnectionManager` and requests `GetProtocolInfo`. For DSF it prefers a MIME type explicitly advertised by the renderer, falling back to the server's DSD MIME when the renderer does not expose that information.

The live status therefore shows a negotiated sink protocol when available.

No claim of a T+A firmware-specific MIME requirement is made until it has been observed on the exact firmware in use.

## 13. Gapless playback

The server exposes exact track duration, original track number and album/artist relationships, and supports byte-range requests. This is the server-side part of a gapless-capable design.

Gapless transition itself remains renderer-dependent because a MediaServer does not control the renderer's internal queue/decoder scheduling. The test plan therefore measures whether the SDX requests the next track early enough and whether there is an audible/transport interruption.

## 14. Persistent cache invalidation

SACD-to-DSF cache entries include:

- cache format version;
- source path identity;
- source file size;
- source write time;
- SACD decoder version;
- subsong number;
- generated DSD rate and sample count.

Changing the source file, changing its timestamp, changing the decoder version or changing the cache format creates a different cache key. Old entries are not silently used for the changed source.

The **Clear DSF Cache** button removes generated DSF files, manifests, partial files and cached artwork. It never deletes source music.

## 15. Media Library tracking

The component registers the foobar2000 Media Library callback interface for:

- library initialization;
- item addition;
- item removal;
- item modification.

Changes are debounced into a safe library refresh instead of rebuilding the server list once for every callback event.

## 16. Diagnostics

Verbose network logging records:

- SSDP discovery and announcements;
- renderer description and identity;
- ConnectionManager negotiation;
- Browse/BrowseMetadata requests;
- HTTP media requests and Range headers;
- cache hits/misses;
- transfer start/stop;
- actual TX bitrate;
- errors and cancellations.

The web status page at `/status` exposes the live state without modifying the library.


## 17. Cache files

The persistent cache is stored under the foobar2000 profile:

```text
<foobar2000 profile>\foo_sacd_dlna\cache\
```

SACD ISO entries have a generated DSF plus a small manifest. The manifest is not music metadata intended for the renderer; it is an integrity/invalidation record used to decide whether an old DSF can safely be reused.

The cache is replace-on-success: a `.partial` file is written while converting, then renamed into place only after the DSF passes structural validation.


## 18. DIDL-Lite bitrate units

The `res@bitrate` field is advertised in **bytes per second**, as required by the UPnP ContentDirectory definition. The live `TX` indicator is shown in **Mbit/s** for easier network diagnostics.
\n\n## 19. DSF cache integrity\n\nBefore a generated SACD ISO cache entry is accepted, the component validates the DSF structure, including:\n\n- DSD chunk identifier and size;\n- advertised file size;\n- `fmt ` chunk;\n- stereo channel layout;\n- 1-bit sample format;\n- 4096-byte DSF block size;\n- DSD64/DSD128/DSD256 sample rate;\n- non-zero sample count;\n- `data` chunk.\n\nThe decoder version and source file size/write time are also part of cache identity.\n