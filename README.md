## OpenDLV Microservice to encode images in I420 format into h264 for network broadcast

This repository provides source code to encode images in I420 format that are accessible
via a shared memory area into x264 frames for the OpenDLV software ecosystem.

[![License: GPLv3](https://img.shields.io/badge/license-GPL--3-blue.svg
)](https://www.gnu.org/licenses/gpl-3.0.txt)


## Table of Contents
* [Dependencies](#dependencies)
* [Building and Usage](#building-and-usage)
* [License](#license)


## Dependencies
You need a C++14-compliant compiler to compile this project.

The following dependency is part of the source distribution:
* [libcluon](https://github.com/chrberger/libcluon) - [![License: GPLv3](https://img.shields.io/badge/license-GPL--3-blue.svg
)](https://www.gnu.org/licenses/gpl-3.0.txt)

The following dependencies are downloaded and installed during the Docker-ized build:
* [libx264](https://www.videolan.org/developers/x264.html) - [![License: GPLv2](https://img.shields.io/badge/license-GPL--2-blue.svg
)](https://www.gnu.org/licenses/gpl-2.0.txt) - [Information about AVC/h264 format](http://www.mpegla.com/main/programs/avc/pages/intro.aspx)
* [libyuv](https://chromium.googlesource.com/libyuv/libyuv/+/master) - [![License: BSD 3-Clause](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause) - [Google Patent License Conditions](https://chromium.googlesource.com/libyuv/libyuv/+/master/PATENTS)


## Building and Usage
Due to legal implications arising from the patents around the [AVC/h264 format](http://www.mpegla.com/main/programs/avc/pages/intro.aspx),
we cannot provide and distribute pre-built Docker images. Therefore, we provide
the build instructions in a `Dockerfile` that can be easily integrated in a
`docker-compose.yml` file.

To run this microservice using `docker-compose`, you can simply add the following
section to your `docker-compose.yml` file to let Docker build this software for you:

* Building for `amd64`:
```yml
version: '2' # Must be present exactly once at the beginning of the docker-compose.yml file
services:    # Must be present exactly once at the beginning of the docker-compose.yml file
    video-x264-encoder-amd64:
        build:
            context: https://github.com/chalmers-revere/opendlv-video-x264-encoder.git
            dockerfile: Dockerfile.amd64
        restart: on-failure
        network_mode: "host"
        ipc: "host"
        volumes:
        - /tmp:/tmp
        command: "--cid=111 --name=video0.i420 --width=640 --height=480"
```
* Building for `armhf`:
```yml
version: '2' # Must be present exactly once at the beginning of the docker-compose.yml file
services:    # Must be present exactly once at the beginning of the docker-compose.yml file
    video-x264-encoder-amd64:
        build:
            context: https://github.com/chalmers-revere/opendlv-video-x264-encoder.git
            dockerfile: Dockerfile.armhf
        restart: on-failure
        network_mode: "host"
        ipc: "host"
        volumes:
        - /tmp:/tmp
        command: "--cid=111 --name=video0.i420 --width=640 --height=480"
```
* Building for `aarch64`:
```yml
version: '2' # Must be present exactly once at the beginning of the docker-compose.yml file
services:    # Must be present exactly once at the beginning of the docker-compose.yml file
    video-x264-encoder-amd64:
        build:
            context: https://github.com/chalmers-revere/opendlv-video-x264-encoder.git
            dockerfile: Dockerfile.aarch64
        restart: on-failure
        network_mode: "host"
        ipc: "host"
        volumes:
        - /tmp:/tmp
        command: "--cid=111 --name=video0.i420 --width=640 --height=480"
```


As this microservice is connecting to another video frame-providing microservice
via a shared memory area using SysV IPC, the `docker-compose.yml` file specifies
the use of `ipc:host`. The parameter `network_mode: "host"` is necessary to
broadcast the resulting frames into an `OD4Session` for OpenDLV. The folder
`/tmp` is shared into the Docker container to attach to the shared memory area.
The parameters to the application are:

* `--cid=111`: Identifier of the OD4Session to broadcast the h264 frames to
* `--id=2`: Optional identifier to set the senderStamp in broadcasted h264 frames in case of multiple instances of this microservice
* `--name=XYZ`: Name of the shared memory area to attach to
* `--width=W`: Width of the image in the shared memory area
* `--height=H`: Height of the image in the shared memory area
* `--gop=G`: desired length of group of pictures (default: 10)


## License

* This project is released under the terms of the GNU GPLv3 License

