
#pragma once

#include <string>
#include <cstdint>

extern "C" {

void wasm_nextFrame();
void wasm_initvga(int xres, int yres);
void wasm_vgaresize(int xres, int yres);
void wasm_vgadump(uint32_t* framebuffer, uint32_t frameBufferSize);
void wasm_syncFileSystem();
}

extern std::string gameRoot;

void initFileSystem(std::string gameRoot);
