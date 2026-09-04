# SSMEL SDK

封闭二进制音源引擎。解压本 zip 即可使用，**只需**本目录里的头文件和 `libssmel`。

本包版本见 `VERSION`。升级说明见 [CHANGELOG.md](CHANGELOG.md)。完整契约与 API 见 [INTEGRATION.md](INTEGRATION.md)。

## 目录

```
ssmel-sdk-<version>/
  README.md                 本文件
  CHANGELOG.md              各版本升级
  INTEGRATION.md            集成指南
  VERSION                   包版本 + ABI
  include/ssmel/            公开头文件
  lib/libssmel.so           引擎（Linux）
  lib/cmake/SSMEL/          find_package(SSMEL)
```

公开符号只有 `ssmel_*`。

## 版本与 ABI

| 项 | 含义 |
|----|------|
| 包版本 | `VERSION`、`SSMEL_VERSION_STRING`、`ssmel_engine_version_string()` |
| ABI | `SSMEL_ABI_VERSION`（头文件）必须等于 `ssmel_abi_version()`（库） |

启动时建议检查：

```c
#include "ssmel/ssmel.h"

if (ssmel_abi_version() != SSMEL_ABI_VERSION)
    /* 头文件与 libssmel 不是同一套，不要继续 */
```

换版本时先读 [CHANGELOG.md](CHANGELOG.md)：

- **补丁**（x.y.Z，ABI 不变）：用新 zip 整包替换旧目录，重新链接即可。
- **次版本**（x.Y.z，ABI 不变）：可多新 API；旧调用一般仍可用，按 CHANGELOG 改即可。
- **ABI 变更**（`SSMEL_ABI_VERSION` 增加）：必须用新头文件重新编译；旧库产出的 `ssmel_map_t` 不能交给新库。

不要混用不同 zip 里的头文件和 `.so`。

## CMake

```cmake
find_package(SSMEL CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE SSMEL::ssmel)
# Windows 动态库还要：
# target_compile_definitions(your_app PRIVATE SSMEL_USE_SHARED)
```

```bash
cmake -S . -B build -DSSMEL_DIR=/path/to/ssmel-sdk-1.0.0/lib/cmake/SSMEL
# 或
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/ssmel-sdk-1.0.0
```

运行时让动态链接器能找到 `lib/libssmel.so`（`rpath`、`LD_LIBRARY_PATH`，或把库装到系统路径）。Linux 上 `libssmel` 已链接 `libm`。

## 最小用法

宿主自己解析格式、解码 WAV；只把 **planar float PCM** 交给 SDK。

```c
#include "ssmel/ssmel.h"

ssmel_map_t* map = ssmel_map_create();
uint32_t zone = ssmel_map_add_zone(map, 21, 108);

ssmel_buffer_t* buf = ssmel_buffer_create(channels, 1, frames, sample_rate);
ssmel_layer_desc_t desc;
ssmel_layer_desc_init(&desc);
desc.root_note = 60;
desc.hi_velocity = 127;
desc.trigger = SSMEL_TRIGGER_ATTACK;
ssmel_map_add_layer(map, zone, buf, &desc); /* 成功则消耗 buf */

ssmel_engine_t* engine = ssmel_engine_create();
ssmel_engine_prepare(engine, 48000.0);
ssmel_engine_set_sample_map(engine, map, SSMEL_INSTRUMENT_UPRIGHT, SSMEL_CHANNEL_STEREO);

ssmel_engine_note_on(engine, 60, 100, 1);
ssmel_engine_render_stereo(engine, left, right, frames);
ssmel_engine_note_off(engine, 60, 0, 1);

ssmel_engine_destroy(engine);
ssmel_map_destroy(map);
```

字段约定、XDI 折算、滤波 / 包络 API：见 [INTEGRATION.md](INTEGRATION.md)。
