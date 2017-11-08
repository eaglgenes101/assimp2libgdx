assimp2libgdx
========

This project was derived from [assimp2json](https://github.com/acgessler/assimp2json/) rather messily, hence some commits before the start of my project, and some files here or there referring to the project as assimp2json. It is heavily based on it, but it ultimately has a different purpose, hence why I say it's a derivative rather than a fork. 

### Status ###

Right now, the project successfully generates json files based on the input. Whether they actually are valid g3dj files is another question entirely. I'm on my way to make sure that the files are acutal g3dj files, but until then, I can make no promises that your converted models won't cause libgdx to crash and burn. 

Only a subset of assimp features are currently translated to g3d json files. This will be lifted as the project goes on. 

### Build ###

The build system for assimp2libgdx is CMake. To build, use either the CMake GUI or the CMake command line utility. __Note__: make sure you pulled the `assimp` submodule, i.e. with `git submodule init && git submodule update`

### Usage ###

``` 
$ assimp2libgdx [flags] input_file [output_file] 
```

(omit the `output_file` argument to get the `json` string on stdout)

Invoke `assimp2libgdx` with no arguments for detailed information.







