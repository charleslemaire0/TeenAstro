# TeenAstroCommandDef

Shared command definitions for the TeenAstro LX200 protocol. This library is consumed by both:

- **TeenAstroMainUnit** (server) — the telescope controller firmware
- **TeenAstroLX200io / LX200Client** (client) — the serial command client

Having a single source of truth for enums, types, wire metadata, and encoding/decoding eliminates protocol drift between client and server.

## Contents

| Header | Purpose |
|--------|---------|
| `TeenAstroCommandDef.h` | Umbrella header (includes everything) |
| `CommandEnums.h` | `LX200RETURN`, `CMDREPLY`, `ErrorsGoTo`, `EncoderSync` |
| `CommandTypes.h` | Structs for typed command parameters and responses |
| `CommandMeta.h` | `Cmd::` namespace (lead character constants), `getReplyType()` function for wire format routing |
| `CommandCodec.h` / `.cpp` | All value encoding/decoding: HMS/DMS/date parsing and formatting |

## Usage

```cpp
#include <TeenAstroCommandDef.h>   // full set
// or pick what you need:
#include <CommandEnums.h>
#include <CommandCodec.h>
```

PlatformIO auto-discovers the library when it's under `libraries/`.
