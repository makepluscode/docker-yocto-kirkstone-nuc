#!/bin/bash

# Comprehensive Whitespace Cleanup Script
# This script cleans all whitespace issues in the entire project

set -e

echo "🧹 Starting comprehensive whitespace cleanup..."

# Find all relevant files
FILES=$(find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "*.qml" -o -name "*.sh" -o -name "CMakeLists.txt" -o -name "*.md" -o -name "*.xml" -o -name "*.conf" -o -name "*.service" \) -not -path "./*/build/*" -not -path "./*/oe-*" -not -path "./*/.git/*" 2>/dev/null || true)

echo "📁 Found $(echo "$FILES" | wc -l) files to process"

# Process each file
for file in $FILES; do
    if [ -f "$file" ]; then
        echo "🔧 Processing: $file"

        # Remove trailing whitespace
        sed -i 's/[[:space:]]*$//' "$file"

        # Ensure file ends with newline
        if [ -s "$file" ] && [ "$(tail -c1 "$file" | wc -l)" -eq 0 ]; then
            echo >> "$file"
        fi

        # Convert line endings to Unix format
        sed -i 's/\r$//' "$file"
    fi
done

echo "✅ Whitespace cleanup completed!"
echo "📊 Processed $(echo "$FILES" | wc -l) files"

# Verify cleanup
echo "🔍 Verifying cleanup..."
PROBLEM_FILES=$(find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "*.qml" -o -name "*.sh" -o -name "CMakeLists.txt" -o -name "*.md" -o -name "*.xml" -o -name "*.conf" -o -name "*.service" \) -not -path "./*/build/*" -not -path "./*/oe-*" -not -path "./*/.git/*" -exec grep -l '[[:space:]]$' {} \; 2>/dev/null || true)

if [ -z "$PROBLEM_FILES" ]; then
    echo "🎉 All files are now clean!"
else
    echo "⚠️  Found $(echo "$PROBLEM_FILES" | wc -l) files still with trailing whitespace:"
    echo "$PROBLEM_FILES"
fi
