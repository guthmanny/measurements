# SSMEL SDK 升级说明

包版本在 zip 根目录的 `VERSION`，以及头文件 `SSMEL_VERSION_STRING`。  
ABI 在 `SSMEL_ABI_VERSION` / `ssmel_abi_version()`。

升级步骤（每个版本都适用）：

1. 读本文件对应章节，确认 ABI 是否变化。
2. 解压新 zip，**整包替换**旧 SDK 目录（头文件、库、CMake 一起换）。
3. 若 ABI 未变：重新链接即可；若 ABI 变了：用新头文件重新编译宿主。
4. 用新库重建 sample map；不要把旧进程里尚未销毁的 `ssmel_map_t` 交给新库。

---

## 1.0.0 — 2026-09-04

**ABI 1**（首发）

首个面向宿主的封闭 SDK。

- 公开头文件：`ssmel/ssmel.h`、`ssmel_api.h`、`ssmel_engine.h`、`ssmel_map.h`
- 库：`libssmel`（符号仅 `ssmel_*`）
- 建图：`ssmel_buffer_create`、`ssmel_map_create` / `add_zone` / `add_layer`
- 发声：`ssmel_engine_*`（MIDI、渲染、滤波、EG、LFO、调音、legato）
- `set_sample_map` 接受 `ssmel_map_t*`；ABI 不符返回 `SSMEL_ERR_STATE`

已知限制见 [INTEGRATION.md](INTEGRATION.md) 文末。
