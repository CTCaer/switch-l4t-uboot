FROM ubuntu:focal

RUN apt-get update && apt-get -qq -y install gcc-aarch64-linux-gnu build-essential bison flex python3 python3-distutils python3-dev swig make python python-dev bc

WORKDIR /build
COPY . /build/
VOLUME /out

ENTRYPOINT CROSS_COMPILE=aarch64-linux-gnu- ARCH=arm64 make nintendo-switch_defconfig && CROSS_COMPILE=aarch64-linux-gnu- ARCH=arm64 make && cp u-boot.elf /out
