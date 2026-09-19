# v142 Toolset Change

This patch changed the `foo_sacd_dlna` project toolset from `v143` to `v142`,
to match the foobar2000 SDK project tree used for this build.

## Correction

The change was originally justified by a build failure attributed to a missing
ATL header. That diagnosis was wrong. The failure that blocks this project is:

```text
foobar2000\helpers\foobar2000-lite+atl.h(15,10): fatal error C1083:
cannot open include file: 'atlapp.h': No such file or directory
```

`atlapp.h` is a **WTL** header. It is not part of ATL, it does not ship with
Visual Studio, and no "C++ ATL" component installs it. Changing the platform
toolset has no effect on it.

The actual fix is to obtain WTL and add its `Include` directory to the include
path for both configurations, and for `libPPUI`, which has the same dependency.
See the WTL section in [`BUILD.md`](BUILD.md).

## Status of the toolset change itself

Keeping v142 is still defensible: the SDK tree in use was built with v142, and
mixing toolsets across the referenced projects is best avoided. But v142 was not
what unblocked the build, and if the SDK projects are rebuilt with v143 the
component can follow.

## Files changed

- `foo_sacd_dlna.vcxproj` — `PlatformToolset` set to `v142` for Debug x64 and Release x64.
- `BUILD.md` — v142/MSVC requirements, WTL dependency, clean rebuild steps.
- `README.md` / `README.pt-PT.md` — short note about the toolset.

## Visual Studio requirement

Install the Visual Studio 2022 components providing the v142 compiler and ATL
support, then add WTL as described in `BUILD.md`. Build as `Debug | x64` or
`Release | x64`.
