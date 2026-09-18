# foo_sacd_dlna Examples

## Example 1 — DSD256 over Gigabit Ethernet

```text
Windows PC
  └─ foobar2000
      └─ foo_sacd_dlna
             │
        Gigabit Ethernet
             │
          Switch
             │
        Gigabit Ethernet
             │
      T+A SDX 3100 HV
```

Settings:

```text
Enable DLNA broadcasting: ON
Share DSD content:        ON
Stability Mode:           ON
Pre-buffer:               15 seconds
```

Expected status while playing:

```text
DLNA discovery: BROADCASTING / ACTIVE
Audio stream: ACTIVE / TRANSMITTING
T+A SDX: DETECTED / STREAMING
DSD: DSD256
TX: ~22–24 Mbit/s
```

---

## Example 2 — SACD ISO

Source:

```text
D:\SACD\Pink Floyd\The Dark Side of the Moon.iso
```

Prerequisite:

```text
foo_input_sacd.dll = installed
```

The first playback request may prepare a DSF cache:

```text
SACD ISO → foo_input_sacd → DSD → DSF cache → HTTP → T+A
```

The original ISO remains untouched.

---

## Example 3 — busy network

Settings:

```text
Stability Mode: ON
Pre-buffer: 30 seconds
```

Preferred transport:

```text
PC ──Ethernet── Switch ──Ethernet── SDX
```

The 30-second read-ahead is designed to absorb short interruptions. It does not increase the physical bandwidth of the network.

---

## Example 4 — diagnosing "it is visible but does not play"

Possible UI:

```text
DLNA: BROADCASTING / ACTIVE
foo_input_sacd: INSTALLED (2.0.25)
Music Library: SHARING (148 DSD tracks)
Audio stream: IDLE
T+A SDX: DETECTED / IDLE
```

Interpretation:

The server is alive and the SDX is visible, but no audio bytes are currently being downloaded.

While playing:

```text
Audio stream: ACTIVE / TRANSMITTING
T+A SDX: DETECTED / STREAMING
TX: 11.4 Mbit/s
DSD128
```

Interpretation:

The renderer is actively requesting the media.

---

## Example 5 — no `foo_input_sacd`

```text
foo_input_sacd: NOT INSTALLED
```

For existing DSF/DFF files this component can still serve compatible native-DSD files, but SACD ISO decoding cannot use the intended external SACD decoder path.

Install `foo_input_sacd` before using SACD ISO sources.
