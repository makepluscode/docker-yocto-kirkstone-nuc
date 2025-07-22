#!/bin/bash

IMAGE=yocto-nuc:builder
DIR=$(dirname "$(realpath "$0")")

if ! docker image inspect $IMAGE >/dev/null 2>&1; then
  echo "▶ Docker 이미지 빌드 중..."
  docker build -t $IMAGE "$DIR/docker" || exit 1
fi

docker run --rm -it --name yocto-nuc $IMAGE