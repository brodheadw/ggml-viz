# GGML-viz Ring Buffer: Evolution & Optimisation Notes
*Last updated 01 Aug 2025*

---

## 1 · Timeline of Designs

| Phase | Date (2025) | Design | Synchronisation | Pain-points |
|-------|-------------|--------|-----------------|-------------|
| **V0 – "naïve" buffer** | ≈ 15 Jul | Fixed-size circular array of `Event`; atomic head & tail **plus a global `std::mutex`** | Full `buffer_mutex_` around every enqueue/dequeue | • 15-18 % overhead on long traces<br>• README called it "lock-free" (incorrect)<br>• Mixed/incorrect memory orders<br>• No overflow accounting |
| **V1 – true SPSC lock-free** | **26 Jul** (PR #29) | Same array; *one-empty-slot* rule; monotonic counters (`head`, `tail`) cache-line aligned | **Zero locks** – single-producer / single-consumer algorithm with acquire/release fences | Contention removed; see §5 for perf gains |

---

## 2 · What Was Wrong With V0

* **Global mutex** – every `GGMLHook::record_event()` call locked `buffer_mutex_`, contradicting the "lock-free" claim.  
* **Fence mismatch** – producer incremented `write_pos_` with `memory_order_relaxed`; consumer loaded it with `memory_order_acquire`. The inverse happened on the read side.  
* **No back-pressure** – when full, producer blocked on the mutex; no metric counted lost events.  

These flaws manifested under token-by-token LLaMA runs or real-time Whisper traces, inflating wall-time by **≈ 15 %**.

---

## 3 · Requirements Driving the Rewrite

| Requirement | Consequence in V1 |
|-------------|-------------------|
| **SPSC topology** (model thread ↔ GUI/trace thread) | Simplest lock-free queue suffices; no ABA tagging. |
| **"Write-fast, drop-cheap"** | When full, producer increments `dropped_events` and skips; model never blocks. |
| **Cross-process (POSIX & Win32 SHM)** | Metadata header (`head`, `tail`, `dropped`) is cache-line-aligned and mmap-shared. |
| **Portable C++17** | No TS/coro; plain array + `std::atomic`. |

---

## 4 · Key Code Changes (PR #29)

| File | Change summary |
|------|----------------|
| `src/instrumentation/ggml_hook.cpp` | **Deleted `buffer_mutex_`**; added SPSC full test `((head+1)&mask)==(tail&mask)`; `write_pos_.store(..., release)`. |
| `ggml_hook.hpp` | Added `alignas(64)` padding for each atomic; constexpr `mask = SIZE-1`. |
| `shm_posix.cpp` & `shm_windows.cpp` | Harmonised to same acquire/release contract for cross-proc traces. |
| `tests/test_ring_buffer.cpp` | New stress test (200 M events/s) under ThreadSanitizer; asserts zero data races & tracks dropped count. |

---

## 5 · Memory-Ordering Contract (V1)

| Role | Steps |
|------|-------|
| **Producer** | 1 · `head = head.load(relaxed)`<br>2 · `tail = tail.load(acquire)`<br>3 · write event data<br>4 · `head.store(head+1, release)` |
| **Consumer** | 1 · `tail = tail.load(relaxed)`<br>2 · `head = head.load(acquire)`<br>3 · read events<br>4 · `tail.store(head, relaxed)` |

Guarantees:

1. Consumer never sees half-written events.  
2. Producer never overwrites unread slots.  
3. Only **two** heavyweight fences per event (steps 2 & 4).

---

## 6 · Performance After the Rewrite

* **Overhead**: **< 5 %** on 7 t/s TinyLlama-1.1B decode (down from ~15 %).  
* **Why it drops**:  
  * Contention gone – no thread ever blocks.  
  * Head & tail on separate cache-lines → no false sharing.  
  * Fewer fences (2 ⇣ from 4).  
* **CI**: GitHub Action "perf-regression" runs each PR; fails if overhead > 8 %. Benchmark CSV lives in `benchmark_results/`.

---

## 7 · Future Work

1. **Expose dropped-event metric in GUI** (done, v0.4).  
2. **MPMC upgrade path** – Vyukov bounded queue when multi-threaded GGML back-ends appear.  
3. **Continuous benchmarking** – perf job now guards against re-introducing locks.

---

> **TL;DR** – The ring buffer is now genuinely lock-free, SPSC-correct, and its behaviour is fully documented, closing the gap between promises and reality.