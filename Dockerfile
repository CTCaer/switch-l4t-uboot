FROM ubuntu:focal

RUN apt-get update && apt-get -qq -y install build-essential gcc bison flex python3 python3-distutils python3-dev swig make python python-dev bc ash
RUN /bin/ash -c 'set -ex && \
    ARCH=`uname -m` && \
    if [ "$ARCH" != "aarch64" ]; then \
       echo "x86_64" && \
       apt install -y gcc-aarch64-linux-gnu; \
    fi'

ENV ARCH=arm64
ENV CROSS_COMPILE=aarch64-linux-gnu-

WORKDIR /out
VOLUME /out

CMD make nintendo-switch_defconfig && \
    make -j$(nproc) && \
    cp u-boot.bin bl33.bin
