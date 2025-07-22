#!/bin/bash

IMAGE=yocto-nuc:builder
CONTAINER=yocto-nuc

echo "🧹 Docker 컨테이너/이미지 정리 중..."

# 실행 중인 컨테이너 정지 및 삭제
if docker ps -a --format '{{.Names}}' | grep -Eq "^$CONTAINER\$"; then
  echo "▶ 컨테이너 중지 및 삭제: $CONTAINER"
  docker rm -f $CONTAINER
else
  echo "✔ 컨테이너 없음"
fi

# 이미지 삭제
if docker image inspect $IMAGE >/dev/null 2>&1; then
  echo "▶ 이미지 삭제: $IMAGE"
  docker rmi $IMAGE
else
  echo "✔ 이미지 없음"
fi

echo "✅ 정리 완료"