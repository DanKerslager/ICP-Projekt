# ICP Projekt

C++20 OpenGL project (GLFW, GLEW, GLM, ImGui, nlohmann-json, OpenCV).
Libraries except OpenCV are fetched automatically by vcpkg. CMake and OpenCV must be installed manually.

## Prerequisites (Windows)

1. **Visual Studio 2026** with the *Desktop development with C++* workload.
   Make sure the *vcpkg package manager* component is checked (Individual components).
2. **CMake 4.0+** from https://cmake.org/download/ — tick *Add CMake to the system PATH* during install.
3. **OpenCV** (tested with 5.0.0) Windows release from https://opencv.org/releases/.
   Extract it so that `C:/OpenCV/build/OpenCVConfig.cmake` exists.
   A different location works too — change the path in `CMakeLists.txt` (`find_package(OpenCV ... PATHS ...)`).
4. Environment variables (System Properties → Environment Variables), then **restart VS / terminals**:
   - `VCPKG_ROOT` = `C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg`
     (or your own vcpkg clone)
   - add the OpenCV DLL folder to `Path`, e.g. `C:\OpenCV\build\x64\vc16\bin`
     (without it the program builds but fails to start: `opencv_world*.dll was not found`)

## Build and run

### Option A: Visual Studio solution

```
cmake --preset vs2026
```

Open `build/vs/icp.slnx` in Visual Studio, press F5. `icp` is already the startup project
and the working directory is set to the repo root, so `resources/` is found.

The first configure takes a while — vcpkg downloads and builds the dependencies.

### Option B: Open folder

In Visual Studio: *File → Open → Folder* and select the repo root. VS reads `CMakePresets.json`;
pick a preset (`vs2026`, `vcpkg-debug`, `vcpkg-release`) in the toolbar and run.

## Expected output

Terminal:

```
Hello!
Hello World! vec3(0.000000, 0.000000, 0.000000)
Current working directory: <repo root>
"first_test"
[x,y] = [1024,768]
Image size: 468,323
2.2.0
```

Plus two windows: `test_window` (one camera frame, black without a camera) and a black
GLFW window `test`. Press **ESC** in the GLFW window to quit; `Bye!` is printed.

## Troubleshooting

- `Could not find a package configuration file provided by "OpenCV"` — OpenCV not at `C:/OpenCV/build`, see step 3.
- `Could not find toolchain file: /scripts/buildsystems/vcpkg.cmake` — `VCPKG_ROOT` not set, or VS/terminal not restarted.
- `Directory 'resources' not found` — program started from the wrong working directory; run it from the repo root.
