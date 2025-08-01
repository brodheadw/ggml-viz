#if defined(_WIN32)
#include "ipc_common.hpp"
#include <windows.h>
#include <stdexcept>
#include <string>

namespace ggml_viz {

class WinSharedMemory final : public SharedMemoryRegion {
public:
    ~WinSharedMemory() override {
        if (view_)  UnmapViewOfFile(view_);
        if (hmap_)  CloseHandle(hmap_);
    }

    bool write(const void* data, size_t nbytes) override;
    bool read(void* dest, size_t nbytes) override;
    void*  get_address() const override          { return view_; }
    size_t get_size() const override             { return map_size_; }
    bool   is_valid() const override             { return view_ != nullptr; }

    static std::unique_ptr<SharedMemoryRegion>
    createImpl(std::wstring_view name, size_t size_bytes, bool create);

private:
    WinSharedMemory() = default;
    HANDLE hmap_ = nullptr;
    void*  view_ = nullptr;
    size_t map_size_{0};
    
    // Ring buffer operations
    uint32_t available_space() const override;
    uint32_t available_data() const override;
};

// -------- factory ----------------------------------------------------------
std::unique_ptr<SharedMemoryRegion>
SharedMemoryRegion::create(const std::string& name, size_t sz) {
    // convert UTF-8 → UTF-16
    std::wstring wname(name.begin(), name.end());
    return WinSharedMemory::createImpl(wname, sz, true);
}

std::unique_ptr<SharedMemoryRegion>
SharedMemoryRegion::open(const std::string& name, size_t sz) {
    std::wstring wname(name.begin(), name.end());
    return WinSharedMemory::createImpl(wname, sz, false);
}

std::unique_ptr<SharedMemoryRegion>
WinSharedMemory::createImpl(std::wstring_view name, size_t sz, bool create) {
    const DWORD protect = PAGE_READWRITE;
    const DWORD access  = FILE_MAP_ALL_ACCESS;
    size_t map_size = sizeof(RingHeader) + sz;

    HANDLE h = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        nullptr,
        protect,
        static_cast<DWORD>(map_size >> 32),
        static_cast<DWORD>(map_size & 0xffffffff),
        name.data()
    );
    if (!h)  throw std::runtime_error("CreateFileMapping failed");

    if (!create && GetLastError() == ERROR_FILE_NOT_FOUND)
        throw std::runtime_error("shared-memory segment doesn't exist");

    void* v = MapViewOfFile(h, access, 0, 0, map_size);
    if (!v) {
        CloseHandle(h);
        throw std::runtime_error("MapViewOfFile failed");
    }

    if (create && GetLastError() != ERROR_ALREADY_EXISTS) {
        auto* hdr = static_cast<RingHeader*>(v);
        
        // Verify atomics are lock-free for shared memory safety
        if (!hdr->head.is_lock_free() || !hdr->tail.is_lock_free()) {
            UnmapViewOfFile(v);
            CloseHandle(h);
            throw std::runtime_error("Atomic operations not lock-free - shared memory ring buffer unsafe");
        }
        
        hdr->head.store(0, std::memory_order_relaxed);
        hdr->tail.store(0, std::memory_order_relaxed);
        hdr->capacity = static_cast<uint32_t>(sz);
    }

    auto ptr = std::unique_ptr<WinSharedMemory>(new WinSharedMemory());
    ptr->hmap_ = h;
    ptr->view_ = v;
    ptr->map_size_ = map_size;
    return ptr;
}

// -------- write/read (same logic as POSIX, using atomics) ------------------
bool WinSharedMemory::write(const void* data, size_t n) {
    auto* hdr = static_cast<RingHeader*>(view_);
    uint32_t head = hdr->head.load(std::memory_order_relaxed);
    uint32_t tail = hdr->tail.load(std::memory_order_acquire);
    uint32_t cap  = hdr->capacity;

    uint32_t free_space = (tail + cap - head - 1) & (cap - 1);
    if (n > free_space) return false;

    auto* base = static_cast<uint8_t*>(view_) + sizeof(RingHeader);
    for (size_t i = 0; i < n; ++i)
        base[(head + i) & (cap - 1)] = static_cast<const uint8_t*>(data)[i];

    hdr->head.store((head + n) & (cap - 1), std::memory_order_release);
    return true;
}

bool WinSharedMemory::read(void* dest, size_t n) {
    auto* hdr = static_cast<RingHeader*>(view_);
    uint32_t head = hdr->head.load(std::memory_order_acquire);
    uint32_t tail = hdr->tail.load(std::memory_order_relaxed);
    uint32_t cap  = hdr->capacity;

    uint32_t avail = (head + cap - tail) & (cap - 1);
    if (avail < n) {
        return false;  // Not enough data available
    }

    const auto* base = static_cast<const uint8_t*>(view_) + sizeof(RingHeader);
    for (size_t i = 0; i < n; ++i)
        static_cast<uint8_t*>(dest)[i] = base[(tail + i) & (cap - 1)];

    hdr->tail.store((tail + n) & (cap - 1), std::memory_order_release);
    return true;
}

uint32_t WinSharedMemory::available_space() const {
    auto* hdr = static_cast<RingHeader*>(view_);
    uint32_t head = hdr->head.load(std::memory_order_relaxed);
    uint32_t tail = hdr->tail.load(std::memory_order_acquire);
    uint32_t cap  = hdr->capacity;
    
    return (tail + cap - head - 1) & (cap - 1);
}

uint32_t WinSharedMemory::available_data() const {
    auto* hdr = static_cast<RingHeader*>(view_);
    uint32_t tail = hdr->tail.load(std::memory_order_relaxed);
    uint32_t head = hdr->head.load(std::memory_order_acquire);
    uint32_t cap  = hdr->capacity;
    
    return (head + cap - tail) & (cap - 1);
}

} // namespace ggml_viz
#endif // _WIN32