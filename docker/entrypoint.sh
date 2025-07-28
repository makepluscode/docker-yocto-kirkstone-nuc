#!/bin/bash

# 디버깅을 위한 로그 추가
echo "🔍 DEBUG: entrypoint.sh started with args: $@"
echo "🔍 DEBUG: Number of arguments: $#"

cd ~/kirkstone || exit 1

# poky/oe-init-build-env가 없으면 오류 출력 후 종료
if [ ! -f poky/oe-init-build-env ]; then
  echo "❌ Error: poky/oe-init-build-env not found. Please check if 'poky' was cloned correctly."
  ls -l poky
  exit 1
fi

# 빌드 환경 초기화
source poky/oe-init-build-env build

CONF="$BUILDDIR/conf/bblayers.conf"

# bblayers.conf 존재 확인
if [ ! -f "$CONF" ]; then
  echo "❌ Error: bblayers.conf not found at $CONF"
  exit 1
fi

# ----------------------------------------------
# local.conf 및 bblayers.conf 설정 이후에만 빌드 수행
# ----------------------------------------------

# local.conf 처리 블록 아래쪽(파일 복사 이후)에 위치해야 함

complete_build() {
  echo "🧹 Cleaning sstate for dashboard and rauc ..."
  for r in dashboard rauc; do
    if bitbake-layers show-recipes "$r" | grep -q "^$r"; then
      bitbake -c cleansstate "$r" || true
    else
      echo "ℹ️  Recipe $r not found (layer missing?) – skipping cleansstate"
    fi
  done

  echo "🚀 Building nuc-image-qt5 ..."
  if ! bitbake nuc-image-qt5; then
    echo "❌ Build failed"; exec bash; fi
  echo "✅ Build completed successfully"
  return 0
}

if ! grep -q "meta-nuc" "$CONF"; then
  echo "🛠 Updating bblayers.conf..."
  cat <<'EOF' >> "$CONF"

BBLAYERS += " \
  ${TOPDIR}/../meta-openembedded/meta-oe \
  ${TOPDIR}/../meta-openembedded/meta-python \
  ${TOPDIR}/../meta-openembedded/meta-networking \
  ${TOPDIR}/../meta-intel \
  ${TOPDIR}/../meta-nuc \
  ${TOPDIR}/../meta-rauc \
  ${TOPDIR}/../meta-qt5 \
  ${TOPDIR}/../meta-apps \
"
EOF
fi

# local.conf 자동 배포/초기화 (Yocto 기본 생성 후 덮어쓰기)
LOCALCONF="$BUILDDIR/conf/local.conf"
LOCALCONF_TEMPLATE="/home/yocto/kirkstone/meta-nuc/conf/local.conf.sample"

echo "🔍 DEBUG: LOCALCONF=$LOCALCONF"
echo "🔍 DEBUG: LOCALCONF_TEMPLATE=$LOCALCONF_TEMPLATE"
echo "🔍 DEBUG: local.conf exists: $([ -f "$LOCALCONF" ] && echo 'YES' || echo 'NO')"
echo "🔍 DEBUG: template exists: $([ -f "$LOCALCONF_TEMPLATE" ] && echo 'YES' || echo 'NO')"

if [ -f "$LOCALCONF_TEMPLATE" ]; then
  # 강제 초기화 옵션이 있거나, 기본 Yocto local.conf를 커스텀 템플릿으로 교체
  if [[ "$@" == *"--reset-localconf"* ]] || ! grep -q "MACHINE.*intel-corei7-64" "$LOCALCONF" 2>/dev/null; then
    echo "🛠 Replacing default local.conf with custom template..."
    if [ -f "$LOCALCONF" ]; then
      cp "$LOCALCONF" "$LOCALCONF.bak.$(date +%Y%m%d%H%M%S)"
    fi
    cp "$LOCALCONF_TEMPLATE" "$LOCALCONF"
    echo "✅ local.conf replaced with custom template"
  else
    echo "ℹ️  Using existing custom local.conf (use --reset-localconf to force reset)"
  fi
else
  echo "❌ Template file not found: $LOCALCONF_TEMPLATE"
fi

# 디버깅을 위한 로그 추가
echo "🔍 DEBUG: About to check manual mode"
echo "🔍 DEBUG: Arguments: '$@'"
echo "🔍 DEBUG: Number of args: $#"
echo "🔍 DEBUG: Manual check: [ $# -eq 0 ] = $([ $# -eq 0 ] && echo 'true' || echo 'false')"
echo "🔍 DEBUG: Manual check: [[ '$@' == *'manual'* ]] = $([[ "$@" == *"manual"* ]] && echo 'true' || echo 'false')"

# Check if we're in manual mode (no arguments or manual argument)
if [ $# -eq 0 ] || [[ "$@" == *"manual"* ]]; then
  echo "🔧 Manual mode: Build environment setup complete"
  echo "   You can now run manual commands like:"
  echo "   - bitbake nuc-image-qt5"
  echo "   - bitbake nuc-bundle"
  echo "   - bitbake -c menuconfig virtual/kernel"
  echo ""
  echo "🔍 DEBUG: Manual mode detected, executing bash"
  exec bash
else
  # Auto mode: run complete build
  echo "🚀 Auto mode: Starting automatic build..."
  if complete_build; then
    echo "🏁 Exiting container after successful build"
    exit 0
  fi
fi

echo "🔍 DEBUG: Reached end of script, executing bash"
exec bash