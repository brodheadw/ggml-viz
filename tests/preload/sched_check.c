// Preload test: run through ggml_backend_sched (the llama.cpp path) and check the hook still
// computes correctly and preserves a user eval callback. Exit code != 0 on failure.
#include "ggml.h"
#include "ggml-backend.h"
#include "ggml-cpu.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
static int user_cb_calls = 0;
static bool user_cb(struct ggml_tensor* t, bool ask, void* ud) { (void)t; (void)ud; user_cb_calls++; return ask ? strcmp(t->name, "sum") == 0 : true; }
int main(int argc, char** argv) {
  int n = 512, use_user_cb = argc > 1;
  struct ggml_init_params p = { ggml_tensor_overhead()*16 + ggml_graph_overhead(), NULL, true };
  struct ggml_context* ctx = ggml_init(p);
  struct ggml_tensor *A = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, n, n), *B = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, n, n);
  ggml_set_input(A); ggml_set_input(B);
  struct ggml_tensor* mm = ggml_mul_mat(ctx, A, B); ggml_set_name(mm, "mm");
  struct ggml_tensor* s = ggml_add(ctx, mm, mm); ggml_set_name(s, "sum");
  struct ggml_tensor* r = ggml_scale(ctx, s, 0.5f); ggml_set_name(r, "out"); ggml_set_output(r);
  struct ggml_cgraph* g = ggml_new_graph(ctx); ggml_build_forward_expand(g, r);
  ggml_backend_t cpu = ggml_backend_cpu_init();
  ggml_backend_sched_t sched = ggml_backend_sched_new(&cpu, NULL, 1, GGML_DEFAULT_GRAPH_SIZE, false, true);
  if (use_user_cb) ggml_backend_sched_set_eval_callback(sched, user_cb, NULL);
  static float a[512*512], b[512*512];
  for (int i = 0; i < n*n; i++) { a[i] = (i % 7) * 0.1f; b[i] = (i % 5) * 0.2f; }
  double e = 0; for (int k = 0; k < n; k++) e += (double)a[k] * b[k];
  int rc = 0;
  for (int it = 0; it < 2; it++) {
    ggml_backend_sched_alloc_graph(sched, g);
    ggml_backend_tensor_set(A, a, 0, sizeof(a)); ggml_backend_tensor_set(B, b, 0, sizeof(b));
    ggml_backend_sched_graph_compute(sched, g);
    float got; ggml_backend_tensor_get(r, &got, 0, sizeof(got));
    int ok = fabs(got - e) <= 1e-3 * fabs(e);
    printf("iter %d: got %f expected %f -> %s\n", it, got, e, ok ? "OK" : "WRONG"); rc |= !ok;
  }
  if (use_user_cb) { printf("user_cb_calls=%d\n", user_cb_calls); rc |= user_cb_calls == 0; }
  ggml_backend_sched_free(sched); ggml_backend_free(cpu); ggml_free(ctx);
  return rc;
}
