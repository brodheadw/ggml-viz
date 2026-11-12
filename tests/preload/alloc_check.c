// Preload test: a context allocation must be recorded with ggml's real size (Q4_0 512x512 = 147456 bytes).
#include "ggml.h"
#include "ggml-alloc.h"
#include "ggml-backend.h"
#include "ggml-cpu.h"
int main(void) {
  struct ggml_init_params p = { ggml_tensor_overhead()*4, NULL, true };
  struct ggml_context* ctx = ggml_init(p);
  ggml_set_name(ggml_new_tensor_2d(ctx, GGML_TYPE_Q4_0, 512, 512), "wq");
  ggml_backend_t cpu = ggml_backend_cpu_init();
  ggml_backend_buffer_t buf = ggml_backend_alloc_ctx_tensors(ctx, cpu);
  int rc = buf == NULL;
  ggml_backend_buffer_free(buf); ggml_backend_free(cpu); ggml_free(ctx);
  return rc;
}
