#!/bin/bash

# Whitespace Check Script
# This script checks for whitespace issues without modifying files

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}[CHECK]${NC} Checking for whitespace issues..."

# Find all relevant files
FILES=$(find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "*.qml" -o -name "*.sh" -o -name "CMakeLists.txt" -o -name "*.md" -o -name "*.xml" -o -name "*.conf" -o -name "*.service" \) -not -path "./*/build/*" -not -path "./*/oe-*" -not -path "./*/.git/*" 2>/dev/null || true)

TOTAL_FILES=$(echo "$FILES" | wc -l)
ISSUE_COUNT=0

echo -e "${BLUE}[INFO]${NC} Checking $TOTAL_FILES files..."

# Check for trailing whitespace
TRAILING_WHITESPACE_FILES=$(find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "*.qml" -o -name "*.sh" -o -name "CMakeLists.txt" -o -name "*.md" -o -name "*.xml" -o -name "*.conf" -o -name "*.service" \) -not -path "./*/build/*" -not -path "./*/oe-*" -not -path "./*/.git/*" -exec grep -l '[[:space:]]$' {} \; 2>/dev/null || true)

# Check for files without final newline
NO_NEWLINE_FILES=""
for file in $FILES; do
    if [ -f "$file" ] && [ -s "$file" ] && [ "$(tail -c1 "$file" | wc -l)" -eq 0 ]; then
        if [ -z "$NO_NEWLINE_FILES" ]; then
            NO_NEWLINE_FILES="$file"
        else
            NO_NEWLINE_FILES="$NO_NEWLINE_FILES
$file"
        fi
    fi
done

# Check for Windows line endings
WINDOWS_LINE_ENDINGS=$(find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "*.qml" -o -name "*.sh" -o -name "CMakeLists.txt" -o -name "*.md" -o -name "*.xml" -o -name "*.conf" -o -name "*.service" \) -not -path "./*/build/*" -not -path "./*/oe-*" -not -path "./*/.git/*" -exec grep -l $'\r' {} \; 2>/dev/null || true)

# Report results
echo
echo -e "${BLUE}[SUMMARY]${NC} Whitespace check completed!"

if [ -n "$TRAILING_WHITESPACE_FILES" ]; then
    echo -e "${RED}[ISSUE]${NC} Found $(echo "$TRAILING_WHITESPACE_FILES" | wc -l) files with trailing whitespace:"
    echo "$TRAILING_WHITESPACE_FILES" | sed 's/^/  /'
    ISSUE_COUNT=$((ISSUE_COUNT + $(echo "$TRAILING_WHITESPACE_FILES" | wc -l)))
else
    echo -e "${GREEN}[OK]${NC} No trailing whitespace found"
fi

if [ -n "$NO_NEWLINE_FILES" ]; then
    echo -e "${RED}[ISSUE]${NC} Found $(echo "$NO_NEWLINE_FILES" | wc -l) files without final newline:"
    echo "$NO_NEWLINE_FILES" | sed 's/^/  /'
    ISSUE_COUNT=$((ISSUE_COUNT + $(echo "$NO_NEWLINE_FILES" | wc -l)))
else
    echo -e "${GREEN}[OK]${NC} All files have final newlines"
fi

if [ -n "$WINDOWS_LINE_ENDINGS" ]; then
    echo -e "${RED}[ISSUE]${NC} Found $(echo "$WINDOWS_LINE_ENDINGS" | wc -l) files with Windows line endings:"
    echo "$WINDOWS_LINE_ENDINGS" | sed 's/^/  /'
    ISSUE_COUNT=$((ISSUE_COUNT + $(echo "$WINDOWS_LINE_ENDINGS" | wc -l)))
else
    echo -e "${GREEN}[OK]${NC} All files have Unix line endings"
fi

echo
if [ $ISSUE_COUNT -eq 0 ]; then
    echo -e "${GREEN}[SUCCESS]${NC} All files are clean! 🎉"
    exit 0
else
    echo -e "${YELLOW}[WARNING]${NC} Found $ISSUE_COUNT files with whitespace issues"
    echo -e "${BLUE}[TIP]${NC} Run './remove_whitespace.sh' to fix these issues automatically"
    exit 1
fi
