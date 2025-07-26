#!/usr/bin/env bash
# Composite build script
# 1. clean workspace via clean.sh
# 2. run Docker-based Yocto build via run-docker.sh
# Only if each step succeeds will the next run.
# Usage: ./build.sh [additional args passed to run-docker.sh]

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P)

# Step 1: clean
echo "🔄 Running clean.sh ..."
if "$SCRIPT_DIR/clean.sh"; then
  echo "✅ clean.sh completed"
else
  echo "❌ clean.sh failed. Aborting."
  exit 1
fi

# Step 2: run docker build
echo "🐳 Starting Docker build ..."
if "$SCRIPT_DIR/run-docker.sh" "$@"; then
  echo "🎉 Build finished successfully"
else
  echo "❌ Docker build failed"
  exit 1
fi 