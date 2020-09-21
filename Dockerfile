FROM ubuntu:focal

RUN apt-get update && apt-get -qq -y install build-essential gcc bison flex python3 python3-distutils python3-dev swig make python python-dev bc ash
RUN /bin/ash -c 'set -ex && \
    ARCH=`uname -m` && \
    if [ "$ARCH" != "aarch64" ]; then \
       echo "x86_64" && \
       apt install -y gcc-aarch64-linux-gnu; \
    fi'

ARG DISTRO
ENV DISTRO=${DISTRO}
ENV ARCH=arm64
ENV CROSS_COMPILE=aarch64-linux-gnu-

WORKDIR /build
COPY . /build
VOLUME /out

CMD sed -i 's/boot_prefixes=\/ \/switchroot\/.*\/\\0/boot_prefixes=\/ \/switchroot\/'${DISTRO}'\/\\0/' /build/include/config_distro_bootcmd.h && make nintendo-switch_defconfig && make && cp u-boot.elf /out
