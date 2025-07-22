#!/bin/bash

IMAGE=yocto-nuc:builder
CONTAINER=yocto-nuc
DIR=$(dirname "$(realpath "$0")")
HOST_YOCTO_DIR="$DIR/yocto"
CONTAINER_YOCTO_DIR="/home/yocto/yocto"

# 호스트 yocto 디렉토리 없으면 생성
mkdir -p "$HOST_YOCTO_DIR"

# 이미지가 없으면 빌드
if ! docker image inspect $IMAGE >/dev/null 2>&1; then
  echo "▶ Docker 이미지 빌드 중..."
  docker build -t $IMAGE "$DIR/docker" || exit 1
fi

# 컨테이너가 실행 중이면 attach
if docker ps --format '{{.Names}}' | grep -q "^$CONTAINER\$"; then
  echo "▶ 실행 중인 컨테이너에 attach 합니다: $CONTAINER"
  docker exec -it $CONTAINER bash
  exit 0
fi

# 정지된 컨테이너가 있으면 재시작
if docker ps -a --format '{{.Names}}' | grep -q "^$CONTAINER\$"; then
  echo "▶ 이전에 종료된 컨테이너를 재시작합니다: $CONTAINER"
  docker start -ai $CONTAINER
  exit 0
fi

# 새 컨테이너 실행 (볼륨 매핑 포함)
echo "▶ 새로운 컨테이너를 시작합니다: $CONTAINER"
docker run -it \
  --name $CONTAINER \
  -v "$HOST_YOCTO_DIR":"$CONTAINER_YOCTO_DIR" \
  $IMAGE