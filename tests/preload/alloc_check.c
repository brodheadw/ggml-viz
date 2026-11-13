// Preload test: a context allocation must be recorded with ggml's real size (Q4_0 512x512 = 147456 bytes),
// and so must a tensor bound to a host-pointer buffer, as llama.cpp does with mmap'd weights.
#include "ggml.h"
#include "ggml-alloc.h"
#include "ggml-backend.h"
#include "ggml-cpu.h"
#include <stdlib.h>
int main(void) {
  struct ggml_init_params p = { ggml_tensor_overhead()*4, NULL, true };
  struct ggml_context* ctx = ggml_init(p);
  ggml_set_name(ggml_new_tensor_2d(ctx, GGML_TYPE_Q4_0, 512, 512), "wq");
  struct ggml_context* mctx = ggml_init(p);
  struct ggml_tensor* wk = ggml_new_tensor_1d(mctx, GGML_TYPE_F32, 1024);
  ggml_set_name(wk, "wk");
  ggml_backend_t cpu = ggml_backend_cpu_init();
  ggml_backend_buffer_t buf = ggml_backend_alloc_ctx_tensors(ctx, cpu);
  void* host = aligned_alloc(64, 4096);
  ggml_backend_buffer_t mbuf = ggml_backend_cpu_buffer_from_ptr(host, 4096);
  int rc = buf == NULL || ggml_backend_tensor_alloc(mbuf, wk, host) != GGML_STATUS_SUCCESS;
  ggml_backend_buffer_free(mbuf); free(host); ggml_free(mctx);
  ggml_backend_buffer_free(buf); ggml_backend_free(cpu); ggml_free(ctx);
  return rc;
}
