/**
 * Windows interposition implementation using MinHook
 * 
 * This implementation uses Microsoft Detours or MinHook to intercept GGML function calls
 * on Windows. Unlike Linux/macOS which use symbol interposition, Windows requires
 * runtime API hooking to achieve the same functionality.
 */

#include "ggml_hook.hpp"
#include "../ipc/ipc_common.hpp"
#include <windows.h>
#include <iostream>
#include <memory>
#include <string>
#include <mutex>
#include <atomic>

// MinHook headers (will be included via vcpkg or similar)
#ifdef USE_MINHOOK
#include <MinHook.h>
#endif

// Forward declarations for GGML types
extern "C" {
    struct ggml_cgraph;
    struct ggml_backend_sched;
    typedef struct ggml_backend_sched ggml_backend_sched_t;
    
    enum ggml_status {
        GGML_STATUS_SUCCESS = 0,
        GGML_STATUS_FAILED = 1,
        GGML_STATUS_ABORTED = 2,
    };
}

namespace ggml_viz {

// Global state for Windows interposition
static std::atomic<bool> g_hooks_initialized{false};
static std::mutex g_init_mutex;
static std::unique_ptr<SharedMemoryRegion> g_shared_memory;
static std::string g_output_file;
static bool g_verbose = false;

// Function pointer type for the original GGML function
typedef enum ggml_status (*ggml_backend_sched_graph_compute_fn)(
    ggml_backend_sched_t sched, struct ggml_cgraph * graph);

// Storage for original function pointer
static ggml_backend_sched_graph_compute_fn original_ggml_backend_sched_graph_compute = nullptr;

// Hook function that wraps the original GGML function
static enum ggml_status hooked_ggml_backend_sched_graph_compute(
    ggml_backend_sched_t sched, struct ggml_cgraph * graph) {
    
    // Initialize hooks if not done yet
    if (!g_hooks_initialized.load()) {
        initialize_hooks();
    }
    
    if (g_verbose) {
        std::cout << "ggml-viz: Intercepted ggml_backend_sched_graph_compute call" << std::endl;
    }
    
    // Record begin event
    if (g_shared_memory) {
        // Create and write begin event (similar to Linux version)
        // This is a simplified version - full implementation would match linux_interpose.cpp
        auto now = std::chrono::high_resolution_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();
        
        // For now, just log the event
        if (g_verbose) {
            std::cout << "ggml-viz: Recording graph compute begin event at " << timestamp << std::endl;
        }
    }
    
    // Call the original function
    if (!original_ggml_backend_sched_graph_compute) {
        std::cerr << "ggml-viz: Error: Original function not found" << std::endl;
        return GGML_STATUS_FAILED;
    }
    
    enum ggml_status result = original_ggml_backend_sched_graph_compute(sched, graph);
    
    // Record end event
    if (g_shared_memory) {
        auto now = std::chrono::high_resolution_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();
        
        if (g_verbose) {
            std::cout << "ggml-viz: Recording graph compute end event at " << timestamp << std::endl;
        }
    }
    
    return result;
}

// Initialize the hooking system
bool initialize_hooks() {
    std::lock_guard<std::mutex> lock(g_init_mutex);
    
    if (g_hooks_initialized.load()) {
        return true;
    }
    
    // Check environment variables
    const char* output_file_env = std::getenv("GGML_VIZ_OUTPUT");
    if (output_file_env) {
        g_output_file = output_file_env;
    }
    
    const char* verbose_env = std::getenv("GGML_VIZ_VERBOSE");
    if (verbose_env && std::string(verbose_env) == "1") {
        g_verbose = true;
    }
    
    if (g_verbose) {
        std::cout << "ggml-viz: Initializing Windows hooks..." << std::endl;
    }
    
#ifdef USE_MINHOOK
    // Initialize MinHook
    if (MH_Initialize() != MH_OK) {
        std::cerr << "ggml-viz: Failed to initialize MinHook" << std::endl;
        return false;
    }
    
    // Find the target function in the current process
    HMODULE ggml_module = GetModuleHandleA("ggml.dll");
    if (!ggml_module) {
        // Try to find it in the main executable
        ggml_module = GetModuleHandleA(NULL);
    }
    
    if (!ggml_module) {
        std::cerr << "ggml-viz: Failed to find GGML module" << std::endl;
        MH_Uninitialize();
        return false;
    }
    
    // Get the address of the target function
    void* target_func = GetProcAddress(ggml_module, "ggml_backend_sched_graph_compute");
    if (!target_func) {
        std::cerr << "ggml-viz: Failed to find ggml_backend_sched_graph_compute function" << std::endl;
        MH_Uninitialize();
        return false;
    }
    
    // Create the hook
    if (MH_CreateHook(target_func, 
                     reinterpret_cast<void*>(hooked_ggml_backend_sched_graph_compute),
                     reinterpret_cast<void**>(&original_ggml_backend_sched_graph_compute)) != MH_OK) {
        std::cerr << "ggml-viz: Failed to create hook" << std::endl;
        MH_Uninitialize();
        return false;
    }
    
    // Enable the hook
    if (MH_EnableHook(target_func) != MH_OK) {
        std::cerr << "ggml-viz: Failed to enable hook" << std::endl;
        MH_Uninitialize();
        return false;
    }
    
    if (g_verbose) {
        std::cout << "ggml-viz: Successfully installed MinHook" << std::endl;
    }
#else
    // Fallback implementation without MinHook
    std::cerr << "ggml-viz: Warning: MinHook not available, hooks will not work" << std::endl;
    return false;
#endif
    
    // Initialize shared memory for IPC
    if (!g_output_file.empty()) {
        const size_t shm_size = 64 * 1024 * 1024; // 64MB ring buffer
        g_shared_memory = SharedMemoryRegion::create("ggml_viz_events", shm_size);
        
        if (!g_shared_memory) {
            std::cerr << "ggml-viz: Failed to create shared memory region" << std::endl;
            return false;
        }
        
        if (g_verbose) {
            std::cout << "ggml-viz: Created shared memory region: " << shm_size << " bytes" << std::endl;
        }
    }
    
    g_hooks_initialized.store(true);
    
    if (g_verbose) {
        std::cout << "ggml-viz: Windows hooks initialized successfully" << std::endl;
    }
    
    return true;
}

// Cleanup function called when DLL is unloaded
void cleanup_hooks() {
    std::lock_guard<std::mutex> lock(g_init_mutex);
    
    if (!g_hooks_initialized.load()) {
        return;
    }
    
    if (g_verbose) {
        std::cout << "ggml-viz: Cleaning up Windows hooks..." << std::endl;
    }
    
#ifdef USE_MINHOOK
    // Disable and remove all hooks
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
#endif
    
    // Clean up shared memory
    g_shared_memory.reset();
    
    g_hooks_initialized.store(false);
    
    if (g_verbose) {
        std::cout << "ggml-viz: Windows hooks cleaned up" << std::endl;
    }
}

} // namespace ggml_viz

// DLL entry point for Windows
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            // Initialize hooks when DLL is loaded
            ggml_viz::initialize_hooks();
            break;
            
        case DLL_PROCESS_DETACH:
            // Clean up when DLL is unloaded
            ggml_viz::cleanup_hooks();
            break;
            
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            // No per-thread initialization needed
            break;
    }
    return TRUE;
}

// Export functions for manual initialization (optional)
extern "C" {
    __declspec(dllexport) bool ggml_viz_initialize() {
        return ggml_viz::initialize_hooks();
    }
    
    __declspec(dllexport) void ggml_viz_cleanup() {
        ggml_viz::cleanup_hooks();
    }
}