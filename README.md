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

The app opens a window with an OpenGL 4.6 CORE debug context. Terminal (vendor/renderer depend on your GPU):

```
GLFW 3.4.0 Win32 WGL Null EGL OSMesa VisualC DLL
GLEW 2.2.0
GL vendor:   NVIDIA Corporation
GL renderer: NVIDIA GeForce RTX 3060 Laptop GPU/PCIe/SSE2
GL version:  4.6.0 NVIDIA 576.02
GLSL:        4.60 NVIDIA
GL context:  4.6 CORE profile
GL flags:    DEBUG
GL_DEBUG enabled.
```

The window title shows `<appname> | FPS: <n> | VSync: ON/OFF`, updated every second.
Mouse clicks and scroll-wheel events are printed to the terminal. `Bye!` is printed on exit.

On notebooks with two GPUs, `src/gpu_select.cpp` asks the NVIDIA/AMD driver to use the dedicated GPU.

### Controls

| Key   | Action        |
|-------|---------------|
| ESC   | Quit          |
| V     | Toggle VSync  |

## Configuration

`resources/app_settings.json` (read from the working directory at startup):

```json
{
  "appname": "first_test",
  "default_resolution": { "x": 1024, "y": 768 },
  "vsync": true
}
```

Every key is optional — missing keys or a missing file fall back to defaults (`ICP`, 800×600, VSync on).
Malformed JSON or a non-positive window size aborts with an error.

## Troubleshooting

- `Could not find a package configuration file provided by "OpenCV"` — OpenCV not at `C:/OpenCV/build`, see step 3.
- `Could not find toolchain file: /scripts/buildsystems/vcpkg.cmake` — `VCPKG_ROOT` not set, or VS/terminal not restarted.
- `Config 'resources/app_settings.json' not found, using defaults.` — program started from the wrong working directory; run it from the repo root.
- `Failed to create window with OpenGL 4.6 CORE context.` — GPU/driver does not support GL 4.6; update the driver
  (on a notebook make sure the dedicated GPU is used).
- `glewinfo` is not included in the vcpkg GLEW package — to compare GL info, download the GLEW Windows binaries from
  https://glew.sourceforge.net and run `bin/Release/x64/glewinfo.exe`.
