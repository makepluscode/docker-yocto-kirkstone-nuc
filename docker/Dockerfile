FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

# 필수 패키지 설치
RUN apt-get update && apt-get install -y \
  gawk wget git diffstat unzip texinfo gcc g++ build-essential \
  chrpath socat cpio python3 python3-pip python3-pexpect xz-utils \
  iputils-ping rsync locales sudo lz4 zstd && \
  apt-get clean

# 로케일 설정
RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8

# yocto 사용자 생성
RUN useradd -m yocto && echo "yocto ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

# 일반 사용자로 전환
USER yocto
WORKDIR /home/yocto

# 컬러 프롬프트 설정
RUN echo "PS1='\[\e[1;36m\]yocto\[\e[0m\]@nuc:\[\e[1;32m\]\w\[\e[0m\]\\$ '" >> ~/.bashrc

# root로 다시 전환하여 entrypoint 복사 및 권한 설정
USER root
COPY entrypoint.sh /home/yocto/entrypoint.sh
RUN chown yocto:yocto /home/yocto/entrypoint.sh && chmod +x /home/yocto/entrypoint.sh

# 다시 yocto 사용자로 전환
USER yocto

# ENTRYPOINT 등록
ENTRYPOINT ["/home/yocto/entrypoint.sh"]