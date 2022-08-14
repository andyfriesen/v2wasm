# V2WASM

This is a WASM port of Verge v2.  Verge was a DOS (and then Windows) based 2D game engine that was developed in the early 00s.  It was kind of fun.

[verge-rpg.com](verge-rpg.com)

## Compiling

So far I have only done this with WSL.  These directions should be mostly the same on Linux or MacOS.

Install Emscripten 3.1.18 or newer and SCons.

```shell
$ scons
```

## Testing

In another terminal, run `localserver.py`.  Point your browser at localhost:8000.

