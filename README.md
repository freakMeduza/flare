# Flare Vulkan Engine 
The Flare Vulkan Engine (FVE) is a Vulkan based render engine meant for learning the Vulkan API.
## Features
- Modern C++17 
- [Shadertoy](https://www.shadertoy.com/) style pipeline
## Build
All platforms depend on CMake, 3.16.0 or higher, to generate IDE/make files. Ensure you are using a compiler with full C++17 support.
```bash
  $ git clone https://github.com/freakMeduza/flare.git
  $ cd flare
  $ git submodule init
  $ git submodule update
```
Also you have to resolve shaderc library dependencies. 
```bash
  $ python flare\libs\shaderc\utils\git-sync-deps
```
The simplest way to build FVE on Windows is to install Microsoft Visual Studio 2019 with C++17 support and open FVE project with "Open folder" feature. Or you can use CMake 
```bash
  $ cmake -S . -B build
  $ cmake --build build
```
## Samples
![mandelbrot](https://user-images.githubusercontent.com/26925856/151431320-826741ac-289a-42d4-9d46-ee7254999d8c.png)

## Dependencies
- [Vulkan](https://www.khronos.org/vulkan)
- [shaderc](https://github.com/google/shaderc)
- [GLFW](https://github.com/glfw/glfw)
- [spdlog](https://github.com/gabime/spdlog)
- [stb](https://github.com/nothings/stb)
- [GLM](https://github.com/g-truc/glm)
- [json](https://github.com/nlohmann/json)
