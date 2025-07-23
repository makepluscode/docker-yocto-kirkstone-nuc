#!/bin/bash

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

# 레이어 추가 (중복 방지)
if ! grep -q "meta-microservicebus-intel-nuc" "$CONF"; then
  echo "🛠 Updating bblayers.conf..."
  cat <<EOF >> "$CONF"

BBLAYERS += " \\
  \${TOPDIR}/../meta-openembedded/meta-oe \\
  \${TOPDIR}/../meta-openembedded/meta-python \\
  \${TOPDIR}/../meta-openembedded/meta-networking \\
  \${TOPDIR}/../meta-intel \\
  \${TOPDIR}/../meta-microservicebus-intel-nuc \\
  \${TOPDIR}/../meta-nuc \\
  \${TOPDIR}/../meta-rauc"

EOF
fi

# local.conf 자동 배포/초기화
LOCALCONF="$BUILDDIR/conf/local.conf"
LOCALCONF_TEMPLATE="$TOPDIR/../local.conf.sample"

if [ ! -f "$LOCALCONF" ] && [ -f "$LOCALCONF_TEMPLATE" ]; then
  echo "🛠 Copying local.conf template..."
  cp "$LOCALCONF_TEMPLATE" "$LOCALCONF"
elif [ -f "$LOCALCONF" ] && [ -f "$LOCALCONF_TEMPLATE" ]; then
  # 필요시 강제 초기화 옵션 처리 (예: ./entrypoint.sh --reset-localconf)
  if [[ "$@" == *"--reset-localconf"* ]]; then
    echo "🛠 Backing up and resetting local.conf..."
    cp "$LOCALCONF" "$LOCALCONF.bak.$(date +%Y%m%d%H%M%S)"
    cp "$LOCALCONF_TEMPLATE" "$LOCALCONF"
  fi
fi

exec bash