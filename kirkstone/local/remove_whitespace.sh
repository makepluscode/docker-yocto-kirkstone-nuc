#!/bin/bash

# Automatic Whitespace Removal Script
# This script removes trailing whitespace, ensures consistent line endings,
# and formats code files in the project.

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if a file has trailing whitespace
has_trailing_whitespace() {
    local file="$1"
    if grep -q '[[:space:]]$' "$file" 2>/dev/null; then
        return 0  # Has trailing whitespace
    else
        return 1  # No trailing whitespace
    fi
}

# Function to check if a file has mixed line endings
has_mixed_line_endings() {
    local file="$1"
    if grep -q $'\r' "$file"; then
        return 0  # Has Windows line endings
    else
        return 1  # Unix line endings only
    fi
}

# Function to process a single file
process_file() {
    local file="$1"
    local original_content
    local modified=false

    # Skip binary files
    if file "$file" | grep -q "binary"; then
        return 0
    fi

    # Read original content
    original_content=$(cat "$file")

    # Remove trailing whitespace
    if has_trailing_whitespace "$file"; then
        sed -i 's/[[:space:]]*$//' "$file"
        modified=true
        print_status "Removed trailing whitespace from: $file"
    fi

    # Ensure file ends with newline
    if [ -s "$file" ] && [ "$(tail -c1 "$file" | wc -l)" -eq 0 ]; then
        echo >> "$file"
        modified=true
        print_status "Added final newline to: $file"
    fi

    # Convert line endings to Unix format
    if has_mixed_line_endings "$file"; then
        dos2unix "$file" 2>/dev/null || sed -i 's/\r$//' "$file"
        modified=true
        print_status "Converted line endings to Unix format in: $file"
    fi

    # Remove multiple consecutive blank lines (keep max 2)
    if grep -q '^[[:space:]]*$' "$file"; then
        # This is a more complex operation, we'll use awk
        awk '
        BEGIN { prev_blank = 0; blank_count = 0; }
        /^[[:space:]]*$/ {
            if (prev_blank) {
                blank_count++;
                if (blank_count <= 2) print;
            } else {
                blank_count = 1;
                print;
            }
            prev_blank = 1;
        }
        !/^[[:space:]]*$/ {
            print;
            prev_blank = 0;
            blank_count = 0;
        }
        ' "$file" > "$file.tmp" && mv "$file.tmp" "$file"
        modified=true
        print_status "Normalized blank lines in: $file"
    fi

    if [ "$modified" = true ]; then
        print_success "Processed: $file"
        return 0
    fi

    return 1
}

# Main processing function
main() {
    local processed_count=0
    local total_count=0
    local error_count=0

    print_status "Starting automatic whitespace removal..."

    # Find all relevant files
    local files
    files=$(find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "*.qml" -o -name "*.sh" -o -name "CMakeLists.txt" -o -name "*.md" -o -name "*.xml" -o -name "*.conf" -o -name "*.service" \) -not -path "./*/build/*" -not -path "./*/oe-*" -not -path "./*/.git/*" 2>/dev/null || true)

    if [ -z "$files" ]; then
        print_warning "No files found to process"
        exit 0
    fi

    # Count total files
    total_count=$(echo "$files" | wc -l)
    print_status "Found $total_count files to process"

    # Process each file
    while IFS= read -r file; do
        if [ -f "$file" ]; then
            if process_file "$file"; then
                ((processed_count++))
            fi
        else
            ((error_count++))
            print_error "File not found: $file"
        fi
    done <<< "$files"

    # Summary
    echo
    print_status "Whitespace removal completed!"
    print_success "Processed: $processed_count files"
    print_status "Total files checked: $total_count"
    if [ $error_count -gt 0 ]; then
        print_warning "Errors encountered: $error_count"
    fi

    if [ $processed_count -gt 0 ]; then
        print_success "Whitespace cleanup completed successfully!"
    else
        print_status "No files needed whitespace cleanup."
    fi
}

# Check if running in dry-run mode
if [ "$1" = "--dry-run" ]; then
    print_status "DRY RUN MODE - No files will be modified"
    # Modify process_file function for dry run
    process_file() {
        local file="$1"
        local has_issues=false

        if has_trailing_whitespace "$file"; then
            print_warning "Would remove trailing whitespace from: $file"
            has_issues=true
        fi

        if has_mixed_line_endings "$file"; then
            print_warning "Would convert line endings in: $file"
            has_issues=true
        fi

        if [ "$has_issues" = true ]; then
            return 0
        fi

        return 1
    }
fi

# Check if running in verbose mode
if [ "$1" = "--verbose" ] || [ "$2" = "--verbose" ]; then
    set -x
fi

# Run main function
main "$@"
