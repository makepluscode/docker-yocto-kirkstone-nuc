#!/bin/bash

IMAGE=yocto-nuc:builder
CONTAINER=yocto-nuc

echo "🧹 Cleaning up Docker environment..."

# 실행 중이거나 중지된 컨테이너가 있다면 제거
if docker ps -a --format '{{.Names}}' | grep -Eq "^${CONTAINER}$"; then
  echo "▶ Removing container: $CONTAINER"
  docker rm -f $CONTAINER
else
  echo "✔ No container to remove"
fi

# 이미지가 있다면 제거
if docker image inspect $IMAGE >/dev/null 2>&1; then
  echo "▶ Removing image: $IMAGE"
  docker rmi $IMAGE
else
  echo "✔ No image to remove"
fi

# 빌드 설정 디렉토리 정리
if [ -d "kirkstone/build/conf" ]; then
  echo "▶ Removing build configuration: kirkstone/build/conf/*"
  rm -rf kirkstone/build/conf/*
  echo "✔ Build configuration cleaned"
else
  echo "✔ No build configuration to clean"
fi

echo "✅ Cleanup complete"