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
  \${TOPDIR}/../meta-microservicebus-intel-nuc"
EOF
fi

exec bash