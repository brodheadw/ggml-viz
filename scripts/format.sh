#!/bin/bash

# GGML Visualizer Code Formatting Script
# Formats C++, CMake, and other source files according to project style

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🎨 Code formatting for GGML Visualizer${NC}"
echo "========================================"
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

# Try to find clang-format
CLANG_FORMAT=""
for cmd in clang-format-15 clang-format-14 clang-format-13 clang-format clang-format-12; do
    if command_exists "$cmd"; then
        CLANG_FORMAT="$cmd"
        break
    fi
done

# Check for formatting tools
TOOLS_AVAILABLE=0

if [ -n "$CLANG_FORMAT" ]; then
    echo -e "${GREEN}✅ Found clang-format: $CLANG_FORMAT${NC}"
    TOOLS_AVAILABLE=1
else
    echo -e "${YELLOW}⚠️  clang-format not found. Install with:${NC}"
    echo "   macOS: brew install clang-format"
    echo "   Ubuntu: sudo apt install clang-format"
fi

if command_exists "cmake-format"; then
    echo -e "${GREEN}✅ Found cmake-format${NC}"
else
    echo -e "${YELLOW}⚠️  cmake-format not found. Install with: pip install cmake-format${NC}"
fi

echo ""

if [ $TOOLS_AVAILABLE -eq 0 ]; then
    echo -e "${YELLOW}⚠️  No formatting tools available. Performing basic style checks...${NC}"
    echo ""
    
    # Basic style checks without external tools
    echo "🔍 Checking for common style issues..."
    
    # Check for tabs vs spaces
    TAB_FILES=$(find src/ tests/ examples/ -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" | xargs grep -l $'\t' 2>/dev/null || true)
    if [ -n "$TAB_FILES" ]; then
        echo -e "${YELLOW}⚠️  Files with tabs (should use 4 spaces):${NC}"
        echo "$TAB_FILES"
    fi
    
    # Check for trailing whitespace
    TRAILING_WS=$(find src/ tests/ examples/ -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" | xargs grep -l '[[:space:]]$' 2>/dev/null || true)
    if [ -n "$TRAILING_WS" ]; then
        echo -e "${YELLOW}⚠️  Files with trailing whitespace:${NC}"
        echo "$TRAILING_WS"
    fi
    
    echo ""
    echo -e "${BLUE}💡 Style guidelines:${NC}"
    echo "  - Use 4 spaces for indentation (no tabs)"
    echo "  - Remove trailing whitespace"
    echo "  - Use consistent brace style (opening on same line)"
    echo "  - Follow existing naming conventions"
    
    exit 0
fi

# Create .clang-format if it doesn't exist
if [ ! -f ".clang-format" ] && [ -n "$CLANG_FORMAT" ]; then
    echo "📝 Creating .clang-format configuration..."
    cat > .clang-format << 'EOF'
---
Language: Cpp
BasedOnStyle: LLVM
IndentWidth: 4
TabWidth: 4
UseTab: Never
ColumnLimit: 100
AlignAfterOpenBracket: Align
AlignConsecutiveAssignments: false
AlignConsecutiveDeclarations: false
AlignOperands: true
AlignTrailingComments: true
AllowAllParametersOfDeclarationOnNextLine: true
AllowShortBlocksOnASingleLine: false
AllowShortCaseLabelsOnASingleLine: false
AllowShortFunctionsOnASingleLine: Empty
AllowShortIfStatementsOnASingleLine: false
AllowShortLoopsOnASingleLine: false
AlwaysBreakAfterReturnType: None
AlwaysBreakBeforeMultilineStrings: false
AlwaysBreakTemplateDeclarations: false
BinPackArguments: true
BinPackParameters: true
BreakBeforeBinaryOperators: None
BreakBeforeBraces: Attach
BreakBeforeTernaryOperators: true
BreakConstructorInitializersBeforeComma: false
ConstructorInitializerAllOnOneLineOrOnePerLine: false
ConstructorInitializerIndentWidth: 4
ContinuationIndentWidth: 4
Cpp11BracedListStyle: true
DerivePointerAlignment: false
PointerAlignment: Left
SpaceAfterCStyleCast: false
SpaceBeforeAssignmentOperators: true
SpaceBeforeParens: ControlStatements
SpaceInEmptyParentheses: false
SpacesBeforeTrailingComments: 1
SpacesInAngles: false
SpacesInContainerLiterals: true
SpacesInCStyleCastParentheses: false
SpacesInParentheses: false
SpacesInSquareBrackets: false
Standard: Cpp11
EOF
    echo -e "${GREEN}✅ Created .clang-format${NC}"
fi

# Format C++ files
if [ -n "$CLANG_FORMAT" ]; then
    echo "🎨 Formatting C++ files..."
    find src/ tests/ examples/ -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" | while read -r file; do
        echo "  Formatting $file"
        "$CLANG_FORMAT" -i "$file"
    done
    echo -e "${GREEN}✅ C++ files formatted${NC}"
fi

# Format CMake files (if cmake-format available)
if command_exists "cmake-format"; then
    echo "🎨 Formatting CMake files..."
    find . -name "CMakeLists.txt" -o -name "*.cmake" | while read -r file; do
        echo "  Formatting $file"
        cmake-format -i "$file"
    done
    echo -e "${GREEN}✅ CMake files formatted${NC}"
fi

echo ""
echo -e "${GREEN}🎉 Code formatting complete!${NC}"
echo ""
echo -e "${BLUE}📋 Next steps:${NC}"
echo "  1. Review the changes with: git diff"
echo "  2. Run tests to ensure nothing broke: ./scripts/run_tests.sh"
echo "  3. Commit formatted code: git add -A && git commit -m 'Format code'"