# RayTracing

## Build

### Windows x64 with Clang and vcpkg

Prerequisites:

- Git, CMake **4.1 or newer**, and Ninja.
- Clang 20 or newer, plus the Visual Studio C++ build tools and Windows SDK. The project requires C++23.
- A bootstrapped [vcpkg installation](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started).
- A Vulkan-capable GPU and driver to run the applications.

Run the following commands in an **x64 Visual Studio Developer PowerShell**, with `clang`, `clang++`, `cmake`, and `ninja` on `PATH`.

Clone the project and set `VCPKG_ROOT` to your vcpkg installation:

```powershell
git clone https://github.com/oskarweb/RayTracing.git
cd RayTracing

$env:VCPKG_ROOT = "C:/vcpkg" # Change this to your installation path.
```

Install the dependencies for the same x64 triplet used by CMake:

```powershell
& "$env:VCPKG_ROOT/vcpkg.exe" install --triplet x64-windows `
    glfw3 `
    glm `
    "imgui[glfw-binding,vulkan-binding]" `
    implot `
    vulkan `
    vulkan-memory-allocator
```

Configure and build from the repository root. The [vcpkg toolchain](https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/cmake-integration) makes the installed dependencies available to CMake:

```powershell
cmake -S . -B build -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
    -DVCPKG_TARGET_TRIPLET=x64-windows `
    -DCMAKE_C_COMPILER=clang `
    -DCMAKE_CXX_COMPILER=clang++ `
    -DCMAKE_C_COMPILER_TARGET=x86_64-pc-windows-msvc `
    -DCMAKE_CXX_COMPILER_TARGET=x86_64-pc-windows-msvc

cmake --build build --parallel
```

If `build` was configured with a different generator, compiler, or toolchain, use a new build directory, such as `build-clang`, in both commands. Subsequent builds only need `cmake --build build --parallel`.

### Run

Run each application from its output directory so it can find its shaders and other assets. CMake copies these assets during the build, including the existing compiled shaders.

```powershell
Push-Location build/Release/ray_tracing
./ray_tracing.exe
Pop-Location
```

For the particle simulation, use `build/Release/coulomb_law_simulation` and run `./coulomb_law_simulation.exe` instead.

## Tests

### Unit tests

Install GoogleTest through vcpkg, then enable and build the unit-test target in the existing build directory:

```powershell
& "$env:VCPKG_ROOT/vcpkg.exe" install gtest --triplet x64-windows

cmake -S . -B build -DRAYTRACING_BUILD_TESTS=ON
cmake --build build --target raytracing_unit_tests --parallel
ctest --test-dir build -L unit --output-on-failure
```

The unit tests cover resource handles, physics integrators, simulation state, model ownership, camera transforms, input, and mesh generation. They do not open a window or create a Vulkan device. If no installed GoogleTest package is found, CMake downloads a pinned, checksum-verified GoogleTest release.

To run a subset, add a name filter, for example:

```powershell
ctest --test-dir build -L unit -R "IntegratorTest|Rk4Test" --output-on-failure
```

### Vulkan smoke test

The smoke test requires a Vulkan-capable GPU and display. It exercises initialization, rendering, resizing, cleanup, and reinitialization using a hidden window.

```powershell
cmake -S . -B build -DRAYTRACING_BUILD_SMOKE_TESTS=ON
cmake --build build --target renderer_lifetime_smoke --parallel
ctest --test-dir build -L gpu --output-on-failure
```

Both test options default to `OFF` and can be enabled independently. Run `ctest --test-dir build --output-on-failure` to run all enabled tests. Debug builds additionally require the Vulkan validation layer; use `-DCMAKE_BUILD_TYPE=Debug` when configuring Ninja to select a Debug build.
