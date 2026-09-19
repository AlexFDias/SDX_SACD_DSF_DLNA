# foo_sacd_dlna 0.8 Alpha 1

## Optional DSD Processor integration

This release adds optional integration with the installed `foo_dsd_processor` DSP component.

### Default

Native DSD bypass remains the default:

```text
DSD → DSF → DLNA → renderer
```

### DSP enabled

```text
PCM → DSD Processor → DSD → DSF → DLNA
DSD → DSD Processor → DSD → DSF → DLNA
```

The private preset is kept by `foo_sacd_dlna`; the user's normal foobar2000 playback DSP configuration is not changed.

### Bandwidth example

A user may configure:

```text
DSD256 → DSD Processor → DSD128 → DSF → DLNA
```

This reduces sustained native-DSD network payload by approximately 50% while keeping the network path DSD. It is not claimed to be sonically identical to the source DSD256.

### Dependency

`foo_dsd_processor` is required only when the option is enabled. `foo_input_sacd` remains required for SACD ISO handling and is still validated by the server.

### Validation status

This alpha still requires compilation on Windows with MSVC and hardware testing against the exact T+A SDX 3100 HV firmware in use.

## Build toolchain note

This repository uses **MSVC v142** with the WTL headers from the SDK tree at:

```text
D:\SDX_SACD_DSF_DLNA\SDK-2025-03-07\WTL\include
```

See `BUILD.md`, `WTL_SETUP.md` and `V142_WTL_FIX.md` for the complete configuration.
