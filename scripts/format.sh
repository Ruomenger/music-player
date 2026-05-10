#!/usr/bin/env bash
#
# format.sh — 代码格式化脚本
#
# 用法:
#   bash scripts/format.sh              # 格式化所有 C++ 源码
#   bash scripts/format.sh --check      # 仅检查差异，不修改文件

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

SOURCES=(
    "${PROJECT_DIR}/src"
    "${PROJECT_DIR}/tests"
)

FILES=$(find "${SOURCES[@]}" -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) 2>/dev/null || true)

if [ -z "$FILES" ]; then
    echo "[format] No C++ files found."
    exit 0
fi

MODE="format"
if [ "${1:-}" == "--check" ]; then
    MODE="check"
    echo "[format] Checking formatting..."
else
    echo "[format] Formatting files..."
fi

EXIT_CODE=0
for file in $FILES; do
    if [ "$MODE" == "check" ]; then
        if ! clang-format --dry-run --Werror "$file" 2>/dev/null; then
            echo "  [FAIL] $file"
            EXIT_CODE=1
        fi
    else
        clang-format -i "$file"
        echo "  [OK] $file"
    fi
done

if [ "$MODE" == "check" ]; then
    if [ $EXIT_CODE -eq 0 ]; then
        echo "[format] All files formatted correctly."
    else
        echo "[format] Some files need formatting. Run 'bash scripts/format.sh' to fix."
    fi
fi

exit $EXIT_CODE