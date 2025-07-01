# CMake generated Testfile for 
# Source directory: /Users/willb/Vaults/Personal/DevLab/ggml/ggml-viz/tests
# Build directory: /Users/willb/Vaults/Personal/DevLab/ggml/ggml-viz/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(ggml_hook_basic "/Users/willb/Vaults/Personal/DevLab/ggml/ggml-viz/bin/test_ggml_hook")
set_tests_properties(ggml_hook_basic PROPERTIES  _BACKTRACE_TRIPLES "/Users/willb/Vaults/Personal/DevLab/ggml/ggml-viz/tests/CMakeLists.txt;39;add_test;/Users/willb/Vaults/Personal/DevLab/ggml/ggml-viz/tests/CMakeLists.txt;0;")
add_test(trace_reader_basic "/Users/willb/Vaults/Personal/DevLab/ggml/ggml-viz/bin/test_trace_reader" "/Users/willb/Vaults/Personal/DevLab/ggml/ggml-viz/test_trace.ggmlviz")
set_tests_properties(trace_reader_basic PROPERTIES  _BACKTRACE_TRIPLES "/Users/willb/Vaults/Personal/DevLab/ggml/ggml-viz/tests/CMakeLists.txt;54;add_test;/Users/willb/Vaults/Personal/DevLab/ggml/ggml-viz/tests/CMakeLists.txt;0;")
