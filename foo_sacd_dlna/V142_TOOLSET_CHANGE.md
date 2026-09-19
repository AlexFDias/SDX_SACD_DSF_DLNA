# v142 Toolset Change

This patch changes only the `foo_sacd_dlna` project toolset from `v143` to `v142`.

The reason is that the foobar2000 SDK project environment being used for this build is based on v142, and the build log showed the project failing before C++ compilation because the required ATL header `atlbase.h` was unavailable.

## Files changed

- `foo_sacd_dlna.vcxproj` — `PlatformToolset` changed from `v143` to `v142` for Debug x64 and Release x64.
- `BUILD.md` — added the v142/MSVC/ATL requirements and clean rebuild steps.
- `README.md` — added a short note about the v142 toolset.
- `README.pt-PT.md` — added the equivalent Portuguese note.

## Visual Studio requirement

Install the Visual Studio 2022 components that provide the v142 compiler and ATL support. The project should then be built as `Debug | x64` or `Release | x64`.
