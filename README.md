# Seam Carving (CSD3130 Assignment 2)

## Build

Linux / WSL:

```bash
sudo apt install -y cmake g++ libopencv-dev
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Windows: install OpenCV (e.g. `vcpkg install opencv4`), then configure with
CMake and point `OpenCV_DIR` at the OpenCV install if it is not found.

## Run

```bash
./build/seamcarve --in input.jpg --out out.png --width 300 --height 200
```

## Layout

| Path | Owner | Contents |
|---|---|---|
| `common/types.h` | A | `Seam`, `ISeamFinder`, `StepCallback` (frozen) |
| `core/` | A | `computeEnergy`, `removeVerticalSeam`, `isValidSeam`, `seamCost`, `carve` |
| `algos/` | B, C, D | seam finders (DP, greedy, graph cut) |
| `app/` | E | CLI, UI, visualisation |
| `tests/`, `bench/` | F | unit tests, benchmarks |

Only Person A edits `CMakeLists.txt`; send new file names to A.
