# TeenAstroAscomNative

Native Windows DLL that wraps TeenAstro LX200 protocol and MountState for the ASCOM driver.

## Build

From the TeenAstro repository root:

```
pio run -d TeenAstroAscomNative
```

Output: `TeenAstroAscomNative/.pio/build/windows_x86/TeenAstroAscomNative.dll`

The ASCOM project copies this DLL to its output directory when building (if the DLL exists).

## C API

- `TeenAstroAscom_ConnectSerial(port)` / `TeenAstroAscom_ConnectTcp(ip, port)` — returns handle
- `TeenAstroAscom_Disconnect(handle)`
- `TeenAstroAscom_CommandBlind(handle, cmd, raw)` — raw=0 adds `:` and `#`
- `TeenAstroAscom_CommandBool(handle, cmd, raw)`
- `TeenAstroAscom_CommandString(handle, cmd, raw, outBuf, outBufSize)`
- `TeenAstroAscom_GetMountState(handle, &GXASState)` — parsed GXAS state

## Architecture

Reuses C++ libraries:
- LX200Client (with IX200Transport)
- TeenAstroMountStatus
- TeenAstroCommandDef
- TeenAstroMath

Win32 transports: Serial (CreateFile/ReadFile/WriteFile), TCP (Winsock2).
