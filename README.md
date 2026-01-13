# DX12 Basic Framework

This is a basic DirectX 12 framework that renders a triangle.

## Prerequisites

- Visual Studio 2019 or 2022 with "Desktop development with C++" workload installed.
- CMake 3.10 or later.

## Building

1. Open a command prompt (or PowerShell).
2. Navigate to the project directory.
3. Create a build directory:
   ```powershell
   mkdir build
   cd build
   ```
4. Generate the Visual Studio solution:
   ```powershell
   cmake ..
   ```
   If you have a specific version of VS, you might need to specify the generator, e.g.:
   ```powershell
   cmake -G "Visual Studio 17 2022" ..
   ```
5. Open the generated `DX12RayTracing.sln` in Visual Studio and build the solution, or build from the command line:
   ```powershell
   cmake --build .
   ```

## Running

After building, the executable `DX12RayTracing.exe` will be located in the `Debug` or `Release` folder (depending on your build configuration).
Ensure the `shaders` directory is copied to the same directory as the executable (CMake should handle this automatically).
