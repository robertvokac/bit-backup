#!/bin/bash

HEADER_FILE="header.txt"

process_file() {
    local f="$1"

    # skip if already contains license
    if grep -q "MIT License" "$f"; then
        echo "Skipping (already has header): $f"
        return
    fi

    echo "Processing: $f"

    # insert header at the top
    cat "$HEADER_FILE" "$f" > "$f.new"
    mv "$f.new" "$f"
}

export -f process_file
export HEADER_FILE

# top-level files
find . -maxdepth 1 -type f \
    \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.ixx" -o -name "*.cppm" \) \
    -exec bash -c 'process_file "$0"' {} \;

# include, src, tests
find ./include ./src ./tests -type f \
    \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.ixx" -o -name "*.cppm" \) \
    -exec bash -c 'process_file "$0"' {} \;
