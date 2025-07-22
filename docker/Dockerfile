FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

# 필수 패키지 설치
RUN apt-get update && apt-get install -y \
  gawk wget git diffstat unzip texinfo gcc g++ build-essential \
  chrpath socat cpio python3 python3-pip python3-pexpect xz-utils \
  iputils-ping rsync locales sudo && \
  apt-get clean

# 로케일 설정
RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8

# yocto 사용자 생성
RUN useradd -m yocto && echo "yocto ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
USER yocto
WORKDIR /home/yocto

# Yocto 레이어 직접 clone (압축 없이)
RUN mkdir -p yocto && cd yocto && \
  git clone -b kirkstone git://git.yoctoproject.org/poky && \
  git clone -b kirkstone https://github.com/openembedded/meta-openembedded.git && \
  git clone -b kirkstone https://github.com/axians/meta-microservicebus-intel-nuc.git && \
  git clone -b kirkstone https://git.yoctoproject.org/meta-intel

# 컬러 프롬프트만 적용
RUN echo "PS1='\[\e[1;36m\]yocto\[\e[0m\]@nuc:\[\e[1;32m\]\w\[\e[0m\]\\$ '" >> ~/.bashrc

# ENTRYPOINT: 빌드 환경 진입 + bblayers.conf 설정
ENTRYPOINT bash -c '\
  cd ~/yocto && \
  source poky/oe-init-build-env build && \
  CONF=$BUILDDIR/conf/bblayers.conf && \
  if ! grep -q "meta-microservicebus-intel-nuc" "$CONF"; then \
    echo "🛠 Updating bblayers.conf..."; \
    sed -i "/^BBLAYERS ?=/a \\
  \ \ \ \ \${TOPDIR}/../meta-openembedded/meta-oe \\\n  \ \ \ \ \${TOPDIR}/../meta-openembedded/meta-python \\\n  \ \ \ \ \${TOPDIR}/../meta-openembedded/meta-networking \\\n  \ \ \ \ \${TOPDIR}/../meta-intel \\\n  \ \ \ \ \${TOPDIR}/../meta-microservicebus-intel-nuc \\" "$CONF"; \
  fi && \
  exec bash'