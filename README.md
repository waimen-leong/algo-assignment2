# Seam Carving (CSD3130 Assignment 2)

![CI](https://github.com/waimen-leong/algo-assignment2/actions/workflows/ci.yml/badge.svg)

## Build

Linux / WSL:

```bash
sudo apt install -y cmake g++ libopencv-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release   # configure (only writes build files)
cmake --build build -j                      # compile everything: app, tests, benchmarks
```

Run `cmake --build build -j` again after every code change. Configuring alone
does not recompile anything. Release is the default build type, so leave it
that way: Debug is far too slow for seam carving and gives meaningless benchmark numbers.

The build produces:

| Program | What it is |
|---|---|
| `build/seamcarve` | the app |
| `build/tests/seam_tests`, `test_core`, `test_dp`, `test_greedy`, `test_graphcut` | tests |
| `build/bench/seam_bench`, `seam_figures`, `bench_energy` | benchmarks and report figures |

Windows: install OpenCV (e.g. `vcpkg install opencv4`), then configure with
CMake and point `OpenCV_DIR` at the OpenCV install if it is not found.

## Run

```bash
./build/seamcarve --in input.jpg --out out.png --width 300 --height 200
```

### App options (Kai Yang)

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
a note on stderr, so the app always runs. Only `dp` exists once Shahir merges;
`greedy` and `graphcut` light up as Wai Men and Jun Siang merge (see `makeFinder` in
`app/args.cpp`).

### Visualisation (bonus a)

```bash
# watch DP remove seams, one key press at a time
./build/seamcarve --in input.jpg --width 300 --algo dp --step

# export a frame per seam for the report / a video
./build/seamcarve --in input.jpg --width 300 --algo dp --frames frames/
ffmpeg -framerate 30 -i frames/frame_%04d.png -pix_fmt yuv420p demo.mp4
```

Frames are the single output Jedrek needs for the report figures and the output Wai Men
and Jun Siang need for seam comparisons: same canvas size every step.

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

## Tests

```bash
ctest --test-dir build --output-on-failure   # run every test program (CI runs exactly this)
./build/tests/seam_tests                     # the doctest suite on its own, with a per-case summary
./build/tests/seam_tests --list-test-cases   # list the individual test cases
./build/tests/test_dp                        # or any single standalone test program
```

| ctest entry | Source | Checks |
|---|---|---|
| `seam_tests` | `test_core_units.cpp`, `test_algorithms.cpp` (doctest) | energy, seam removal, `isValidSeam`, carving; DP / DP low-mem / greedy / graph cut against brute force and each other |
| `test_core` | `test_core.cpp` | core sanity checks |
| `test_dp` | `test_dp.cpp` | DP worked example, cost table, leftmost tie rule, brute force, low-mem gives the same seam |
| `test_greedy` | `test_greedy.cpp` | greedy counter-example, ties, valid seams, single- vs multi-threaded energy |
| `test_graphcut` | `test_graphcut.cpp` | max flow, graph cut cost equals the DP optimum |

The doctest algorithm tests switch on automatically for each seam finder that
exists in the code (`tests/SeamDetect.cmake`).
Tests never open a window.

## Benchmarks and report figures

Build in Release first (see above). Each program writes its CSV / images into the
**current directory**, so run them from the repo root.

```bash
# Seam finder timing + seam cost (100x75 .. 800x600, 20 seams, 3 runs each)
#   -> results.csv, timing_breakdown.csv
./build/bench/seam_bench
./build/bench/seam_bench --image photo.jpg            # use a real image, resized to each size
./build/bench/seam_bench --graphcut-max-width 400     # run graph cut on bigger sizes (default 200)

# Energy map: single- vs multi-threaded (1/2/4/8/16 threads)
#   -> energy_bench.csv
./build/bench/bench_energy

# Charts and Markdown tables for section 5(c) (needs: pip install matplotlib)
#   -> figures/running_time.png, seam_cost.png, time_breakdown.png, tables.md
python3 bench/plot_results.py results.csv timing_breakdown.csv --outdir figures

# Seam overlays and side-by-side quality comparison (scaled | cropped | each algorithm)
./build/bench/seam_figures --image beach.jpg --width 300 --outdir figs/beach
./build/bench/seam_figures --image big.jpg --width 300 --resize-width 200 --outdir figs/big  # shrink first so graph cut runs in reasonable time
./build/bench/seam_figures --image big.jpg --width 300 --skip-graphcut --outdir figs/big
```

## Layout

| Path | Owner | Contents |
|---|---|---|
| `common/types.h` | Javier | `Seam`, `ISeamFinder`, `StepCallback` (frozen) |
| `core/` | Javier | `computeEnergy`, `removeVerticalSeam`, `isValidSeam`, `seamCost`, `carve` |
| `algos/` | Shahir, Wai Men, Jun Siang | seam finders (DP, greedy, graph cut) |
| `app/` | Kai Yang | CLI, UI, visualisation |
| `tests/`, `bench/` | Jedrek | unit tests, benchmarks |

Only Javier edits `CMakeLists.txt`; send new file names to Javier.
