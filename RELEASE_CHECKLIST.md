# Release Checklist

Before a public release, verify on a Windows machine with the target foobar2000 build:

- [ ] Visual Studio Release x64 builds cleanly with the pinned SDK
- [ ] `foo_input_sacd` is detected
- [ ] DSF DSD64 plays
- [ ] DSF DSD128 plays
- [ ] DSF DSD256 plays
- [ ] SACD ISO is decoded through the installed `foo_input_sacd`
- [ ] Root / Artist / Album / Track Browse works
- [ ] BrowseMetadata works
- [ ] album art loads
- [ ] HTTP Range works
- [ ] live TX rate is displayed
- [ ] T+A SDX is detected correctly
- [ ] ConnectionManager capability negotiation is recorded
- [ ] no DSD→PCM conversion occurs in the network path
- [ ] cache invalidates after source/decoder changes
- [ ] cancellation leaves no `.partial` cache artifacts
- [ ] Media Library add/remove/modify callbacks refresh the server
- [ ] Windows firewall guidance tested
- [ ] exact SDX firmware recorded in `HARDWARE_VALIDATION.md`
- [ ] 30+ minute DSD256 test completed
- [ ] gapless behaviour recorded as PASS/FAIL, never assumed
