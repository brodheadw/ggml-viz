#include <windows.h>
#include <minhook.h>
#include <cstdio>

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(hMod);

    MH_Initialize();
    printf("[ggml-viz] Win32 hook loaded (PID %lu)\n", GetCurrentProcessId());
    // TODO: MH_CreateHook for ggml_backend_sched_graph_compute
    return TRUE;
}