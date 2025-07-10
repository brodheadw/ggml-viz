#!/bin/bash

# GGML Visualizer Code Linting Script
# Performs static analysis on C++, CMake, and documentation files

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🔍 Code linting for GGML Visualizer${NC}"
echo "====================================="
echo ""

cd "$PROJECT_ROOT"

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo -e "${RED}❌ ERROR: Please run this script from the project root directory${NC}"
    exit 1
fi

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Initialize counters
ISSUES_FOUND=0
TOOLS_AVAILABLE=0

# Check for linting tools
CLANG_TIDY=""
for cmd in clang-tidy-15 clang-tidy-14 clang-tidy-13 clang-tidy clang-tidy-12; do
    if command_exists "$cmd"; then
        CLANG_TIDY="$cmd"
        break
    fi
done

if [ -n "$CLANG_TIDY" ]; then
    echo -e "${GREEN}✅ Found clang-tidy: $CLANG_TIDY${NC}"
    TOOLS_AVAILABLE=1
else
    echo -e "${YELLOW}⚠️  clang-tidy not found. Install with:${NC}"
    echo "   macOS: brew install llvm (then use /opt/homebrew/bin/clang-tidy)"
    echo "   Ubuntu: sudo apt install clang-tidy"
fi

if command_exists "cppcheck"; then
    echo -e "${GREEN}✅ Found cppcheck${NC}"
    TOOLS_AVAILABLE=1
else
    echo -e "${YELLOW}⚠️  cppcheck not found. Install with:${NC}"
    echo "   macOS: brew install cppcheck"
    echo "   Ubuntu: sudo apt install cppcheck"
fi

echo ""

# Basic static analysis (always available)
echo -e "${BLUE}🔍 Basic Static Analysis${NC}"
echo "========================"

# Check for common issues
echo "Checking for common code issues..."

# Check for TODO/FIXME comments
TODO_COUNT=$(find src/ tests/ examples/ -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" | xargs grep -n "TODO\|FIXME" 2>/dev/null | wc -l || echo 0)
if [ "$TODO_COUNT" -gt 0 ]; then
    echo -e "${YELLOW}⚠️  Found $TODO_COUNT TODO/FIXME comments${NC}"
    find src/ tests/ examples/ -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" | xargs grep -n "TODO\|FIXME" 2>/dev/null | head -5
    if [ "$TODO_COUNT" -gt 5 ]; then
        echo "   ... and $((TODO_COUNT - 5)) more"
    fi
fi

# Check for potential memory issues
MALLOC_COUNT=$(find src/ tests/ examples/ -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" | xargs grep -n "malloc\|free\|new\|delete" 2>/dev/null | wc -l || echo 0)
if [ "$MALLOC_COUNT" -gt 0 ]; then
    echo -e "${BLUE}ℹ️  Found $MALLOC_COUNT manual memory management calls${NC}"
    echo "   Consider using smart pointers where possible"
fi

# Check for potential security issues
UNSAFE_FUNCS=$(find src/ tests/ examples/ -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" | xargs grep -n "strcpy\|strcat\|sprintf\|gets" 2>/dev/null | wc -l || echo 0)
if [ "$UNSAFE_FUNCS" -gt 0 ]; then
    echo -e "${RED}⚠️  Found $UNSAFE_FUNCS potentially unsafe function calls${NC}"
    find src/ tests/ examples/ -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" | xargs grep -n "strcpy\|strcat\|sprintf\|gets" 2>/dev/null
    ISSUES_FOUND=$((ISSUES_FOUND + UNSAFE_FUNCS))
fi

# Check for include guard consistency
HEADER_FILES=$(find src/ tests/ examples/ -name "*.hpp" -o -name "*.h")
if [ -n "$HEADER_FILES" ]; then
    echo "Checking header files for include guards..."
    for header in $HEADER_FILES; do
        if ! grep -q "#pragma once\|#ifndef.*_H\|#ifndef.*_HPP" "$header" 2>/dev/null; then
            echo -e "${YELLOW}⚠️  Missing include guard: $header${NC}"
            ISSUES_FOUND=$((ISSUES_FOUND + 1))
        fi
    done
fi

echo ""

# Run cppcheck if available
if command_exists "cppcheck"; then
    echo -e "${BLUE}🔍 Running cppcheck${NC}"
    echo "=================="
    
    if cppcheck --enable=warning,style,performance,portability --std=c++17 \
               --suppress=missingIncludeSystem \
               --suppress=unusedFunction \
               --quiet \
               src/ tests/ examples/ 2>&1 | tee /tmp/cppcheck.log; then
        CPPCHECK_ISSUES=$(wc -l < /tmp/cppcheck.log)
        if [ "$CPPCHECK_ISSUES" -gt 0 ]; then
            echo -e "${YELLOW}⚠️  cppcheck found $CPPCHECK_ISSUES issues${NC}"
            ISSUES_FOUND=$((ISSUES_FOUND + CPPCHECK_ISSUES))
        else
            echo -e "${GREEN}✅ No cppcheck issues found${NC}"
        fi
    fi
    echo ""
fi

# Run clang-tidy if available and build directory exists
if [ -n "$CLANG_TIDY" ] && [ -d "build" ]; then
    echo -e "${BLUE}🔍 Running clang-tidy${NC}"
    echo "=================="
    
    # Create .clang-tidy config if it doesn't exist
    if [ ! -f ".clang-tidy" ]; then
        echo "📝 Creating .clang-tidy configuration..."
        cat > .clang-tidy << 'EOF'
---
Checks: >
  clang-diagnostic-*,
  clang-analyzer-*,
  cppcoreguidelines-*,
  modernize-*,
  performance-*,
  readability-*,
  -readability-magic-numbers,
  -cppcoreguidelines-avoid-magic-numbers,
  -modernize-use-trailing-return-type,
  -readability-braces-around-statements,
  -cppcoreguidelines-avoid-c-arrays,
  -modernize-avoid-c-arrays
WarningsAsErrors: ''
HeaderFilterRegex: '.*'
FormatStyle: file
EOF
        echo -e "${GREEN}✅ Created .clang-tidy${NC}"
    fi
    
    # Run clang-tidy on source files
    TIDY_ISSUES=0
    for file in $(find src/ -name "*.cpp" | head -5); do  # Limit to first 5 files for speed
        echo "  Analyzing $file..."
        if ! "$CLANG_TIDY" "$file" --extra-arg=-std=c++17 \
                            --extra-arg=-I"$PROJECT_ROOT/src" \
                            --extra-arg=-I"$PROJECT_ROOT/third_party/ggml/include" \
                            --extra-arg=-I"$PROJECT_ROOT/third_party/imgui" \
                            --quiet > /tmp/clang-tidy-$$.log 2>&1; then
            FILE_ISSUES=$(wc -l < /tmp/clang-tidy-$$.log)
            if [ "$FILE_ISSUES" -gt 0 ]; then
                echo -e "${YELLOW}⚠️  Found $FILE_ISSUES issues in $file${NC}"
                head -3 /tmp/clang-tidy-$$.log
                TIDY_ISSUES=$((TIDY_ISSUES + FILE_ISSUES))
            fi
        fi
    done
    
    if [ "$TIDY_ISSUES" -gt 0 ]; then
        echo -e "${YELLOW}⚠️  clang-tidy found $TIDY_ISSUES total issues${NC}"
        ISSUES_FOUND=$((ISSUES_FOUND + TIDY_ISSUES))
    else
        echo -e "${GREEN}✅ No clang-tidy issues found${NC}"
    fi
    echo ""
elif [ -n "$CLANG_TIDY" ]; then
    echo -e "${YELLOW}⚠️  Build directory not found. Run 'mkdir build && cd build && cmake ..' first${NC}"
fi

# CMake linting
echo -e "${BLUE}🔍 CMake Analysis${NC}"
echo "================"

CMAKE_FILES=$(find . -name "CMakeLists.txt" -o -name "*.cmake")
for cmake_file in $CMAKE_FILES; do
    # Check for common CMake issues
    if grep -q "aux_source_directory\|GLOB\*" "$cmake_file" 2>/dev/null; then
        echo -e "${YELLOW}⚠️  $cmake_file: Consider explicit file lists instead of GLOB${NC}"
        ISSUES_FOUND=$((ISSUES_FOUND + 1))
    fi
    
    if ! grep -q "cmake_minimum_required" "$cmake_file" 2>/dev/null && [ "$(basename "$cmake_file")" = "CMakeLists.txt" ]; then
        echo -e "${YELLOW}⚠️  $cmake_file: Missing cmake_minimum_required${NC}"
        ISSUES_FOUND=$((ISSUES_FOUND + 1))
    fi
done

echo ""

# Documentation checks
echo -e "${BLUE}🔍 Documentation Analysis${NC}"
echo "========================="

# Check for broken links in README
if [ -f "README.md" ]; then
    BROKEN_LINKS=$(grep -o '\[.*\](.*\.md)' README.md | sed 's/.*(\(.*\)).*/\1/' | while read -r link; do
        if [ ! -f "$link" ]; then
            echo "$link"
        fi
    done | wc -l)
    
    if [ "$BROKEN_LINKS" -gt 0 ]; then
        echo -e "${YELLOW}⚠️  Found $BROKEN_LINKS potentially broken documentation links${NC}"
    fi
fi

echo ""

# Summary
echo -e "${BLUE}📊 Linting Summary${NC}"
echo "=================="

if [ $ISSUES_FOUND -eq 0 ]; then
    echo -e "${GREEN}🎉 No significant issues found!${NC}"
    echo ""
    echo -e "${BLUE}💡 Recommendations:${NC}"
    echo "  - Review TODO/FIXME comments and address them"
    echo "  - Consider using smart pointers for memory management"
    echo "  - Keep up the good work!"
else
    echo -e "${YELLOW}⚠️  Found $ISSUES_FOUND issues that should be addressed${NC}"
    echo ""
    echo -e "${BLUE}📋 Next steps:${NC}"
    echo "  1. Address security issues (unsafe functions) first"
    echo "  2. Add missing include guards to headers"
    echo "  3. Review and fix other reported issues"
    echo "  4. Re-run linting: ./scripts/lint.sh"
fi

echo ""

if [ $TOOLS_AVAILABLE -eq 0 ]; then
    echo -e "${YELLOW}💡 Install more tools for comprehensive analysis:${NC}"
    echo "   brew install llvm cppcheck          # macOS"
    echo "   sudo apt install clang-tidy cppcheck  # Ubuntu"
fi

# Clean up temp files
rm -f /tmp/cppcheck.log /tmp/clang-tidy-*.log

exit $ISSUES_FOUND