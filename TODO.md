# TODO: ggml-viz Roadmap

Remaining work, ordered by priority. Items already shipped are not re-listed; use `git log` for what was done.

---

## Current status

- Core instrumentation, trace format, desktop UI, live mode, CLI, config manager, logger: **done** on macOS/Linux.
- Windows build compiles and CI runs, but interposition (MinHook-based DLL injection) is **experimental** and under-tested on real workloads.
- Demos (`examples/llama_demo`, `examples/whisper_demo`) run real models end-to-end.
- Overhead measured at <5% on benchmark workloads (see `benchmark_results/`).

---

## Phase 2 — Remote / IPC (medium priority)

- [ ] Shared-memory IPC for cross-process live streaming — scaffolding present in `src/ipc/`, not wired to the GUI.
- [ ] gRPC server for remote API access — not started (prior empty stub removed).
- [ ] WebSocket support for browser clients — not started.
- [ ] Minimal web dashboard (HTML/JS over the HTTP server) — not started.

## Phase 3 — Extensibility & viz (low priority)

- [ ] Plugin loader API — not started (prior empty stubs removed).
- [ ] GPU kernel tracing (Metal/CUDA timeline) — not started.
- [ ] Attention heatmaps for transformer workloads — not started.
- [ ] Memory-arena explorer — partial via memory events; no dedicated viewer yet.
- [ ] Export to Chrome Trace / Tracy / SVG / CSV — not started.

## Cross-cutting

- [ ] Windows interposition hardening — real end-to-end run on a llama.cpp binary, not just build/unit tests.
- [ ] Doxygen (or similar) reference doc generation — not started.
- [ ] Accurate `n_threads` in graph events outside the `ggml_graph_compute_with_ctx` path — scheduler/backend paths currently emit `0` (unknown); requires per-backend querying.

---

## Known issues / code debt

- `imgui_app.cpp`: several unfinished UI hooks (about dialog, file browser, live op timings computed from events, node→event selection mapping, per-tensor inspection). Marked with `TODO` inline.
- `imgui_widgets.cpp:431`: tensor→node dependency analysis is stubbed.
- `win32_interpose.cpp:11`: `MH_CreateHook` for `ggml_backend_sched_graph_compute` not installed — Windows only traces a subset of entry points.

---

## Pointers

- Format spec: [`docs/ggmlviz.md`](docs/ggmlviz.md)
- Build: [`docs/BUILD.md`](docs/BUILD.md)
- Ring buffer design: [`docs/RING_BUFFER_ANALYSIS.md`](docs/RING_BUFFER_ANALYSIS.md)

| Platform | Status | Interposition |
|---|---|---|
| macOS (arm64/x64) | Production | `DYLD_INTERPOSE` |
| Linux (x64) | Production | `LD_PRELOAD` |
| Windows 10+ | Experimental | MinHook DLL |
| Raspberry Pi | Untested | `LD_PRELOAD` (should work) |
