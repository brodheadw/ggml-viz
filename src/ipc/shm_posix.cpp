#if !defined(_WIN32)
#include "ipc_common.hpp"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>
#include <string>

namespace ggml_viz {

class PosixSharedMemory final : public SharedMemoryRegion {
public:
    ~PosixSharedMemory() override {
        if (view_ && view_ != MAP_FAILED) munmap(view_, map_size_);
        if (fd_ != -1) close(fd_);
        if (is_creator_) shm_unlink(name_.c_str());
    }

    bool   write(const void* data, size_t nbytes) override;
    bool   read (void* dest,       size_t nbytes) override;
    void*  get_address() const override { return view_; }
    size_t get_size() const override    { return map_size_; }
    bool   is_valid() const override    { return view_ != MAP_FAILED; }

    static std::unique_ptr<SharedMemoryRegion>
    createImpl(const std::string& name, size_t size_bytes, bool create);

private:
    PosixSharedMemory() = default;
    friend std::unique_ptr<PosixSharedMemory> std::make_unique<PosixSharedMemory>();
    int    fd_ = -1;
    void*  view_ = MAP_FAILED;
    size_t map_size_{0};
    std::string name_;
    bool   is_creator_{false};
    
    // Ring buffer operations
    uint32_t available_space() const override;
    uint32_t available_data() const override;
};

// -------- factory ----------------------------------------------------------
std::unique_ptr<SharedMemoryRegion>
SharedMemoryRegion::create(const std::string& name, size_t sz) {
    return PosixSharedMemory::createImpl(name, sz, true);
}

std::unique_ptr<SharedMemoryRegion>
SharedMemoryRegion::open(const std::string& name, size_t sz) {
    return PosixSharedMemory::createImpl(name, sz, false);
}

std::unique_ptr<SharedMemoryRegion>
PosixSharedMemory::createImpl(const std::string& name, size_t sz, bool create) {
    std::string shm_name = "/ggml_viz_" + name;
    size_t map_size = sizeof(RingHeader) + sz;

    int fd = create ? 
        shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666) :
        shm_open(shm_name.c_str(), O_RDWR, 0666);
    
    if (fd == -1) {
        throw std::runtime_error(create ? "shm_open create failed" : "shm_open open failed");
    }

    if (create && ftruncate(fd, map_size) == -1) {
        close(fd);
        shm_unlink(shm_name.c_str());
        throw std::runtime_error("ftruncate failed");
    }

    void* v = mmap(nullptr, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (v == MAP_FAILED) {
        close(fd);
        if (create) shm_unlink(shm_name.c_str());
        throw std::runtime_error("mmap failed");
    }

    if (create) {
        auto* hdr = static_cast<RingHeader*>(v);
        hdr->head.store(0, std::memory_order_relaxed);
        hdr->tail.store(0, std::memory_order_relaxed);
        hdr->capacity = static_cast<uint32_t>(sz);
    }

    auto ptr = std::make_unique<PosixSharedMemory>();
    ptr->fd_ = fd;
    ptr->view_ = v;
    ptr->map_size_ = map_size;
    ptr->name_ = shm_name;
    ptr->is_creator_ = create;
    return ptr;
}

// -------- write/read (same logic as Windows, using atomics) ----------------
bool PosixSharedMemory::write(const void* data, size_t n) {
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

bool PosixSharedMemory::read(void* dest, size_t n) {
    auto* hdr = static_cast<RingHeader*>(view_);
    uint32_t head = hdr->head.load(std::memory_order_acquire);
    uint32_t tail = hdr->tail.load(std::memory_order_relaxed);
    uint32_t cap  = hdr->capacity;

    uint32_t avail = (head + cap - tail) & (cap - 1);
    if (n > avail) return false;

    auto* base = static_cast<uint8_t*>(view_) + sizeof(RingHeader);
    for (size_t i = 0; i < n; ++i)
        static_cast<uint8_t*>(dest)[i] = base[(tail + i) & (cap - 1)];

    hdr->tail.store((tail + n) & (cap - 1), std::memory_order_release);
    return true;
}

uint32_t PosixSharedMemory::available_space() const {
    auto* hdr = static_cast<RingHeader*>(view_);
    uint32_t head = hdr->head.load(std::memory_order_relaxed);
    uint32_t tail = hdr->tail.load(std::memory_order_acquire);
    uint32_t cap  = hdr->capacity;
    
    return (tail + cap - head - 1) & (cap - 1);
}

uint32_t PosixSharedMemory::available_data() const {
    auto* hdr = static_cast<RingHeader*>(view_);
    uint32_t tail = hdr->tail.load(std::memory_order_relaxed);
    uint32_t head = hdr->head.load(std::memory_order_acquire);
    uint32_t cap  = hdr->capacity;
    
    return (head + cap - tail) & (cap - 1);
}

} // namespace ggml_viz
#endif // !_WIN32