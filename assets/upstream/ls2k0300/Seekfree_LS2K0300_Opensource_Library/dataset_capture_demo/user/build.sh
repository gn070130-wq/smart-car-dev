#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="${SCRIPT_DIR}/../out"
TARGET_NAME="dataset_capture_demo"

cmake -S "${SCRIPT_DIR}" -B "${OUT_DIR}"
cmake --build "${OUT_DIR}" --parallel "$(nproc)"

echo "Built: ${OUT_DIR}/${TARGET_NAME}"
if [[ $# -gt 0 ]]; then
    BOARD_IP="$1"
    scp -O "${OUT_DIR}/${TARGET_NAME}" "root@${BOARD_IP}:/home/root/"
    echo "Uploaded to root@${BOARD_IP}:/home/root/${TARGET_NAME}"
fi
