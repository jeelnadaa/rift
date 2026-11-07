#!/usr/bin/env bash
# Build the Qt-only targets and run the UI harness (real QML, mock controller). Needs Qt 6.5+.
#   QT_DIR=D:/Qt/6.12.0/mingw_64 bash scripts/ui-harness.sh
set -euo pipefail
cd "$(dirname "$0")/.."
B=${BUILD_DIR:-build-ui}
cmake -S . -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DRIFT_UI_ONLY=ON ${QT_DIR:+-DCMAKE_PREFIX_PATH=$QT_DIR} >/dev/null
cmake --build "$B"
QT_QPA_PLATFORM=${QT_QPA_PLATFORM:-offscreen} "$B/rift_ui_harness" "$@"
