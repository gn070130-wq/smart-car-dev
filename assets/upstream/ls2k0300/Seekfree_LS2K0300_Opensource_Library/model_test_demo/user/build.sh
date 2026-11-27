#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="${SCRIPT_DIR}/../out"

cmake -S "${SCRIPT_DIR}" -B "${OUT_DIR}"
cmake --build "${OUT_DIR}" --parallel "$(nproc)"

echo "Built: ${OUT_DIR}/model_test_demo"
