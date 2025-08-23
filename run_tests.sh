#!/bin/bash

# Star Voyager Test Runner Script
# Builds and runs the test suite with various options

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_usage() {
    echo "Star Voyager Test Runner"
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  --build-only    Build tests without running"
    echo "  --headless      Run tests without GUI"
    echo "  --quick         Run only quick tests"
    echo "  --clean         Clean and rebuild"
    echo "  --help          Show this help"
}

BUILD_ONLY=false
HEADLESS=false
QUICK=false
CLEAN=false

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --build-only)
            BUILD_ONLY=true
            shift
            ;;
        --headless)
            HEADLESS=true
            shift
            ;;
        --quick)
            QUICK=true
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --help)
            print_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

echo -e "${YELLOW}Star Voyager Test Suite${NC}"
echo "======================="

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo -e "${YELLOW}Cleaning...${NC}"
    make clean || true
    cd tests
    make clean || true
    cd ..
fi

# Build main project first
echo -e "${YELLOW}Building main project...${NC}"
if ! make; then
    echo -e "${RED}Failed to build main project${NC}"
    exit 1
fi

# Build tests
echo -e "${YELLOW}Building tests...${NC}"
if ! cd tests; then
    echo -e "${RED}Failed to change to tests directory${NC}"
    exit 1
fi
if ! make; then
    echo -e "${RED}Failed to build tests${NC}"
    exit 1
fi

if [ "$BUILD_ONLY" = true ]; then
    echo -e "${GREEN}Build completed successfully${NC}"
    exit 0
fi

# Run tests
echo -e "${YELLOW}Running tests...${NC}"

if [ "$HEADLESS" = true ] && [ "$QUICK" = true ]; then
    TEST_CMD="./starvoyager-tests --headless --quick"
elif [ "$HEADLESS" = true ]; then
    TEST_CMD="./starvoyager-tests --headless"
elif [ "$QUICK" = true ]; then
    TEST_CMD="./starvoyager-tests --quick"
else
    TEST_CMD="./starvoyager-tests"
fi

if $TEST_CMD; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi