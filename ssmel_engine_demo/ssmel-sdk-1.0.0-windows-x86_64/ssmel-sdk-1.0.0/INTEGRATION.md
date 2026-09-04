# SSMEL SDK 集成指南

本文说明如何在 **Dream Instrument Editor**（或任意宿主）中集成本 zip 提供的 **SSMEL 采样音源引擎**。

解压、CMake、换版本：见同目录 [README.md](README.md)、[CHANGELOG.md](CHANGELOG.md)。

宿主负责把 XDI 原样写进 sample map；引擎只按 map 发声。不要用引擎 preset 覆盖文件里的 Amplifier / Envelope / Filter 表。

| 章节 | 内容 |
|------|------|
| [职责划分](#职责划分) | 谁解析 XDI、谁发声 |
| [Sample Map 契约](#sample-map-契约) | zone / layer 字段 |
| [XDI → map](#xdi--map客户端) | 包络、Rate、力度、滤波、循环 |
| [构建与链接](#构建与链接) | CMake / ABI |
| [最小集成流程](#最小集成流程纯发声路径) | create → MIDI → render |
| [Phase 3 / 4](#phase-3--dream-synth-标签页) | 滤波、EG、LFO、调音 |
| [常见问题](#常见问题) | 坑与不要做的事 |

## SSMEL（一句话）

| 名称 | 含义 |
|------|------|
| **SSMEL SDK** | 封闭二进制：`include/ssmel/*.h` + `libssmel.so`。公开符号只有 `ssmel_*` |
| **`ssmel_map`** | 客户建 sample map 的唯一 API（WAV 由宿主解码成 float PCM） |
| **`ssmel_engine`** | 发声：MIDI、渲染、滤波、包络、共振 |

只使用本 zip 中的头文件与 `libssmel`。

## 职责划分

| 模块 | 负责方 | 说明 |
|------|--------|------|
| XDI 解析 | **客户端** | 读取 Dream `.xdi` 工程、Split / Layer / ASV 等元数据 |
| WAV 加载 | **客户端** | 自行解码；只把 **planar float PCM** 交给 `ssmel_buffer_create` |
| Sample Map | **客户端** | 用 `ssmel_map.h` 组装 `ssmel_map_t`（见 [Sample Map 契约](#sample-map-契约)） |
| 音源引擎 | **SDK** | `libssmel.so` — MIDI、渲染、滤波、包络、共振等 |

引擎 **不** 解析 XDI、**不** 读 WAV 文件。Map 由客户 `ssmel_map_create` / `ssmel_map_destroy`。

```
┌─────────────────────────────────────────────────────────┐
│  Dream Instrument Editor (你的后端)                      │
│  ┌─────────────┐  ┌─────────────┐  ┌──────────────────┐ │
│  │ XDI Parser  │→ │ WAV decode  │→ │ ssmel_map_*      │ │
│  └─────────────┘  └─────────────┘  └────────┬─────────┘ │
│                                              │ ssmel_map_t* │
│  ┌──────────────────────────────────────────▼─────────┐ │
│  │ libssmel.so                                          │ │
│  │  prepare / set_sample_map / MIDI / render            │ │
│  └────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
```

只 `#include "ssmel/ssmel.h"`（或 `ssmel_engine.h` + `ssmel_map.h`），只链接 `libssmel`。

## Sample Map 契约

`ssmel_engine_set_sample_map(engine, map, …)` 接受 **`ssmel_map_t*`**（由本 SDK 的 `ssmel_map_create` 产出）。  
`ssmel_map` 的 `abi` 必须等于 `SSMEL_ABI_VERSION`（当前为 1）；不匹配返回 `SSMEL_ERR_STATE`。宿主也可比较 `ssmel_abi_version()` 与头文件里的 `SSMEL_ABI_VERSION`。

### 类型与 ABI

| 要求 | 说明 |
|------|------|
| 实际类型 | 必须为 `ssmel_map_t*`，用 `ssmel_map_create` / `ssmel_map_add_zone` / `ssmel_map_add_layer` 构建 |
| ABI 版本 | `SSMEL_ABI_VERSION`（头文件）= `ssmel_abi_version()`（所链 `libssmel.so`） |
| 不推荐 | 自研 struct / 直接塞内部指针 |

### 生命周期与所有权

- 引擎 **不拥有** map：不 `create`、不 `destroy` 客户的 map。
- 自 `set_sample_map` 起至下次 `set_sample_map` 或 `engine_destroy` 之前，map 必须 **保持有效**。
- `ssmel_map_add_layer` **成功时消耗** `ssmel_buffer_t`（不要再 `ssmel_buffer_destroy`）；失败时 buffer 仍归调用方。
- 销毁 map 用 `ssmel_map_destroy`（会释放其中已接管的 PCM）。
- **播放中禁止** 修改 map（增删 layer、更换 buffer 等）；应先 `all_notes_off`，再换 map 或重建。

### 最少能出声的内容

要能 `note_on` 后有声音，通常需要：

1. **至少 1 个 zone**：`lo_key`～`hi_key`（MIDI 0–127，含边界）覆盖目标音高。
2. **至少 1 个 note_on 层**：`trigger` 为 `ATTACK`、`FIRST` 或 `LEGATO`（最常见为 `ATTACK`）。
3. **有效 sample buffer**：
   - float 平面数据，**1 或 2 声道**
   - `sample_rate > 0`，`num_frames > 0`
   - `start_frame` / `end_frame` 落在有效范围内（均为 0 时会按全长处理）

空 map 可 `set_sample_map` 成功，但按键 **无声**。

### Layer 字段

| 字段 | 要求 / 说明 |
|------|-------------|
| PCM | **必填**：`ssmel_buffer_create` 的 planar float，1 或 2 声道 |
| `root_note` | 样本根音（MIDI），playback 据此变调 |
| `lo_velocity` / `hi_velocity` | MIDI 力度 0–127；若写反会自动交换 |
| `trigger` | `ATTACK` / `RELEASE` / `FIRST` / `LEGATO` / `RESONANCE` |
| `group` / `off_by` | 同组切音、release 互斥（`0` = 默认组） |
| `loop_mode` + loop 区间 | 循环层需合法 `loop_start` / `loop_end`（半开 `[start, end)`）。FORWARD 在 playhead **进入 loop 之后**才环绕插值；进入前按文件线性读（锤击/前摇）。松键后 FORWARD **继续环绕**，由 amp EG 收声，见 [循环与松键](#循环与松键) |
| `amp_vel` | Dream Amplifier `Min/MaxVelMod`。Y 存 **dB**；`VelCurve=Straight` 先 `10^(dB/20)` 再对线性增益插值。见 [Amplifier 力度](#amplifier-力度) |
| `crossfade_below` / `crossfade_above` | velocity 层交叉淡化宽度（ MIDI 力度单位） |
| `min_hold_ms` | release 层：短于该按键时长则不触发 |
| `legato_offset_ms` | `LEGATO` 层：跳过样本开头毫秒数 |
| `use_release_velocity` | release 层是否采用 `note_off` 力度 |
| `gain` | 静态层增益（≤0 视为 1） |
| `amp_eg` / `filter_eg` | 可选分区 AHDSR（含 `hold_ms` / `break_*` / `oneshot`）。`note_on` 只改当前 voice。折算规则见 [Dream 包络](#dream-包络--ahdsr) |
| `filter_kbd` | Dream KbdTable，单位：倍频。Frequency 空间：`log2(Frequency+Value)-log2(Frequency)` |
| `filter_cutoff_oct` | 分区截止偏移（倍频）。Dream `Frequency`+`FreqOffset` → `log2` |
| `filter_vel` | Dream `FreqMin/MaxVelMod` 表；`note_on` 把 Y 转成倍频叠到 cutoff。空表视为 1 |
| `filter_env_amount` | Dream `Env2Amount`。`note_on` 后 cutoff `*= eg2^amount`；>0 时该 voice 的 `EG2→FILTER` 矩阵清零。宿主不要再叠一层 `VEL/EG2→FILTER` |

**Zone 重叠**：同一 note 命中多个 zone 时，取 **键区最窄** 的 zone。

**Layer 模式**（velocity 选层 / 全播 / round-robin / crossfade）由 map 上的 `layer_mode` 决定；见下节 preset 会覆盖部分设置。

Dream 概念映射：

| Dream Editor | SSMEL SDK |
|--------------|-----------|
| Split (键区) | `ssmel_map_add_zone` |
| Layer | `ssmel_map_add_layer` |
| Note On / Off | `trigger`: `SSMEL_TRIGGER_ATTACK` / `RELEASE` / `FIRST` / `LEGATO` |
| ASV (轮替) | `group` + `ssmel_map_set_layer_mode(…, SSMEL_LAYER_ROUND_ROBIN)` |
| Note Off 层 | `SSMEL_TRIGGER_RELEASE` + `min_hold_ms` / `use_release_velocity` |
| velocity 层 | 多 layer + `crossfade_below/above` |
| 声学连奏 | `legato_offset_ms` + `SSMEL_TRIGGER_LEGATO` |
| 共振层 | `SSMEL_TRIGGER_RESONANCE` |

### `set_sample_map` 对 map 的副作用

调用 `set_sample_map(engine, map, preset, layout)` 时，引擎会执行：

按 **UPRIGHT / EP preset** 写入 map 的：

- `layer_mode`（如 upright → velocity crossfade）
- `release_layer_mode`

**客户在 map 上预先设置的 layer_mode 会被 preset 覆盖。** zone、layer、buffer 内容不会被引擎修改。

`layout`（`MONO` / `STEREO`）只影响 **输出** 声道数，不改正样本数据。

### 采样率

- `prepare(engine, sample_rate)`：引擎 **播放采样率**（如 48000 Hz）。
- 样本 buffer 自带 `sample_rate`；播放时自动做 `buffer_sr / engine_sr` 比率换算（44100 样本 + 48000 引擎可正常工作）。

### 引擎不验证的内容

- XDI / WAV 路径是否正确
- zone 是否覆盖全键盘
- 样本是否 clip、声道是否与 Dream 一致

以上由 **客户内容框架** 负责；引擎仅按 map 规则 resolve 并播放。

### Map 构建示例

```c
#include "ssmel/ssmel.h"

if (ssmel_abi_version() != SSMEL_ABI_VERSION)
    return; /* 头文件与 libssmel.so 不匹配 */

ssmel_map_t* map = ssmel_map_create();
uint32_t zone = ssmel_map_add_zone(map, 21, 108);

ssmel_buffer_t* buf = ssmel_buffer_create(channels, 1, num_frames, sample_rate);

ssmel_layer_desc_t desc;
ssmel_layer_desc_init(&desc);
desc.root_note = 60;
desc.lo_velocity = 0;
desc.hi_velocity = 127;
desc.trigger = SSMEL_TRIGGER_ATTACK;
desc.group = 1;
ssmel_map_add_layer(map, zone, buf, &desc); /* 成功则消耗 buf */
```

---

## XDI → map（客户端）

引擎不读 XDI。下面是把 Dream Editor 工程折成 `ssmel_layer_desc_t` 的约定。折算由宿主自己实现。

### Dream 包络 → AHDSR

Dream Envelope 是 **Rate / Level 点列**，不是 ADSR 旋钮。点的角色：

| 点 | 角色 | Rate | Level |
|----|------|------|-------|
| 1 | 起点 | **无**（文件里的 Rate 忽略） | 初始电平 |
| 2 | Attack | 点 1 → 点 2 的速度 | 峰值 |
| 3 | Decay（按住时第一段） | 点 2 → 点 3 | 衰减目标 |
| 4…n−1 | 继续按住时的段 | 上一点 → 本点 | 本段目标 |
| 最后一点 | Release（有门限时） | 衰减 → 释放 | 释放终点 |

`SustainPoint` **不是**「停在第 N 点、跳过 decay」的冻结下标。只用符号：

- `SustainPoint < 0`：oneshot。松键后保持末段电平，不再收到 0。写 `eg.oneshot = true`。Piano / EP 的 **Env2** 都是 oneshot；按住出声靠 FORWARD loop + Env1，不是 Env2 门限。
- `SustainPoint ≥ 0`：有门限。**最后一点 = release**；按住时走完第 3 点到倒数第二点。

折到 AHDSR：

| 文件 | 点数 | attack | break | sustain | release |
|------|------|--------|-------|---------|---------|
| Piano / EP 中低音 Env1 | 4 | 点 2 | — | 点 3 | 点 4 |
| Piano 约 MIDI 78 以上 Env1 | 5 | 点 2 | 点 3（`break_ms` / `break_level`） | 点 4 | 点 5 |

瞬时 attack：起点 Level≈1，点 2 Level≈1，且点 2 Rate ≥ 0.99 → `attack_ms = 0`。

Env2 三点 oneshot：若点 2 在下降，把点 2 写成 AHDSR break，后面段叠进 `decay_ms`。

`VelScaling=0` 时 EG 峰值保持 1，力度只走 Amplifier 表。

**不要：**

- 把 `SustainPoint` 当成冻结下标而跳过 decay（高音 5 点会停在非 0 的点 3，听起来一直响）。
- 用点 1 的 Rate 当 attack。
- 把 Rate 乘以 `|Δlevel|`（除非日后实测要求）。
- 把 Env2 oneshot 当成 amp / sample oneshot。

### Rate → 毫秒

Dream Rate：0 = 最慢，约 0.99 = UI 最快，文件里偶见 1.27。

当前是 **线性占位**（客户端 `xdiRateToMs`），不是 1-pole τ，也不是指数表。实测官方时间后只改端点，不要改折段逻辑：

```
T(0)      = 8000 ms
T(0.99)   = 10 ms      （0 … 0.99 直线）
T(≥1.27)  = 1 ms       （0.99 … 1.27 从 10 ms 收到 1 ms）
```

| Rate | T(ms) |
|------|-------|
| 0 | 8000 |
| 0.25（钢琴 decay 常见） | ≈ 5980 |
| 0.50（EP decay 常见） | ≈ 3970 |
| 0.99 | 10 |
| 1.27 | 1 |

`release_ms` 存满幅 `T(rate)`。AHDSR 打开 `scale_release_with_level` 后，实际释放时间再乘松键时的电平。Sampler / `note_on` 层 EG 用接近线性的 timed curve（约 0.02），不要用默认 curve 0。

### Amplifier 力度

这些 bank 的 Amplifier 表：`MinVelModX/Y = 0 / −96`，`MaxVelModX/Y = 127 / −0.136`，`VelCurve=Straight`。

GUI 的 Y 是 dB，但 Straight **先换成线性增益再插值**：

```
lin_lo = 10^(lo_y/20)
lin_hi = 10^(hi_y/20)
gain   = lerp(lin_lo, lin_hi, vel)
```

MIDI 0 ≈ 1.58×10⁻⁵（−96 dB），64 ≈ 0.50（−6 dB），127 ≈ 0.985（−0.14 dB）。

写 layer `amp_vel`（Y 仍存文件里的 dB）。`note_on` 时：resolve / 滤波用原始 MIDI；EG 峰值 = 1；VCA 用该表换算出的线性增益。引擎 preset 的 SOFT / LINEAR 曲线不要再叠。

**不要：** 在 dB 上拉直线；同时开 sampler `apply_velocity` 和 VCA `VEL→AMP`；用引擎 soft/linear 替换 XDI 表。

滤波力度仍用 `FreqMin/MaxVelMod`，按原始 MIDI 烘焙 Y（线性 Y，不是 dB）。

### 滤波

- `filter_cutoff_oct = log2(Frequency + FreqOffset)`。`Frequency` 不是已经取过对数。
- KbdTable 存倍频：`log2(Frequency+Value) - log2(Frequency)`。
- `filter_env_amount` = `Env2Amount`。`note_on` 后 cutoff `*= eg2^amount`；amount > 0 时该 voice 的 `EG2→FILTER` 矩阵清零。
- 宿主不要再叠一层 `VEL→FILTER` / `EG2→FILTER`。
- Sampler VCF cutoff 平滑约 3 ms。
- Piano / EP 这类 bank：**关掉** sample 共振和 harmonic resonance（`set_resonance_enabled(0)` / `set_harmonic_resonance_enabled(0)`）。

### 循环与松键

- FORWARD：playhead **到达 `loop_start` 之前**，Hermite 抽头按文件线性读。提前把抽头 wrap 进 loop，会把锤击读成随机 loop 相位（咔咔）。
- 进入 loop 后，抽头只在 `[loop_start, loop_end)` 内环绕。
- 松键：**不**退出 FORWARD loop 去播 loop 后的尾巴。短 loop（如高音钢琴几十帧）在 exclusive `loop_end` 上 Hermite 越界会爆响。Amp EG 负责收声。
- 只有将要生成 **release-trigger 层** 时，才对 looping attack 做 fade/stop；先 `resolve_release` 再 spawn，避免新 release voice 占到刚被停掉的槽。
- one-shot（无 loop）松键仍按原样 fade/stop。
- fade 进行中也必须继续 wrap。

---

## 构建与链接

本 zip 是封闭二进制：`include/ssmel/` + `libssmel`。CMake 与换版本步骤见 [README.md](README.md)。

```cmake
find_package(SSMEL CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE SSMEL::ssmel)
target_compile_definitions(your_app PRIVATE SSMEL_USE_SHARED) # Windows dllimport
```

```bash
cmake -S . -B build -DSSMEL_DIR=/path/to/ssmel-sdk-<version>/lib/cmake/SSMEL
```

或 `-DCMAKE_PREFIX_PATH=/path/to/ssmel-sdk-<version>`。只 include `ssmel/ssmel.h`，只链接 `SSMEL::ssmel`。

## 最小集成流程（纯发声路径）

### 1. 创建与初始化

```c
#include "ssmel/ssmel.h"

ssmel_engine_t* engine = ssmel_engine_create();
ssmel_engine_prepare(engine, 48000.0);
```

### 2. 绑定 Map

Map 由宿主用 `ssmel_map_*` 建好后传入：

```c
ssmel_map_t* map = content_module_get_sample_map();

ssmel_engine_set_sample_map(
    engine,
    map,
    SSMEL_INSTRUMENT_UPRIGHT, /* 或 SSMEL_INSTRUMENT_EP */
    SSMEL_CHANNEL_STEREO);    /* 或 MONO */
```

`set_sample_map` 会重建内部 voice，并应用 instrument profile（velocity 曲线、sustain CC64、共振等）。Map 字段要求见 [Sample Map 契约](#sample-map-契约)。

### 3. 音频线程

```c
/* MIDI */
ssmel_engine_note_on(engine, note, velocity, voice_id);
ssmel_engine_note_off(engine, note, velocity, voice_id);
ssmel_engine_control_change(engine, 64, pedal_value); /* CC64 sustain */

/* 渲染：单次最多 SSMEL_ENGINE_MAX_BLOCK (2048) 帧 */
ssmel_engine_render_stereo(engine, out_l, out_r, frames);
/* 或 */
ssmel_engine_render_mono(engine, out, frames);
```

推荐顺序：`note_on/off` → `render`（内部会 `update` modulator）。块大小与宿主 buffer 对齐即可。

### 4. 销毁

```c
ssmel_engine_destroy(engine);
ssmel_map_destroy(map);
```

## Phase 3 — Dream Synth 标签页

Sampler prefab 默认配置：

- **滤波器**：2-pole SVF，默认 LP @ 16 kHz，Q = 0.707
- **EG1 → VCA**（AHDSR，默认 A=1 ms, H=0, D=10 ms, S=1, R=80 ms）
- **VEL → VCA**（深度 1：`amp = EG1 * velocity`，不是相加）
- **EG2 → Filter cutoff**（AHDSR，默认 A=3 ms, H=0, D=40 ms, S=0, R=180 ms，矩阵量 4 oct）
- **VEL → Filter cutoff**（默认 3.5 oct；host 可按乐器改）
- **EG3**：ADSR，可经 mod matrix 路由
- **LFO1/2**：正弦，默认 5 Hz / 0.25 Hz，经 `set_mod_amount` 路由

### 滤波器 API

```c
ssmel_engine_set_filter_cutoff_hz(engine, 8000.0);
ssmel_engine_set_filter_Q(engine, 1.2);
ssmel_engine_set_filter_mode(engine, SSMEL_FILTER_LP);
/* SSMEL_FILTER_BP | SSMEL_FILTER_HP */
```

Cutoff 调制：`set_mod_amount(engine, SSMEL_MOD_EG2, SSMEL_MOD_DST_FILTER, 4.0f)`  
单位：**倍频（octaves）**。Dream `Env2Amount` 不要走这条矩阵：写 layer `filter_env_amount`，cutoff 变成 `Frequency × velY × eg2^amount`。

Layer `gain`（默认 1）在 resolve 时乘进播放增益，用来对齐 XDI OscAmp / Amplifier / Mixer 的分区响度。

`note_on` 会按命中层应用可选的 `amp_eg` / `filter_eg`（只改当前 voice），以及 `filter_kbd` + `filter_cutoff_oct` + `filter_vel` + `filter_env_amount` + `amp_vel`。力度表和 Env2Amount 在按键时烘焙，宿主若使用它们应把 `VEL→FILTER` / `EG2→FILTER` 矩阵量设为 0，避免再乘一次。拧全局 EG / Filter 旋钮不会覆盖已在响的分区包络。

Layer `hold_ms` 对应 Dream 里保持在峰值的段（例如 Env2 中高音约 50 ms 满开再收滤波）。`hold_ms = 0` 时行为与 ADSR 相同。XDI 折算、Rate、力度、循环见 [XDI → map](#xdi--map客户端)。

### 包络 API（AHDSR / ADSR）

```c
/* eg_index: 0=EG1, 1=EG2, 2=EG3 */
ssmel_engine_set_eg_attack_ms(engine, 0, 2.0);
ssmel_engine_set_eg_decay_ms(engine, 0, 50.0);
ssmel_engine_set_eg_sustain_level(engine, 0, 0.85f);
ssmel_engine_set_eg_release_ms(engine, 0, 120.0);
```

### LFO 与 Mod Matrix

```c
ssmel_engine_set_lfo_rate_hz(engine, 0, 4.5f);
ssmel_engine_set_lfo_depth(engine, 0, 0.08f); /* octaves，pitch 目标时 */

ssmel_engine_set_mod_amount(engine, SSMEL_MOD_LFO1,
                                    SSMEL_MOD_DST_PITCH, 0.5f);
ssmel_engine_set_mod_amount(engine, SSMEL_MOD_EG1,
                                    SSMEL_MOD_DST_AMP, 1.0f);
```

Mod 源 / 目标枚举见 `ssmel_engine.h`。

## Phase 4 — Dream Global / Layer

### 调音表（± cents / key）

```c
ssmel_engine_set_tuning_cents(engine, 60, 3.5f);   /* Middle C +3.5 cent */
ssmel_engine_set_tuning_cents(engine, 72, -2.0f);
```

内部为分段线性表（0–127），Dream Tuning Table 导入时逐点调用即可。

### Keyboard 调制表

```c
ssmel_engine_set_keyboard_mod_enabled(engine, 1);
ssmel_engine_set_keyboard_mod_cents(engine, 64, 10.0f);
```

与 tuning 叠加，作用于 playback `fine_cents`。

### Legato 模式

```c
ssmel_engine_set_legato_mode(engine, SSMEL_LEGATO_OFF);     /* 默认 */
ssmel_engine_set_legato_mode(engine, SSMEL_LEGATO_SYNTH);    /* 单音滑音，不重触发 */
ssmel_engine_set_legato_mode(engine, SSMEL_LEGATO_ACOUSTIC);  /* LEGATO 层 + legato_offset_ms */
```

Layer 定义示例（声学连奏）：

```c
desc.trigger = SSMEL_TRIGGER_LEGATO;
desc.legato_offset_ms = 120.0;  /* 跳过样本前 120 ms */
```

## 内置乐器能力（引擎侧）

绑定 map 后，引擎还提供：

| 功能 | 说明 |
|------|------|
| 32 复音 | 引擎 voice pool：每个 MIDI 音一个独立 voice（EG / VCF / VCA） |
| Voice 余量 | 每 voice 固定 −12 dB（`SSMEL_ENGINE_VOICE_HEADROOM`），不随复音数压已开的音 |
| Release triggers | `off_by` / group |
| Velocity 交叉淡化 | Kontakt 式 curve + 层 crossfade |
| Sustain CC64 | `control_change(64, val)` |
| Sample 共振层 | `RESONANCE` trigger +  held-note 谐波匹配 |
| Harmonic resonator | 12 路 SVF bandpass 激励 |
| Voice steal | release-first / oldest / none |

这些在 **map + preset** 层配置。Piano / EP 预览请关掉共振：

```c
ssmel_engine_set_resonance_enabled(engine, 0);
ssmel_engine_set_harmonic_resonance_enabled(engine, 0);
ssmel_engine_set_attack_release_fade_ms(engine, fade_ms); /* 可选 */
```

## 线程与生命周期

- **单引擎实例** 非线程安全；音频与 UI 线程之间请自行加锁或 SPSC 队列传递 MIDI。
- **`set_sample_map`** 会销毁并重建最多 `SSMEL_ENGINE_MAX_VOICES`（32）个独立 voice：请在无音频播放时调用，或先 `all_notes_off` 并等待 `is_active == 0`。
- **`note_on` / `note_off`** 按 note + `voice_id` 路由到其中一个 voice；同一键再按时重触发该槽，满额时偷最老的 voice。
- **Map 与 buffer 生命周期** 见 [Sample Map 契约](#sample-map-契约)；引擎只持有 map 指针，不拷贝 WAV 数据。

## 错误码

| 值 | 常量 | 含义 |
|----|------|------|
| 0 | `SSMEL_OK` | 成功 |
| -1 | `SSMEL_ERR_NULL` | 空指针 |
| -2 | `SSMEL_ERR_STATE` | 非法状态（如 sample_rate ≤ 0） |

## 版本

```c
const char* ver = ssmel_engine_version_string();
/* 当前: "ssmel 1.0.0" */
```

## 参考头文件

| 用途 | 头文件 | 链接 |
|------|--------|------|
| 一站式 | `ssmel/ssmel.h` | `libssmel` |
| 引擎 C ABI | `ssmel/ssmel_engine.h` | `libssmel` |
| Sample map / buffer | `ssmel/ssmel_map.h` | `libssmel` |
| 导出宏 / ABI | `ssmel/ssmel_api.h` | （被上面两个 include） |

## 常见问题

**Q: 客户端要 include 哪些头文件？**  
A: `#include "ssmel/ssmel.h"`，或分别 include `ssmel_engine.h` 与 `ssmel_map.h`。只链接 `libssmel`。

**Q: 对客户建的 map 有什么要求？**  
A: 见 [Sample Map 契约](#sample-map-契约)：必须是 `ssmel_map_t*`、`ssmel_abi_version()` 与头文件一致、生命周期由客户管理、至少一个有效 ATTACK 层才能出声。

**Q: 能否让引擎加载 XDI？**  
A: 否。XDI→map 是编辑器/内容模块职责。

**Q: 立体声样本如何输出？**  
A: `SSMEL_CHANNEL_STEREO` + map 内 stereo buffer；mono 样本会按布局规则 upmix。

**Q: EG 曲线 / Dream rate-level 段？**  
A: Sampler 现为 AHDSR（可加一段 `break`）。完整多段与 Envelope `LoopStart` 尚未实现。折算见 [Dream 包络](#dream-包络--ahdsr)。

**Q: Rate 0.25 为什么不该只有几百毫秒？**  
A: 旧实现用指数表，0.25 → ~768 ms。现为直线：0.25 → ~6 s。端点待实测后只改 `xdiRateToMs`。

**Q: 松键爆响 / 起音咔咔？**  
A: 松键爆响 = 退出 FORWARD loop 后 Hermite 读到 loop 外。起音咔咔 = playhead 还在 loop 前就把抽头 wrap 进 loop。引擎已按 [循环与松键](#循环与松键) 处理。

**Q: sustain Level=0，循环是不是该静音？**  
A: 否。那是 Env1 点 3/4 的目标电平：按住时 **decay 到 0**，不是「loop 增益 = 0」。声音靠 FORWARD 循环撑到 EG 收完。

**Q: 滤波斜率 12/24 dB？**  
A: SVF 为 2-pole（约 12 dB/oct）。更高阶可在后续串联两枚 SVF 或扩展 processor。

**Q: Mixer / AutoFx / 总线限幅？**  
A: 尚未进引擎。这些 bank 的 AutoFx=0。总线限幅以后再加。

**尚未实现 / 已知限制**

- 官方 Rate→ms（占位端点，客户端可换）
- 真·多段 EG、Envelope `LoopStart`
- Mixer FX / AutoFx
- `set_sample_map` 在音频线程重建 32 voice（可能 xrun）

---

如有 API 变更，以本包 `include/ssmel/` 与 [CHANGELOG.md](CHANGELOG.md) 为准。
