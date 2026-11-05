#!/usr/bin/env bash
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
xdg-open "$ROOT/field_preview_color.svg" >/dev/null 2>&1 || \
echo "请手动打开: $ROOT/field_preview_color.svg"
