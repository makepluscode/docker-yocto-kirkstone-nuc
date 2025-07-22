FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
  gawk wget git diffstat unzip texinfo gcc g++ build-essential \
  chrpath socat cpio python3 python3-pip python3-pexpect xz-utils \
  iputils-ping rsync locales zstd lz4 sudo && \
  apt-get clean

RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8

RUN useradd -m yocto && echo "yocto ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
USER yocto
WORKDIR /home/yocto

# Git clone 후 압축
RUN mkdir src && cd src && \
  git clone -b kirkstone git://git.yoctoproject.org/poky && \
  git clone -b kirkstone https://github.com/openembedded/meta-openembedded.git && \
  git clone -b kirkstone https://github.com/axians/meta-microservicebus-intel-nuc.git && \
  git clone -b kirkstone https://git.yoctoproject.org/meta-intel && \
  cd .. && tar -I zstd -cf yocto.tar.zst src && rm -rf src

# 자동 진입 + 프롬프트 + 초기화
RUN echo "PS1='\[\e[1;36m\]yocto\[\e[0m\]@nuc:\[\e[1;32m\]\w\[\e[0m\]\\$ '" >> ~/.bashrc && \
    echo 'if [ ! -d ~/yocto ]; then mkdir -p ~/yocto && tar -I zstd -xf ~/yocto.tar.zst -C ~/yocto --strip-components=1; fi' >> ~/.bashrc && \
    echo 'cd ~/yocto && source poky/oe-init-build-env build' >> ~/.bashrc

CMD ["bash"]