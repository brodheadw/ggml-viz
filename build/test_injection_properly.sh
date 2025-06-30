#!/bin/bash

echo "=== 🧪 Testing GGML Injection with Real File Output ==="

# Clean up any existing trace files
rm -f injection_test.ggmlviz

# Set environment for file output
export GGML_VIZ_OUTPUT="injection_test.ggmlviz"
export GGML_VIZ_VERBOSE=1

echo ""
echo "1️⃣ Testing injection with our built test program..."
../scripts/inject_macos.sh ./bin/test_trace_reader

echo ""
echo "2️⃣ Checking if trace file was created..."
if [ -f "injection_test.ggmlviz" ]; then
    echo "   ✅ Trace file created: injection_test.ggmlviz"
    echo "   📊 File size: $(stat -f%z injection_test.ggmlviz) bytes"
    
    echo ""
    echo "3️⃣ Examining file contents..."
    echo "   Header (hex): $(hexdump -C injection_test.ggmlviz | head -2)"
    
    echo ""
    echo "4️⃣ Testing file with GUI..."
    echo "   🖥 Loading in ggml-viz..."
    echo "   Command: ./bin/ggml-viz injection_test.ggmlviz"
    echo "   (You can run this command to see the trace in the GUI)"
    
else
    echo "   ❌ No trace file created"
    echo "   This suggests the injection hooks aren't being triggered properly"
fi

echo ""
echo "5️⃣ Now testing with a simple GGML operation program..."

# Create a minimal GGML test program that actually does operations
cat > minimal_ggml_test.cpp << 'EOF'
#include <stdio.h>
#include <ggml.h>

int main() {
    printf("Creating minimal GGML operations...\n");
    
    // Initialize GGML context
    struct ggml_init_params params = {
        .mem_size   = 16*1024*1024,  // 16 MB
        .mem_buffer = NULL,
        .no_alloc   = false,
    };
    
    struct ggml_context* ctx = ggml_init(params);
    if (!ctx) {
        printf("Failed to initialize GGML context\n");
        return 1;
    }
    
    printf("GGML context initialized\n");
    
    // Create some tensors
    struct ggml_tensor* a = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 4, 4);
    struct ggml_tensor* b = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 4, 4);
    
    printf("Tensors created: a=%p, b=%p\n", (void*)a, (void*)b);
    
    // Perform some operations
    struct ggml_tensor* sum = ggml_add(ctx, a, b);
    struct ggml_tensor* mul = ggml_mul(ctx, a, b);
    
    printf("Operations created: sum=%p, mul=%p\n", (void*)sum, (void*)mul);
    
    // Build computation graph
    struct ggml_cgraph* gf = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf, sum);
    ggml_build_forward_expand(gf, mul);
    
    printf("Graph built with %d nodes\n", gf->n_nodes);
    
    // This should trigger our hooks if injection is working
    ggml_graph_compute_with_ctx(ctx, gf, 1);
    
    printf("Graph computation complete\n");
    
    ggml_free(ctx);
    printf("GGML context freed\n");
    
    return 0;
}
EOF

echo "   📝 Created minimal_ggml_test.cpp"

# Compile the minimal test
echo "   🔨 Compiling minimal GGML test..."
g++ -std=c++17 -I ../third_party/ggml/include -I ../third_party/ggml/src \
    minimal_ggml_test.cpp -L ../third_party/ggml/src -lggml -lggml-cpu -lggml-base \
    -o minimal_ggml_test 2>/dev/null

if [ -f "minimal_ggml_test" ]; then
    echo "   ✅ Compilation successful"
    
    echo ""
    echo "6️⃣ Testing injection with real GGML operations..."
    rm -f minimal_test.ggmlviz
    export GGML_VIZ_OUTPUT="minimal_test.ggmlviz"
    
    ../scripts/inject_macos.sh ./minimal_ggml_test
    
    echo ""
    echo "7️⃣ Checking results..."
    if [ -f "minimal_test.ggmlviz" ]; then
        echo "   ✅ Trace file created: minimal_test.ggmlviz"
        echo "   📊 File size: $(stat -f%z minimal_test.ggmlviz) bytes"
        
        # Check if file has events (more than just header)
        SIZE=$(stat -f%z minimal_test.ggmlviz)
        if [ "$SIZE" -gt 12 ]; then
            echo "   🎉 SUCCESS! File contains data beyond header"
            echo "   📁 Open with: ./bin/ggml-viz minimal_test.ggmlviz"
        else
            echo "   ⚠️  File only contains header (no events captured)"
        fi
    else
        echo "   ❌ No trace file created"
    fi
else
    echo "   ❌ Compilation failed - may need to link GGML libraries"
fi

echo ""
echo "🏁 Test complete!"
echo "   If you see SUCCESS above, the injection system is working!"
echo "   You can open any .ggmlviz files with: ./bin/ggml-viz <filename>"