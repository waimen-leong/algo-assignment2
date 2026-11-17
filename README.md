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

### App options (Person E)

| Flag | Meaning | Default |
|---|---|---|
| `--in <path>` | input image (required) | — |
| `--out <path>` | output image | `out.png` |
| `--width <px>` | target width (shrink only) | original width |
| `--height <px>` | target height (shrink only) | original height |
| `--algo dp\|greedy\|graphcut\|stub` | seam finder | `dp` |
| `--step` | open a window and show each red seam, waiting for a key press per seam | off |
| `--interactive` | open the slider UI (bonus b); ignores `--width/--height/--out` | off |
| `--frames <dir>` | write `frame_0001.png`, `frame_0002.png`, … one per seam, each on a canvas the size of the original (ready for video) | off |

An algorithm name whose finder has not merged yet falls back to the stub, with
a note on stderr, so the app always runs. Only `dp` exists once Person B merges;
`greedy` and `graphcut` light up as C and D merge (see `makeFinder` in
`app/args.cpp`).

### Visualisation (bonus a)

```bash
# watch DP remove seams, one key press at a time
./build/seamcarve --in input.jpg --width 300 --algo dp --step

# export a frame per seam for the report / a video
./build/seamcarve --in input.jpg --width 300 --algo dp --frames frames/
ffmpeg -framerate 30 -i frames/frame_%04d.png -pix_fmt yuv420p demo.mp4
```

Frames are the single output F needs for the report figures and the output C
and D need for seam comparisons: same canvas size every step.

### Interactive mode (bonus b)

```bash
./build/seamcarve --in input.jpg --interactive
```

| Key | Action |
|---|---|
| `1` `2` `3` | switch algorithm: DP, greedy, graph cut |
| `Space` | carve from the original down to the slider sizes (animated) |
| `r` | reset to the original |
| `s` | save the current result (`interactive_NNN.png`) |
| `q` / `Esc` | quit |

Width and Height sliders set the target size. Every carve starts from the
original, so dragging a slider back up restores detail. A status line on the
window shows the algorithm, current size and last run time. Graph cut on a
large image is downscaled first (with an on-window note) to stay responsive.

## Layout

| Path | Owner | Contents |
|---|---|---|
| `common/types.h` | A | `Seam`, `ISeamFinder`, `StepCallback` (frozen) |
| `core/` | A | `computeEnergy`, `removeVerticalSeam`, `isValidSeam`, `seamCost`, `carve` |
| `algos/` | B, C, D | seam finders (DP, greedy, graph cut) |
| `app/` | E | CLI, UI, visualisation |
| `tests/`, `bench/` | F | unit tests, benchmarks |

Only Person A edits `CMakeLists.txt`; send new file names to A.
