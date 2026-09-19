# foo_sacd_dlna — Roadmap

Current version: **0.8-alpha1** (see `VERSION`, `CHANGELOG.md`).

This file is the single source of truth for project status and planned work.
`README.md` and `README.pt-PT.md` link here instead of keeping their own lists.

Status legend: **DONE** (in the tree) · **PARTIAL** (implemented, incomplete or
unvalidated) · **PLANNED** (not implemented) · **BLOCKED**.

---

## 0. Current blocker — the component does not build

`Debug/foo_sacd_dlna.log`:

```text
foobar2000\helpers\foobar2000-lite+atl.h(15,10): fatal error C1083:
cannot open include file: 'atlapp.h': No such file or directory
```

`atlapp.h` is a **WTL** header, not an ATL header. It does not ship with Visual
Studio and is not installed by any "C++ ATL" component. The SDK helpers and
`libPPUI` require WTL to be on the include path.

| # | Item | Status |
|---|------|--------|
| 0.1 | Obtain WTL 10.x and place it where the project now expects it (`../../WTL/Include`, already added to `AdditionalIncludeDirectories` for both configurations). `libPPUI` needs the same path. | **BLOCKED — do this first** |
| 0.2 | Correct the diagnosis in `BUILD.md` and `V142_TOOLSET_CHANGE.md`, which attributed the failure to a missing v142 ATL component | **DONE (0.8-alpha2)** |
| 0.3 | Re-evaluate the `v143 → v142` toolset downgrade once WTL is present; it was applied for a cause that turned out to be misidentified | **PLANNED** |
| 0.4 | Full clean `Debug|x64` and `Release|x64` rebuild, then record the first successful build in `CHANGELOG.md` | **PLANNED** |

Nothing below can be validated until 0.1 is resolved. No item in this file has
ever been executed on hardware.

---

## 1. Delivered (0.5 → 0.8-alpha1)

Verified present in the source tree:

| Item | Where | Status |
|------|-------|--------|
| SSDP discovery, device/service descriptions, HTTP media delivery | `dlna_server.cpp` | **DONE** |
| `Browse` with `BrowseDirectChildren` + `BrowseMetadata` | `browseDidl`, `browseResponse` | **DONE** |
| Pagination (`StartingIndex` / `RequestedCount` / `NumberReturned` / `TotalMatches`) | `browseDidl` | **DONE** |
| `GetSystemUpdateID` + Media Library change callbacks | `library_tracker` | **DONE** |
| DIDL-Lite metadata with artist roles and `albumArtURI` | `appendTrack` | **DONE** |
| Persistent artwork cache with source invalidation | `dlna_server.cpp` | **DONE** |
| Persistent SACD→DSF cache with manifest and source/decoder/version invalidation | `dsf_writer.*`, `.partial` staging | **DONE** |
| Bounded concurrency (`kMaxConcurrentStreams = 2`) with `abort_callback` cancellation and `.partial` cleanup | `dlna_server.cpp` | **DONE** |
| HTTP `Range` support | `serveMedia` | **DONE** |
| Stability Mode: read-ahead + enlarged TCP send buffer | `config.*`, `preferences.cpp` | **DONE** |
| Live status: TX rate, DSD rate, client IP, T+A detection, cache size | `status.h`, `ui_element.cpp` | **DONE** |
| Console + timestamped `network.log` diagnostics | `dlna_server.cpp` | **DONE** |
| Optional `foo_dsd_processor` integration with preset fingerprint in cache invalidation | `dsp_bridge.*` | **DONE** |
| `Clear DSF Cache` action | `preferences.cpp` | **DONE** |
| Diagnostic tooling | `tools/*.py` | **DONE** |

These items were previously still listed as *planned* in both READMEs while
`CHANGELOG.md` recorded them as shipped in 0.7-alpha2/alpha3. That contradiction
is the reason this file exists.

---

## 2. Implemented but not validated

| Item | Note | Status |
|------|------|--------|
| Renderer-aware `protocolInfo` negotiation | `ConnectionManager::GetProtocolInfo` is queried and a negotiated Sink is stored, but no real renderer response has ever been parsed | **PARTIAL** |
| DSD64 / DSD128 / DSD256 delivery | never played on hardware | **PARTIAL** |
| SACD ISO decode via `foo_input_sacd` | never run | **PARTIAL** |
| DSD Processor path (PCM→DSD, DSD256→DSD128) | never run; native-DSD output check is untested | **PARTIAL** |
| T+A SDX 3100 HV detection | logic exists, unit never present | **PARTIAL** |

Every row here maps to an unchecked box in `RELEASE_CHECKLIST.md`.

---

## 3. Planned — protocol completeness

| Item | Status |
|------|--------|
| `ConnectionManager` handler currently answers **any** POST to `/ctl/ConnectionManager` with a fixed `GetProtocolInfoResponse`; dispatch on SOAPACTION and add `GetCurrentConnectionIDs` / `GetCurrentConnectionInfo` | **PLANNED** |
| `GetSortCapabilities` and `GetSearchCapabilities` (queried by several renderers during setup) | **PLANNED** |
| `X_GetFeatureList` (DLNA feature list) | **PLANNED** |
| Advertised `Source` `protocolInfo` is static; derive it from what the server can actually serve | **PLANNED** |
| Root container exposes only `Artists`; add `Albums`, `All Tracks`, and optionally `Folders` | **PLANNED** |
| `SortCriteria` is accepted and ignored | **PLANNED** |
| `Filter` is accepted and ignored; honour it or always return the full property set deliberately | **PLANNED** |
| Deterministic track order and reliable duration metadata (prerequisite for gapless testing) | **PLANNED** |
| Additional artwork formats and sizes | **PLANNED** |

---

## 4. Planned — build, packaging, release

| Item | Status |
|------|--------|
| `.github/workflows/` does not exist in the tree, although `CHANGELOG.md` 0.7-alpha2 and `README.md` describe a Windows GitHub Actions workflow; either add it or remove the claim | **PLANNED** |
| Automated packaging of `foo_sacd_dlna.fb2k-component` | **PLANNED** |
| Reproducible build instructions including the WTL dependency | **PLANNED** |
| Pin the SDK snapshot used for release builds | **PLANNED** |

---

## 5. Planned — hardware validation (gated on section 0)

Executed against `HARDWARE_VALIDATION.md` and `RELEASE_CHECKLIST.md`:

| Item | Status |
|------|--------|
| Record the exact SDX 3100 HV firmware version under test | **PLANNED** |
| Capture the real `ConnectionManager` Sink `protocolInfo` from the renderer | **PLANNED** |
| DSF DSD64 / DSD128 / DSD256 playback | **PLANNED** |
| SACD ISO playback through `foo_input_sacd` | **PLANNED** |
| 30+ minute sustained DSD256 transfer | **PLANNED** |
| Gapless behaviour recorded as PASS/FAIL — **renderer/firmware dependent, never assumed** | **PLANNED** |
| Confirm no DSD→PCM conversion anywhere in the network path | **PLANNED** |
| Windows Firewall guidance verified on a clean machine | **PLANNED** |

---

## 6. Version targets

| Version | Scope |
|---------|-------|
| 0.8-alpha2 | Section 0 only: WTL dependency documented and building, corrected build docs, first clean `Release|x64` |
| 0.9-alpha1 | Section 3 protocol completeness; CI/packaging from section 4 |
| 0.9-beta | Section 5 executed on the real SDX 3100 HV; `RELEASE_CHECKLIST.md` fully ticked |
| 1.0 | Beta stable on the validated firmware, with gapless documented as PASS or FAIL rather than pending |

---

## 7. Known documentation defects

Not roadmap work, but they should be fixed alongside it:

- Read-ahead range is stated as **5–60 s** in `README.md` and `README.pt-PT.md`
  and as **5–120 s** in `CHANGELOG.md`. One of the two is wrong.
- `README.pt-PT.md` labels Stability Mode and live monitoring as **V0.7**,
  `README.md` labels the same sections **V0.5** / **V0.7**, and `CHANGELOG.md`
  introduces Stability Mode under **0.6 Alpha 1**.
- `RELEASE_NOTES_0.6.md` is missing; 0.5, 0.7 and 0.8 exist.
- `README.pt-PT.md` still carries the "Current Alpha roadmap" section in English.
