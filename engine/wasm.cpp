
#include <string>
#include <memory>
#include <vector>

#include "wasm.h"
#include <emscripten.h>

std::string gameRoot;

EM_JS(void, wasm_initvga, (int width, int height), {
    window.vergeCanvas = document.getElementById('vergeCanvas');
    window.vergeCanvas.width = width;
    window.vergeCanvas.height = height;

    window.vergeContext = window.vergeCanvas.getContext('2d');
    window.vergeImageData = new ImageData(width, height);
    window.vergeImageArray = window.vergeImageData.data;
});

EM_JS(void, wasm_vgaresize, (int width, int height), {
    console.log("wasm_vgaresize", width, height);

    window.vergeCanvas.width = width;
    window.vergeCanvas.height = height;
    // clear?

    window.vergeImageData = new ImageData(width, height);
    window.vergeImageArray = window.vergeImageData.data;
});

EM_JS(void, wasm_vgadump, (uint32_t * frameBuffer, uint32_t frameBufferSize), {
    const fb = HEAPU8.subarray(frameBuffer, frameBuffer + frameBufferSize);

    window.vergeImageArray.set(fb);

    window.vergeContext.putImageData(window.vergeImageData, 0, 0);
});

EM_JS(void, wasm_nextFrame, (), {
    return Asyncify.handleSleep(requestAnimationFrame);
});

EM_JS(void, wasm_initFileSystem, (const char* c), {
    let sgr = UTF8ToString(c);
    if (sgr.endsWith('/'))
        sgr = sgr.substr(0, sgr.length - 1);
    FS.mkdir("persist");
    FS.mkdir(sgr);
    FS.mkdir("persist/" + sgr);
    // Then mount with IDBFS type
    FS.mount(IDBFS, {}, "persist/" + sgr);

    // Then sync
    FS.syncfs(true, function (err) {
        // Error
        if (err) {
            console.error('wasm_initFileSystem failed!', err);
        } else {
            console.log("wasm_initFileSystem ok");
        }
    });
});

EM_JS(void, wasm_syncFileSystem, (), {
    console.log("wasm_syncFileSystem");
    FS.syncfs(false, err => {
        if (err) {
            console.error("wasm_syncFileSystem failed!!", err);
        } else {
            console.log("wasm_syncFileSystem ok");
        }
    });
});

using DownloadCB = void(*)(char* filename, size_t size, char* data);

EM_JS(void, downloadAll, (const char** manifest, DownloadCB putFile), {
    return Asyncify.handleSleep(resume => {
        let promises = [];
        let count = 0;

        function download(pathPtr) {
            const path = UTF8ToString(pathPtr);
            return fetch(path).then(response => {
                if (!response.ok) {
                    console.error('fetchSync failed', path);
                    HEAP32[size >> 2] = 0;
                    HEAP32[data >> 2] = 0;
                    throw 'fetchSync failed';
                }
                return response.blob();
            }).then(blob =>
                blob.arrayBuffer()
            ).then(array => {
                const bytes = new Uint8Array(array);
                const dataPtr = _malloc(bytes.length);
                HEAP8.set(bytes, dataPtr);
                Module.dynCall_viii(putFile, pathPtr, bytes.length, dataPtr);

                ++count;
                verge.setLoadingProgress((100 * count / promises.length) | 0)
            });
        }

        while (true) {
            let pathPtr = HEAPU32[manifest >> 2];
            if (pathPtr == 0) {
                break;
            }
            manifest += 4;
            promises.push(download(pathPtr));
        }

        Promise.all(promises).then(() => { resume(); });
    });
});

EM_JS(void, fetchSync, (const char* pathPtr, size_t* size, char** data), {
    return Asyncify.handleSleep(resume => {
        const path = UTF8ToString(pathPtr);
        // console.log('fetchSync', path);
        return fetch(path).then(response => {
            if (!response.ok) {
                console.error('fetchSync failed', path);
                HEAP32[size >> 2] = 0;
                HEAP32[data >> 2] = 0;
                resume();
                return;
            }
            return response.blob();
        }).then(blob =>
            blob.arrayBuffer()
        ).then(array => {
            const bytes = new Uint8Array(array);
            HEAP32[size >> 2] = bytes.length;
            const dataPtr = _malloc(bytes.length);
            HEAP32[data >> 2] = dataPtr;
            HEAP8.set(bytes, dataPtr);
            resume();
        });
    });
});

struct FreeDelete { void operator()(char* p) { free(p); } };
using Deleter = std::unique_ptr<char, FreeDelete>;

void downloadGame() {
    std::string manifestPath = gameRoot + "manifest.txt";
    char* manifestPtr;
    size_t manifestLength;
    fetchSync(manifestPath.c_str(), &manifestLength, &manifestPtr);
    Deleter hello{ manifestPtr };

    std::string_view manifest{ manifestPtr, manifestLength };

    std::vector<std::string> files;
    auto append = [&](std::string_view name) {
        if (name.empty())
            return;

        if (name[name.size() - 1] == '\r')
            name.remove_suffix(1);

        files.push_back(gameRoot + std::string{ name });
    };

    while (!manifest.empty()) {
        auto pos = manifest.find('\n');
        if (pos == std::string::npos) {
            append(std::string{ manifest });
            break;
        }
        append(std::string{ manifest.substr(0, pos) });
        manifest.remove_prefix(pos + 1);
    }

    char** stuff = new char*[files.size() + 1];
    for (int i = 0; i < files.size(); ++i) {
        stuff[i] = (char*)files[i].c_str();
    }
    stuff[files.size()] = nullptr;

    downloadAll((const char**)stuff, [](char* filename, size_t size, char* data) {
        for (char* c = filename; *c; ++c)
            *c = tolower(*c);

        printf("Writing %s (%zu bytes)\n", filename, size);
        FILE* outFile = fopen(filename, "wb");
        fwrite(data, 1, size, outFile);
        fclose(outFile);
    });

    delete[] stuff;

    EM_ASM({
        window.verge.setLoadingProgress(100);
    });
}

void initFileSystem(std::string gr)
{
    gameRoot = std::move(gr);
    wasm_initFileSystem(gameRoot.c_str());

    downloadGame();
}
