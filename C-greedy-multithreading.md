# Person C — Greedy and Multithreading

**Branch:** `feat/greedy` · **Files:** `algos/greedy_seam.*`, `core/energy_mt.*`
**Depends on:** A's `types.h` (Tue morning). **Target:** greedy merged Tue night, threading Wed night.

## Concept first

**Greedy** means: at every step take the option that looks best right now, and never look ahead or go back.

For seams: start at the lowest-energy pixel in the top row. Then in each next row, look only at the 3 pixels below you and step to the cheapest. That is it.

It is fast and simple, but it can be wrong. A cheap first step can lead into an expensive region that it cannot escape from. DP avoids this because it effectively considers every path. Your job is to implement greedy, then show clearly where and why it loses.

## Part 1 — Greedy

### 1. See it fail on paper first

```
e =  1 2 3
     9 9 1
     9 9 1
```

- Greedy starts at column 0 (energy 1, the smallest in row 0). Below it the choices are 9 and 9. Total = 1 + 9 + 9 = **19**.
- DP finds column 1 → 2 → 2: 2 + 1 + 1 = **4**.

Greedy grabbed the 1 and paid for it later. Keep this grid: it is your unit test and your slide.

### 2. Implement `GreedySeamFinder`

```cpp
class GreedySeamFinder : public ISeamFinder {
public:
    Seam find(const cv::Mat1f& energy) override;
    const char* name() const override { return "greedy"; }
};
```

Logic of `find`:

1. `seam[0]` = column of the minimum in row 0.
2. For each row `i` from 1: let `j = seam[i-1]`. Compare `e(i, j-1)`, `e(i, j)`, `e(i, j+1)`, skipping any that are outside the image. `seam[i]` = the column of the smallest.
3. **Tie rule:** leftmost wins, same as B. Comment it.

### 3. Check it

- On the grid above it returns `{0, 0, 0}` with cost 19.
- `isValidSeam` passes on a real image.
- On random grids, `seamCost(greedy) >= seamCost(dp)` always. It can be equal, never lower.

Tell A the file names for CMake and E the class name.

### 4. Collect comparison evidence (give to F)

- One real photo where greedy and DP give visibly different results after removing about 100 seams. Photos with a clear subject and a busy area work well.
- For the same image: the seam overlay from each (ask E for `drawSeam`).

### 5. Complexity

- Finding one greedy seam: O(n) to scan row 0, then 3 comparisons per row, so O(n + m). DP is O(m·n).
- **But** each iteration still recomputes the energy map, which is O(m·n). So the total per seam is O(m·n) for both. Greedy is faster only by a constant factor. Say this in the report; it explains the timing numbers.

## Part 2 — Multithreading (for 5d, "does multithreading help?")

### Concept

Energy at each pixel depends only on the input image, never on another pixel's energy. So different threads can compute different rows at the same time with no conflict. DP is different: row `i` needs row `i-1` finished first, so rows cannot run in parallel (only the cells within one row can).

### 6. Write a plain single-threaded Sobel

`cv::Mat1f computeEnergyST(const cv::Mat1b& gray)` using your own loops over the 3×3 Sobel kernels. You need this because `cv::Sobel` is already heavily optimised internally, so comparing against it would not be a fair test of threading.

Skip the 1-pixel border (set it to 0) to keep the loops simple.

### 7. Write the threaded version

`cv::Mat1f computeEnergyMT(const cv::Mat1b& gray, int threads)`

- Split rows `1 .. rows-2` into `threads` equal bands.
- One `std::thread` per band, each running the same inner loop as step 6 on its band.
- `join` them all.
- No locks needed: each thread writes to its own rows.

Check: output is identical to `computeEnergyST`.

### 8. Measure

- Images: small (about 200×150), medium (about 1000×750), large (about 4000×3000; upscale one if needed).
- Threads: 1, 2, 4, 8.
- Run each 20 times and take the average. Build in Release mode.
- Record speed-up = time(1 thread) / time(N threads).

What you will likely see and should explain: near-linear speed-up on the large image, little or negative gain on the small one (creating threads costs more than the work), and flattening beyond the number of physical cores.

## Done when

- [x] Greedy merged, selectable with `--algo greedy`
- [x] Counter-example test passes
- [ ] Threading table (3 sizes × 4 thread counts) handed to F

## Report sections you write (by Thu night)

- **Greedy:** algorithm, the 3×3 counter-example, why it is not optimal, complexity.
- **Does it match DP visually?** Your side-by-side images and a short verdict.
- **Multithreading:** what can and cannot be parallelised and why, your speed-up table, the overhead explanation.

## Presentation (2 min)

The counter-example grid, one side-by-side image, the speed-up chart. Hand over to D.
