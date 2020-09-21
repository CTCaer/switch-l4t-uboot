# Using Dockerfile

## Usage

```sh
docker run --rm -it -e DISTRO=ubuntu -v "$PWD":/out alizkan/switch-uboot:linux-norebase
```

You can override the workdir used in the docker, to use your own changes, without rebuilding the image by adding this repository directory as a volume to the docker command above.
```sh
-v $(pwd):build/
```