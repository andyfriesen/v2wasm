import os
import os.path
import sys

# Find emcc
emcc = None
path = os.environ['PATH'].split(os.path.pathsep)
for p in path:
    p2 = os.path.join(p, 'emcc')
    if os.path.exists(p2):
        emcc = p2
        break

if not emcc:
    print('Could not find emcc.  Check your PATH?')
    sys.exit(1)

def EmscriptenEnvironment():
    if sys.platform in ('windows', 'win32'):
        env_dict = {
            'path': os.environ['PATH']
        }

        for key in ['HOME', 'USERPROFILE', 'EM_CONFIG']:
            value = os.environ.get(key)
            if value is not None:
                env_dict[key] = value

        env = Environment(ENV=env_dict, TOOLS=['mingw'])
    else:
        env = Environment()

    env['CC'] = emcc
    env['CXX'] = emcc

    emscriptenOpts = [
        '-s', 'ASYNCIFY',
        '-s', 'ASYNCIFY_IMPORTS=["fetchSync","downloadAll","wasm_nextFrame","emscripten_sleep"]',
        '-s', 'FETCH=1',
        '-s', 'FORCE_FILESYSTEM=1',
        '-s', 'ALLOW_MEMORY_GROWTH=1',
    ]

    cflags = ['-fcolor-diagnostics', '-fno-rtti', '-fno-exceptions']

    asmjs = ARGUMENTS.get('asmjs', 0)
    debug = int(ARGUMENTS.get('debug', 0))
    asan = ARGUMENTS.get('asan', 0)
    ubsan = ARGUMENTS.get('ubsan', 0)

    debug_flags = {
        'debug_functions',
        'debug_locals',
        'debug_input',
        'debug_vc',
        'profile_vc'
    }
    
    for flag in debug_flags:
        if ARGUMENTS.get(flag):
            env.Append(CPPDEFINES=[
                flag.upper()
            ])

    if debug:
        if not asan:
            emscriptenOpts += [
                '-s', 'SAFE_HEAP=1',
            ]

        emscriptenOpts += [
            '-s', 'ASYNCIFY_STACK_SIZE=327680',
            '-s', 'ASSERTIONS=1',
            '-s', 'STACK_OVERFLOW_CHECK=1',
            '-s', 'DEMANGLE_SUPPORT=1',
        ]

        cflags.append('-g')

        env.Append(LINKFLAGS=[
            '-g',
        ])

    else:
        emscriptenOpts += [
            '-s', 'ASYNCIFY_STACK_SIZE=32768',
        ]
        cflags.append('-O3')
        cflags.append('-flto')
        env.Append(LINKFLAGS=['-flto'])

    if asmjs:
        env.Append(LINKFLAGS=[
            '-s', 'WASM=0',
        ])

    if asan:
        cflags.append('-fsanitize=address')
        env.Append(LINKFLAGS=['-fsanitize=address'])
    if ubsan:
        cflags.append('-fsanitize=undefined')
        env.Append(LINKFLAGS=['-fsanitize=undefined'])

    cflags.extend([
        '-MMD',
        '-Wno-parentheses',
        '-Wno-long-long',
        '-Wno-dangling-else',
        '-Wno-writable-strings',
    ])

    env.Append(CFLAGS=cflags)
    env.Append(CXXFLAGS=cflags)

    env.Append(LINKFLAGS=[
        '-lidbfs.js',
    ] + emscriptenOpts)

    return env

sources = [
    'a_memory.cpp',
    'entity.cpp',
    'i_png.cpp',
    'message.cpp',
    'startup.cpp',
    'timer.cpp',
    'vfile.cpp',
    'console.cpp',
    'font.cpp',
    'linked.cpp',
    'render.cpp',
    'str.cpp',
    'vc.cpp',
    'w_graph.cpp',

    'engine.cpp',
    'image.cpp',
    'memstr.cpp',
    'sound.cpp',
    'strk.cpp',
    'verge.cpp',
    'w_input.cpp',

    'wasm.cpp',
]
sources = ['engine/' + s for s in sources]

env = EmscriptenEnvironment()

env.Append(
    CXXFLAGS=[
        '-std=c++17',
        #'-Werror', # Hahaha no way.  This code dates back to like 1997.
    ],
)

verge = env.Program('verge2.out.js', sources, PROGSUFFIX='.js')
