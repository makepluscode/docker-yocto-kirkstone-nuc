#!/bin/bash

IMAGE=yocto-nuc:builder
CONTAINER=yocto-nuc
DIR=$(dirname "$(realpath "$0")")
HOST_YOCTO_DIR="$DIR/kirkstone"
CONTAINER_YOCTO_DIR="/home/yocto/kirkstone"

# Check for required argument
if [ $# -eq 0 ]; then
    echo "Usage: $0 <mode>"
    echo "  auto   - Automatic build mode: clean, configure, and build nuc-image-qt5"
    echo "  manual - Manual mode: enter container with build environment setup"
    echo "  bundle - Bundle build mode: clean, configure, and build nuc-image-qt5-bundle"
    echo ""
    echo "Examples:"
    echo "  $0 auto     # Run automatic build and exit"
    echo "  $0 manual   # Enter container for manual operations"
    echo "  $0 bundle   # Build RAUC bundle for nuc-image-qt5"
    exit 1
fi

MODE=$1

# Validate mode argument
if [ "$MODE" != "auto" ] && [ "$MODE" != "manual" ] && [ "$MODE" != "bundle" ]; then
    echo "Error: Invalid mode '$MODE'"
    echo "Valid modes: auto, manual, bundle"
    exit 1
fi

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
  if [ "$MODE" = "auto" ]; then
    # Auto mode: run entrypoint script and exit
    docker exec -it $CONTAINER /home/yocto/entrypoint.sh
  elif [ "$MODE" = "bundle" ]; then
    # Bundle mode: run entrypoint script with bundle argument
    docker exec -it $CONTAINER /home/yocto/entrypoint.sh bundle
  else
    # Manual mode: just enter bash
    docker exec -it $CONTAINER bash
  fi
  exit 0
fi

# 정지된 컨테이너가 있으면 재시작
if docker ps -a --format '{{.Names}}' | grep -q "^$CONTAINER\$"; then
  echo "▶ 이전에 종료된 컨테이너를 재시작합니다: $CONTAINER"
  # 기존 컨테이너 제거 (볼륨 마운트를 위해)
  docker rm $CONTAINER
  if [ "$MODE" = "auto" ]; then
    # Auto mode: start and run entrypoint script
    docker run -it \
      --name $CONTAINER \
      -v "$HOST_YOCTO_DIR":"$CONTAINER_YOCTO_DIR" \
      $IMAGE /home/yocto/entrypoint.sh
  elif [ "$MODE" = "bundle" ]; then
    # Bundle mode: start and run entrypoint script with bundle argument
    docker run -it \
      --name $CONTAINER \
      -v "$HOST_YOCTO_DIR":"$CONTAINER_YOCTO_DIR" \
      $IMAGE /home/yocto/entrypoint.sh bundle
  else
    # Manual mode: start and enter bash (override entrypoint)
    docker run -it \
      --name $CONTAINER \
      -v "$HOST_YOCTO_DIR":"$CONTAINER_YOCTO_DIR" \
      --entrypoint /bin/bash \
      $IMAGE
  fi
  exit 0
fi

# 새 컨테이너 실행 (볼륨 매핑 포함)
echo "▶ 새로운 컨테이너를 시작합니다: $CONTAINER"
if [ "$MODE" = "auto" ]; then
  # Auto mode: run with entrypoint script
  docker run -it \
    --name $CONTAINER \
    -v "$HOST_YOCTO_DIR":"$CONTAINER_YOCTO_DIR" \
    $IMAGE /home/yocto/entrypoint.sh
elif [ "$MODE" = "bundle" ]; then
  # Bundle mode: run with entrypoint script with bundle argument
  docker run -it \
    --name $CONTAINER \
    -v "$HOST_YOCTO_DIR":"$CONTAINER_YOCTO_DIR" \
    $IMAGE /home/yocto/entrypoint.sh bundle
else
  # Manual mode: run with bash entrypoint
  docker run -it \
    --name $CONTAINER \
    -v "$HOST_YOCTO_DIR":"$CONTAINER_YOCTO_DIR" \
    --entrypoint /bin/bash \
    $IMAGE
fi