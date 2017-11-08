assimp2libgdx
========

This project was forked from [assimp2json](https://github.com/acgessler/assimp2json/) rather messily, hence some commits before the start of my project, and some files here or there referring to the project as assimp2json. It is heavily based on it, but it ultimately has a different purpose, hence why I say it's a derivative rather than a fork. 

### Build ###

The build system for assimp2libgdx is CMake. To build, use either the CMake GUI or the CMake command line utility. __Note__: make sure you pulled the `assimp` submodule, i.e. with `git submodule init && git submodule update`

### Usage ###

``` 
$ assimp2libgdx [flags] input_file [output_file] 
```

(omit the `output_file` argument to get the `json` string on stdout)

Invoke `assimp2libgdx` with no arguments for detailed information.







