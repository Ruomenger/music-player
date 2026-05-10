#!/usr/bin/env bash
#
# tidy.sh — Clang-Tidy 静态分析脚本
#
# 用法:
#   bash scripts/tidy.sh              # 自动检测 build 目录
#   bash scripts/tidy.sh -d <dir>     # 指定构建目录
#   bash scripts/tidy.sh --help       # 帮助

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

BUILD_DIR="${PROJECT_DIR}/build"

show_help() {
    cat <<EOF
用法: bash scripts/tidy.sh [选项]

选项:
  (无参数)         自动检测 build/ 下 compile_commands.json
  -d <dir>         指定构建目录 (含 compile_commands.json)
  --help           显示帮助

前置: 需先运行 cmake --preset ci-debug (或 debug)
EOF
    exit 0
}

if [ "${1:-}" == "--help" ]; then
    show_help
fi

if [ "${1:-}" == "-d" ]; then
    shift
    BUILD_DIR="${1}"
    shift || true
fi

# 自动检测 compile_commands.json
if [ ! -f "${BUILD_DIR}/compile_commands.json" ]; then
    DB=$(find "${PROJECT_DIR}/build" -maxdepth 3 -name compile_commands.json 2>/dev/null | head -1)
    if [ -n "$DB" ]; then
        BUILD_DIR="$(dirname "$DB")"
        echo "[tidy] auto-detected: ${BUILD_DIR}"
    else
        echo "[tidy] compile_commands.json not found. Run: cmake --preset ci-debug"
        exit 1
    fi
fi

NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# CI 环境使用版本号二进制，本地使用系统默认
if [ "${CI:-}" == "true" ]; then
    TIDY_BIN="clang-tidy-21"
else
    TIDY_BIN="clang-tidy"
fi
echo "[tidy] build_dir=${BUILD_DIR}, parallel=${NPROC}, binary=${TIDY_BIN}"

if ! run-clang-tidy \
    -clang-tidy-binary "$TIDY_BIN" \
    -p "$BUILD_DIR" \
    -header-filter='.*src/.*' \
    -j "$NPROC" \
    -quiet; then
    echo "[tidy] issues found (see above)"
    exit 1
fi

echo "[tidy] no issues found"