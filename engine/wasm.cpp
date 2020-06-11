
#include "wasm.h"
#include <emscripten.h>

EM_JS(void, wasm_initvga, (int width, int height), {
    window.vergeCanvas = document.getElementById('vergeCanvas');
    window.vergeCanvas.width = width;
    window.vergeCanvas.height = height;

    window.vergeContext = window.vergeCanvas.getContext('2d');
    window.vergeImageData = new ImageData(width, height);
    window.vergeImageArray = window.vergeImageData.data;
});

EM_JS(void, wasm_vgaresize, (int width, int height), {
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
