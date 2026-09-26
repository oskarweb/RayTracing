# RayTracing


## Build

### Windows x64, clang20
cmake --fresh -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER_TARGET=x86_64-pc-windows-msvc -DCMAKE_CXX_COMPILER_TARGET=x86_64-pc-windows-msvc -DVCPKG_TARGET_TRIPLET=x64-windows ..

## Tests

Enable the GoogleTest unit tests in an already configured build directory:

```powershell
cmake -S . -B build -DRAYTRACING_BUILD_TESTS=ON
cmake --build build --config Release --target raytracing_unit_tests
ctest --test-dir build -C Release -L unit --output-on-failure
```
