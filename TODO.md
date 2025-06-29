# TODO: ggml-viz Implementation Tracker

This file tracks the remaining implementation work required for the `ggml-viz` project.

---

## 🧩 Empty Source Files (Need Implementation)

### 🎨 Frontend Components
- [ ] `src/frontend/imgui_app.cpp` — Main ImGui application and UI rendering
- [ ] `src/frontend/imgui_widgets.hpp` — Custom ImGui widgets for visualization

### 🛰 Server/Data Layer
- [ ] `src/server/grpc_server.cpp` — gRPC API server for real-time data access
- [ ] `src/server/data_collector.cpp` — Event collection and processing logic
- [ ] `src/server/data_structs.hpp` — Data structures for server components
- [ ] `src/server/server_core.hpp` — Core server functionality definitions

### 🧪 IPC Layer
- [ ] `src/ipc/ipc_common.hpp` — Common IPC definitions and structures
- [ ] `src/ipc/shm_posix.cpp` — POSIX shared memory implementation
- [ ] `src/ipc/shm_windows.cpp` — Windows shared memory implementation

### 🔌 Plugin System
- [ ] `src/plugins/plugins_api.hpp` — Plugin API definitions
- [ ] `src/plugins/plugins_loader.cpp` — Dynamic plugin loading system

### 🧰 Utilities
- [ ] `src/utils/config.cpp` — Configuration management
- [ ] `src/utils/logger.cpp` — Logging system

---

## 🛠 Development Scripts
- [ ] `scripts/lint.sh` — Code linting (required for PRs)
- [ ] `scripts/format.sh` — Code formatting
- [ ] `scripts/run_tests.sh` — Test execution script

---

## 🚀 Example Applications
- [ ] `examples/llama_demo/run_llama_vis.cpp` — LLaMA integration demo
- [ ] `examples/whisper_demo/run_whisper_vis.cpp` — Whisper integration demo

---

## ⚙️ Missing GGML Submodule Integration
Hooks are **not yet patched** into the GGML compute graph:

- [✅] No `ggml_viz_hook_*` functions found in `third_party/ggml/`
- [✅] Modify `third_party/ggml/src/ggml.c` (or `ggml.cpp`) to inject hooks:
  - `ggml_viz_hook_graph_compute_begin(graph);`
  - `ggml_viz_hook_op_compute_begin(tensor);`
  - `ggml_viz_hook_op_compute_end(tensor);`
  - `ggml_viz_hook_graph_compute_end(graph);`

---

## 🧱 Missing Core Features (From README)

- [ ] `main.cpp` — Main executable entrypoint for the ggml-viz binary
- [ ] WebSocket API — For remote visualizer control (mentioned in README)
- [ ] Electron client — Optional web dashboard frontend
- [ ] GPU kernel tracing — Integration with Metal or CUDA
- [ ] Attention visualization — Transformer-specific internals UI

---

✅ **Implemented**
- Instrumentation hook infrastructure (partial)
- Trace recording and flushing to file
- Initial unit test (`test_ggml_hook.cpp`)

---
---
---

## 🛠 Hook Integration Strategy

- [✅] **Conditional Compilation** — Use `#ifdef GGML_VIZ_ENABLE_HOOKS`
- [✅] **Link-Time Integration** — Hook functions are provided by the `ggml_hook` library
- [✅] **CMake Build Integration** — Ensure `GGML_VIZ_ENABLE_HOOKS` is defined in CMake when visualization is enabled

### 📂 Files to Modify
- [✅] `third_party/ggml/src/ggml-cpu/ggml-cpu.c` — Add hook invocations
- [✅] `third_party/ggml/include/ggml-cpu.h` — Add hook function declarations
- [✅] `third_party/ggml/src/CMakeLists.txt` — Add `GGML_VIZ_ENABLE_HOOKS` flag

---

