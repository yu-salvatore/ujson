# uJSON

Ultra-lightweight JSON parser designed for low-power, resource-constrained IoT devices.

Built from hands-on experience developing firmware for small embedded systems — where every byte of RAM matters and `malloc` is not an option.

## Why uJSON

- **6 bytes per token**
- **Single header** — just `#include "ujson.h"`
- **Zero dependencies** — no stdlib, no heap allocation
- **Single-pass parsing** — fully inlined for speed
- **Path query** — access nested values with `"config.wifi.ssid"` syntax

## Quick Start

```c
#include "ujson.h"

const char *json = "{\"name\":\"test\",\"value\":42}";

uj_t p;
uj_tok tk[32];

uj_init(&p);
int n = uj_parse(&p, json, strlen(json), tk, 32);

if (n > 0)
{
    uj_val v = uj_get(json, tk, n, "value");
    if (v.ok)
    {
        int val = uj_v_int(&v);  // 42
    }
}
```

## Path Query

```c
uj_get(json, tk, n, "key");           // Object key
uj_get(json, tk, n, "obj.nested");    // Nested object
uj_get(json, tk, n, "arr[0]");        // Array index
uj_get(json, tk, n, "obj.arr[2].x");  // Mixed path
```

## API

### Parser

| Function | Description |
|----------|-------------|
| `uj_init(&p)` | Initialize parser |
| `uj_parse(&p, json, len, tk, ntk)` | Parse JSON, returns token count or error |

### Value Extraction

| Function | Description |
|----------|-------------|
| `uj_v_int(&v)` | Get integer |
| `uj_v_bool(&v)` | Get bool (1/0/-1) |
| `uj_v_null(&v)` | Check if null |
| `uj_v_str(&v, buf, sz)` | Copy string to buffer |
| `uj_v_len(&v)` | String length |
| `uj_v_size(&v)` | Array/object size |
| `uj_v_ptr(&v)` | Raw string pointer |
| `uj_v_eq(&v, "str")` | Compare string |

### Error Codes

| Code | Value | Meaning |
|------|-------|---------|
| `UJ_OK` | 0 | Success |
| `UJ_ENOMEM` | -1 | Not enough tokens |
| `UJ_EINVAL` | -2 | Invalid JSON |
| `UJ_EPART` | -3 | Incomplete JSON |

## Token Structure

```
6 bytes per token (default)
8 bytes per token (with UJ_PARENT_LINKS)

┌──────────┬──────────┬──────┬──────┐
│  start   │   len    │ size │ _pad │
│ 2 bytes  │ 2 bytes  │  1B  │  1B  │
└──────────┴──────────┴──────┴──────┘
             ├─ 14-bit length
             └─ 2-bit type
```

## Configuration

### Parent Links (optional)

```c
#define UJ_PARENT_LINKS
#include "ujson.h"

// Token becomes 8 bytes, enables upward traversal
int parent_idx = uj_parent(&tk[i]);
uj_val parent = uj_v_parent(&v, tk);
```

## Limits

| Item | Limit |
|------|-------|
| JSON size | 64 KB |
| Token length | 16 KB |
| Children | 255 |
| Tokens (with parent) | 32K |

## License

MIT
