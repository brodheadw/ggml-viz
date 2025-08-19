// cuda_wrap.cpp — interpose CUDA allocations (runtime + driver API)
#include <dlfcn.h>
#include <stdint.h>
#include <cstddef>
#include "ggml_hook.hpp"

extern ggml_viz::GGMLHook& ggml_viz_get_hook(); // or GGMLHook::instance()

// --- Minimal types to avoid <cuda*.h>
using cudaError_t = int;
using cudaStream_t = void*;
constexpr cudaError_t cudaSuccess = 0;

using CUresult = int;
using CUdeviceptr = unsigned long long;   // 64-bit device pointer
using CUstream = void*;
constexpr CUresult CUDA_SUCCESS = 0;

// --- Runtime API originals
static cudaError_t (*real_cudaMalloc)(void**, size_t) = nullptr;
static cudaError_t (*real_cudaFree)(void*) = nullptr;
static cudaError_t (*real_cudaMallocAsync)(void**, size_t, cudaStream_t) = nullptr;
static cudaError_t (*real_cudaFreeAsync)(void*, cudaStream_t) = nullptr;

// --- Driver API originals
static CUresult (*real_cuMemAlloc)(CUdeviceptr*, size_t) = nullptr;
static CUresult (*real_cuMemFree)(CUdeviceptr) = nullptr;
static CUresult (*real_cuMemAllocAsync)(CUdeviceptr*, size_t, CUstream) = nullptr;
static CUresult (*real_cuMemFreeAsync)(CUdeviceptr, CUstream) = nullptr;

static void resolve_cuda_syms_once() {
    static bool done = false; if (done) return; done = true;
    real_cudaMalloc      = (cudaError_t(*)(void**,size_t)) dlsym(RTLD_NEXT, "cudaMalloc");
    real_cudaFree        = (cudaError_t(*)(void*))         dlsym(RTLD_NEXT, "cudaFree");
    real_cudaMallocAsync = (cudaError_t(*)(void**,size_t,cudaStream_t)) dlsym(RTLD_NEXT, "cudaMallocAsync");
    real_cudaFreeAsync   = (cudaError_t(*)(void*,cudaStream_t)) dlsym(RTLD_NEXT, "cudaFreeAsync");

    real_cuMemAlloc      = (CUresult(*)(CUdeviceptr*,size_t)) dlsym(RTLD_NEXT, "cuMemAlloc");
    real_cuMemFree       = (CUresult(*)(CUdeviceptr))         dlsym(RTLD_NEXT, "cuMemFree");
    real_cuMemAllocAsync = (CUresult(*)(CUdeviceptr*,size_t,CUstream)) dlsym(RTLD_NEXT, "cuMemAllocAsync");
    real_cuMemFreeAsync  = (CUresult(*)(CUdeviceptr,CUstream))         dlsym(RTLD_NEXT, "cuMemFreeAsync");
}

// ---------------- Runtime API
extern "C" cudaError_t cudaMalloc(void **devPtr, size_t size) {
    resolve_cuda_syms_once();
    auto &hook = ggml_viz_get_hook();
    cudaError_t rc = real_cudaMalloc ? real_cudaMalloc(devPtr, size) : 1;
    if (rc == cudaSuccess && devPtr && *devPtr) {
        hook.on_backend_buffer_alloc(*devPtr, size);
    }
    return rc;
}

extern "C" cudaError_t cudaFree(void *devPtr) {
    resolve_cuda_syms_once();
    auto &hook = ggml_viz_get_hook();
    if (devPtr) hook.on_backend_buffer_free(devPtr);
    return real_cudaFree ? real_cudaFree(devPtr) : 1;
}

extern "C" cudaError_t cudaMallocAsync(void **devPtr, size_t size, cudaStream_t stream) {
    resolve_cuda_syms_once();
    auto &hook = ggml_viz_get_hook();
    cudaError_t rc = real_cudaMallocAsync ? real_cudaMallocAsync(devPtr, size, stream) : 1;
    if (rc == cudaSuccess && devPtr && *devPtr) {
        hook.on_backend_buffer_alloc(*devPtr, size);
    }
    return rc;
}

extern "C" cudaError_t cudaFreeAsync(void *devPtr, cudaStream_t stream) {
    resolve_cuda_syms_once();
    auto &hook = ggml_viz_get_hook();
    if (devPtr) hook.on_backend_buffer_free(devPtr);
    return real_cudaFreeAsync ? real_cudaFreeAsync(devPtr, stream) : 1;
}

// ---------------- Driver API
extern "C" CUresult cuMemAlloc(CUdeviceptr *dptr, size_t bytesize) {
    resolve_cuda_syms_once();
    auto &hook = ggml_viz_get_hook();
    CUresult rc = real_cuMemAlloc ? real_cuMemAlloc(dptr, bytesize) : 1;
    if (rc == CUDA_SUCCESS && dptr && *dptr) {
        // cast device pointer to void* purely as key identity
        hook.on_backend_buffer_alloc((void*)(uintptr_t)(*dptr), bytesize);
    }
    return rc;
}

extern "C" CUresult cuMemFree(CUdeviceptr dptr) {
    resolve_cuda_syms_once();
    auto &hook = ggml_viz_get_hook();
    if (dptr) hook.on_backend_buffer_free((void*)(uintptr_t)dptr);
    return real_cuMemFree ? real_cuMemFree(dptr) : 1;
}

extern "C" CUresult cuMemAllocAsync(CUdeviceptr *dptr, size_t bytesize, CUstream hStream) {
    resolve_cuda_syms_once();
    auto &hook = ggml_viz_get_hook();
    CUresult rc = real_cuMemAllocAsync ? real_cuMemAllocAsync(dptr, bytesize, hStream) : 1;
    if (rc == CUDA_SUCCESS && dptr && *dptr) {
        hook.on_backend_buffer_alloc((void*)(uintptr_t)(*dptr), bytesize);
    }
    return rc;
}

extern "C" CUresult cuMemFreeAsync(CUdeviceptr dptr, CUstream hStream) {
    resolve_cuda_syms_once();
    auto &hook = ggml_viz_get_hook();
    if (dptr) hook.on_backend_buffer_free((void*)(uintptr_t)dptr);
    return real_cuMemFreeAsync ? real_cuMemFreeAsync(dptr, hStream) : 1;
}