#!/bin/bash

# Simple Whitespace Cleanup Script
# Removes trailing whitespace and ensures Unix line endings

echo "🧹 Cleaning whitespace..."

# Find and process all relevant files
find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "*.qml" -o -name "*.sh" -o -name "CMakeLists.txt" -o -name "*.md" -o -name "*.xml" -o -name "*.conf" -o -name "*.service" \) -not -path "./*/build/*" -not -path "./*/oe-*" -not -path "./*/.git/*" | while read file; do
    if [ -f "$file" ]; then
        echo "Processing: $file"
        # Remove trailing whitespace
        sed -i 's/[[:space:]]*$//' "$file"
        # Convert line endings to Unix format
        sed -i 's/\r$//' "$file"
        # Ensure file ends with newline
        if [ -s "$file" ] && [ "$(tail -c1 "$file" | wc -l)" -eq 0 ]; then
            echo >> "$file"
        fi
    fi
done

echo "✅ Whitespace cleanup completed!"
