#!/bin/bash

set -e

BASE_DIR="$(cd "$(dirname "$0")" && pwd)"
KIRKSTONE_DIR="$BASE_DIR/kirkstone"

mkdir -p "$KIRKSTONE_DIR"
cd "$KIRKSTONE_DIR"

clone_repo() {
  local url=$1
  local dir=$2
  local branch="kirkstone"

  if [ -d "$dir" ]; then
    echo -e "🔁 \033[1;33m$dir already exists. Skipping.\033[0m"
  else
    echo -e "📥 \033[1;32mCloning $dir...\033[0m"
    git clone -b "$branch" "$url" "$dir"
  fi
}

echo -e "📂 \033[1;36mCloning Yocto layers into: $KIRKSTONE_DIR\033[0m"

clone_repo git://git.yoctoproject.org/poky poky
clone_repo https://github.com/openembedded/meta-openembedded.git meta-openembedded
clone_repo https://git.yoctoproject.org/meta-intel meta-intel
clone_repo https://github.com/rauc/meta-rauc.git meta-rauc
clone_repo https://github.com/meta-qt5/meta-qt5.git meta-qt5

echo -e "✅ \033[1;32mAll layers prepared.\033[0m"