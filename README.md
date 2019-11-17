OpenSoundMixer2
==============

This is a sound mixer for multiplatforms.

This project includes the following projects.

* libogg-1.3.2
* libvorbis-1.3.5

## How to build

- Windows

```
cd scripts
GenerateProjects_x64.bat
```

Open a project in build directory

- Linux

```
cd scripts
GenerateProjectsWithSanitize.sh
cd ../build
make
```

## Requirements

### Common

* Python3.x

### Windows

* WASAPI

### Ubuntu

- apt-get install libpulse-dev

### MacOSX

* OpenAL

