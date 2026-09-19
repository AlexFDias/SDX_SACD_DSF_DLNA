# Network Requirements

## Recommended setup

Use wired Gigabit Ethernet whenever possible:

```text
Windows PC / foobar2000
          │
       1 GbE
          │
       Switch
          │
       1 GbE
          │
T+A SDX 3100 HV
```

Native stereo DSD payload rates are approximately:

- DSD64: 5.64 Mbit/s
- DSD128: 11.29 Mbit/s
- DSD256: 22.58 Mbit/s

100 Mbps Ethernet can carry these rates in theory, but Gigabit Ethernet is recommended for headroom and network congestion.

The SDX 3100 HV provides 10/100/1000 Base-T Ethernet and Wi-Fi. T+A documents DFF/DSF and DSD64/DSD128/DSD256 for the Streaming Client.

## Stability Mode

Use Stability Mode on busy networks. The default 15-second read-ahead provides approximately:

- DSD64: 10.6 MB
- DSD128: 21.2 MB
- DSD256: 42.3 MB

These are approximate raw stereo DSD payload values.

Stability Mode is designed for short interruptions and throughput fluctuations. It cannot overcome a sustained link slower than the required bitrate.

## Wi-Fi

5 GHz Wi-Fi can work, but wired Ethernet is preferred for predictable latency and reduced susceptibility to interference.

2.4 GHz Wi-Fi should not be the preferred transport for DSD256.

## Firewall

Allow the foobar2000 application/component to accept:

- TCP: the configured HTTP/DLNA port (default 8192)
- UDP: SSDP multicast 239.255.255.250:1900

The PC and T+A should normally be on the same LAN/VLAN for SSDP discovery.

## Build toolchain note

This repository uses **MSVC v142** with the WTL headers from the SDK tree at:

```text
D:\SDX_SACD_DSF_DLNA\SDK-2025-03-07\WTL\include
```

See `BUILD.md`, `WTL_SETUP.md` and `V142_WTL_FIX.md` for the complete configuration.
