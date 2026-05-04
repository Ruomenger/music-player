#!/bin/bash
set -euo pipefail

script_dir="$(cd "$(dirname "$0")" && pwd)"
project_dir="$(dirname "$script_dir")"

clang_format="clang-format"
if [ -n "${CLANG_FORMAT:-}" ]; then
    clang_format="$CLANG_FORMAT"
fi

if ! command -v "$clang_format" &>/dev/null; then
    echo "Error: $clang_format not found" >&2
    exit 1
fi

cd "$project_dir"

if [ "${1:-}" = "--check" ] || [ "${1:-}" = "-c" ]; then
    echo "Checking formatting..."
    find src tests -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) \
        -exec "$clang_format" --dry-run --Werror {} +
    echo "Format check passed."
else
    echo "Formatting..."
    find src tests -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) \
        -exec "$clang_format" -i {} +
    echo "Done."
fi
