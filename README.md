# RayTracing

## Build
```powershell
git clone https://github.com/oskarweb/RayTracing.git
cd RayTracing

& "$VCPKG_ROOT\vcpkg.exe" install `
    glfw3 `
    glm `
    imgui[glfw-binding,vulkan-binding] `
    implot `
    vulkan `
    vulkan-memory-allocator

cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config=Release --parallel
```
