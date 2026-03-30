# TeenAstro ASCOM driver verification (Conform-like)

This folder builds **TeenAstroASCOM_V7_TcpConnectivityTest.exe**, a CLI that runs structured checks against the real TeenAstro ASCOM telescope driver (same code paths as the installed driver when using in-process mode).

The core logic lives in the **TeenAstroASCOM_V7.DriverVerification** class library: phases (profile, precheck, connect, metadata, `Can*`, `DeviceState`, poll stability, optional rates, ITelescopeV4 property enumeration), `VerificationReport`, and JUnit/JSON export.

**ASCOM Conform** remains the official certification tool; this harness is for developer/CI smoke tests.

## Build

```text
dotnet build TeenAstroASCOM_V7_TcpConnectivityTest/TeenAstroASCOM_V7_TcpConnectivityTest.csproj -c Release
```

Output: `TeenAstroASCOM_V7_TcpConnectivityTest\bin\Release\net472\TeenAstroASCOM_V7_TcpConnectivityTest.exe` (and dependencies, including `TeenAstroASCOM_V7.DriverVerification.dll` and `ASCOM.TeenAstro.exe`).

## Run against hardware (IP example)

```text
dotnet run --project TeenAstroASCOM_V7_TcpConnectivityTest/TeenAstroASCOM_V7_TcpConnectivityTest.csproj -c Release -- --ip 192.168.1.17 --runs 5 --report-junit out.xml --report-json out.json
```

## Exit codes

- **0** – no checks with outcome `Issue`
- **1** – at least one `Issue`
- **2** – fatal exception before a normal report

## CI

- Gate on exit code `0` after a Release build.
- Publish `out.xml` (JUnit) or `out.json` as a build artifact for dashboards.

Optional **COM ProgID** mode (closer to Conform’s native COM hosting): add `--com`.
