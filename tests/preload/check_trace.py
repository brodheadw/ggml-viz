"""Assert a sched_check trace has real per-op timings: 2 graphs, 3 ops each, ops not all equal.
With "alloc": assert an alloc_check trace holds exactly the one Q4_0 weight allocation."""
import struct, sys
from collections import Counter

NAMES = ["GRAPH_BEGIN", "GRAPH_END", "OP_BEGIN", "OP_END", "ALLOC", "FREE", "BARRIER", "THREAD_BEGIN", "THREAD_FREE"]
EVENT_DATA = 32  # sizeof(Event::data) on 64-bit

d = open(sys.argv[1], "rb").read()
assert d[:8] == b"GGMLVIZ1", "bad magic"
i, counts, begin, durations, allocs = 12, Counter(), {}, {}, []
while i < len(d):
    etype, ts = struct.unpack_from("<BQ", d, i)
    ptr, size = struct.unpack_from("<QQ", d, i + 13)
    i += 13 + EVENT_DATA
    label = ""
    if d[i]:
        n, = struct.unpack_from("<I", d, i + 1)
        label = d[i + 5:i + 5 + n].decode()
        i += 4 + n
    i += 1
    name = NAMES[etype]
    counts[name] += 1
    if name == "ALLOC":
        allocs.append((label, size))
    elif name == "OP_BEGIN":
        begin[ptr] = ts
    elif name == "OP_END":
        durations.setdefault(label, []).append(ts - begin.pop(ptr))

print(dict(counts), {k: min(v) for k, v in durations.items()}, allocs)
if sys.argv[2:] == ["alloc"]:
    assert allocs == [("wq", 147456)], "expected one 147456-byte allocation of wq"
    sys.exit(0)
assert counts["GRAPH_BEGIN"] == counts["GRAPH_END"] == 2, "expected one graph per compute"
assert counts["OP_BEGIN"] == counts["OP_END"] == 6, "expected 3 ops per graph"
# 3 nodes + 2 inputs, allocated once: re-allocating the same graph reuses its memory
assert counts["ALLOC"] == 5, "expected 3 nodes + 2 leafs allocated once"
# Ops stamped around the whole graph all get the same duration; real ones differ widely.
fastest = {k: min(v) for k, v in durations.items()}
assert max(fastest.values()) > 3 * min(fastest.values()), "per-op timings look like whole-graph timings"
