#!/usr/bin/env python3
"""plot_results.py - turns the benchmark CSVs into the section 5(c) charts and tables.

Usage (after running seam_bench):
    python3 bench/plot_results.py results.csv timing_breakdown.csv --outdir figures

Needs:  pip install matplotlib   (nothing else)

Makes:
    running_time.png     ms per seam vs image size, one line per algorithm, log time axis
    seam_cost.png        total cost of the first N seams, per algorithm, per size
    time_breakdown.png   where the time goes: energy map vs seam finding vs removal
    tables.md            the same numbers as Markdown tables, ready to paste into the report
"""
import argparse
import csv
import math
import os
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")  # no window needed
import matplotlib.pyplot as plt
import matplotlib.ticker

# Each algorithm always keeps the same colour and marker in every chart.
ALGO_STYLE = {
    "dp":        ("#2a78d6", "o", "DP"),
    "dp_lowmem": ("#1baf7a", "D", "DP (low memory)"),
    "greedy":    ("#eb6834", "s", "Greedy"),
    "graphcut":  ("#eda100", "^", "Graph cut"),
}
ALGO_ORDER = ["dp", "dp_lowmem", "greedy", "graphcut"]
# Breakdown parts use different hues from the algorithms, so blue never means two things.
PART_STYLE = [("energy_ms_per_seam", "#e87ba4", "Energy map"),
              ("find_ms_per_seam", "#008300", "Seam finding"),
              ("remove_ms_per_seam", "#6250d6", "Seam removal")]

INK, MUTED, GRID, SURFACE = "#1f1f1e", "#6b6a64", "#e4e3de", "#ffffff"


def style_for(algo):
    if algo in ALGO_STYLE:
        return ALGO_STYLE[algo]
    return ("#6b6a64", "x", algo)  # unknown name: neutral grey, still labelled


def ordered(algos):
    known = [a for a in ALGO_ORDER if a in algos]
    return known + sorted(a for a in algos if a not in ALGO_ORDER)


def read_csv(path):
    with open(path, newline="") as f:
        return list(csv.DictReader(f))


def setup_axes(ax, title, ylabel):
    ax.set_facecolor(SURFACE)
    ax.set_title(title, loc="left", fontsize=12, color=INK, pad=12)
    ax.set_ylabel(ylabel, color=MUTED)
    ax.tick_params(colors=MUTED, length=0)
    for side in ("top", "right", "left"):
        ax.spines[side].set_visible(False)
    ax.spines["bottom"].set_color(GRID)
    ax.grid(axis="y", color=GRID, linewidth=0.8)
    ax.set_axisbelow(True)


def size_label(w, h):
    return f"{w}×{h}"


def plot_running_time(rows, out):
    by_algo = defaultdict(list)
    for r in rows:
        by_algo[r["algo"]].append((int(r["width"]) * int(r["height"]), int(r["width"]),
                                   int(r["height"]), float(r["ms_per_seam"])))
    sizes = sorted({(int(r["width"]) * int(r["height"]), int(r["width"]), int(r["height"])) for r in rows})

    fig, ax = plt.subplots(figsize=(7.5, 4.5), dpi=200)
    fig.patch.set_facecolor(SURFACE)
    setup_axes(ax, "Running time per seam (log scale)", "milliseconds per seam")
    ax.set_yscale("log")
    ax.set_xscale("log")
    ends = []
    for algo in ordered(by_algo):
        pts = sorted(by_algo[algo])
        color, marker, label = style_for(algo)
        xs = [p[0] for p in pts]
        ys = [p[3] for p in pts]
        ax.plot(xs, ys, color=color, linewidth=2, marker=marker, markersize=8,
                markeredgecolor=SURFACE, markeredgewidth=1.5, label=label)
        ends.append([math.log10(ys[-1]), xs[-1], label])
    # Direct labels at line ends, nudged apart (in log space) so they never overlap.
    ends.sort()
    gap = 0.09
    for i in range(1, len(ends)):
        if ends[i][1] == ends[i - 1][1] and ends[i][0] - ends[i - 1][0] < gap:
            ends[i][0] = ends[i - 1][0] + gap
    for logy, x, label in ends:
        ax.annotate(label, (x, 10 ** logy), xytext=(9, 0), textcoords="offset points",
                    va="center", fontsize=9, color=INK)
    ax.yaxis.set_major_formatter(matplotlib.ticker.FuncFormatter(lambda v, _: f"{v:g}"))
    ax.set_xticks([s[0] for s in sizes])
    ax.set_xticklabels([size_label(s[1], s[2]) for s in sizes])
    ax.minorticks_off()
    ax.set_xlabel("image size (width × height)", color=MUTED)
    ax.margins(x=0.12)
    ax.legend(frameon=False, loc="upper left", fontsize=9, labelcolor=INK)
    fig.tight_layout()
    fig.savefig(out, facecolor=SURFACE)
    plt.close(fig)


def grouped_bars(ax, groups, algos, value_of, fmt):
    n = len(algos)
    width = 0.8 / max(n, 1)
    for i, algo in enumerate(algos):
        color, _, label = style_for(algo)
        xs, ys = [], []
        for g, key in enumerate(groups):
            v = value_of(key, algo)
            if v is None:
                continue
            xs.append(g - 0.4 + width * (i + 0.5))
            ys.append(v)
        # Bars slightly narrower than their slot leave a gap between neighbours.
        bars = ax.bar(xs, ys, width=width * 0.88, color=color, label=label)
        if fmt is None:
            continue
        for b, key in zip(bars, [k for k in groups if value_of(k, algo) is not None]):
            text = fmt(key, algo)
            if text:
                ax.annotate(text, (b.get_x() + b.get_width() / 2, b.get_height()), xytext=(0, 3),
                            textcoords="offset points", ha="center", va="bottom", fontsize=8, color=INK)


def plot_seam_cost(rows, out):
    sizes = sorted({(int(r["width"]), int(r["height"])) for r in rows}, key=lambda s: s[0] * s[1])
    algos = ordered({r["algo"] for r in rows})
    cost = {(int(r["width"]), int(r["height"]), r["algo"]): float(r["total_seam_cost"]) for r in rows}
    seams = rows[0]["seams"] if rows else "?"

    fig, ax = plt.subplots(figsize=(7.5, 4.5), dpi=200)
    fig.patch.set_facecolor(SURFACE)
    setup_axes(ax, f"Total energy of the first {seams} seams (lower is better)", "total seam cost")
    def greedy_vs_dp(s, a):  # only greedy gets a label: how much worse than optimal
        if a != "greedy" or (s[0], s[1], "dp") not in cost:
            return ""
        dp = cost[(s[0], s[1], "dp")]
        return f"+{(cost[(s[0], s[1], a)] - dp) / dp:.0%}" if dp > 0 else ""

    grouped_bars(ax, sizes, algos, lambda s, a: cost.get((s[0], s[1], a)), greedy_vs_dp)
    ax.yaxis.set_major_formatter(matplotlib.ticker.FuncFormatter(lambda v, _: f"{v:,.0f}"))
    ax.text(0, -0.2, "Label on greedy bars: extra cost compared with DP (the optimum).",
            transform=ax.transAxes, fontsize=8, color=MUTED)
    ax.set_xticks(range(len(sizes)))
    ax.set_xticklabels([size_label(*s) for s in sizes])
    ax.set_xlabel("image size (width × height)", color=MUTED)
    ax.legend(frameon=False, fontsize=9, labelcolor=INK, ncol=len(algos), loc="upper left")
    ax.margins(y=0.15)
    fig.tight_layout()
    fig.savefig(out, facecolor=SURFACE)
    plt.close(fig)


def plot_breakdown(rows, out):
    # One horizontal stacked bar per (size, algorithm). Each part gets its own
    # panel-free stack; values are shares of the total so sizes are comparable.
    keys = sorted({(int(r["width"]), int(r["height"]), r["algo"]) for r in rows},
                  key=lambda k: (k[0] * k[1], ALGO_ORDER.index(k[2]) if k[2] in ALGO_ORDER else 99))
    data = {(int(r["width"]), int(r["height"]), r["algo"]): r for r in rows}

    fig, ax = plt.subplots(figsize=(7.5, 0.42 * len(keys) + 1.6), dpi=200)
    fig.patch.set_facecolor(SURFACE)
    ax.set_facecolor(SURFACE)
    ax.set_title("Where the time goes (share of time per seam)", loc="left", fontsize=12, color=INK, pad=12)
    for i, k in enumerate(keys):
        r = data[k]
        parts = [float(r[col]) for col, _, _ in PART_STYLE]
        total = sum(parts) or 1.0
        left = 0.0
        for (col, color, label), v in zip(PART_STYLE, parts):
            share = v / total
            ax.barh(i, share, left=left, color=color, edgecolor=SURFACE, linewidth=2, height=0.7,
                    label=label if i == 0 else None)
            if share >= 0.12:
                ax.text(left + share / 2, i, f"{share:.0%}", ha="center", va="center", fontsize=7,
                        color="#ffffff")
            left += share
        ax.text(1.01, i, f"{total:.3g} ms", va="center", fontsize=7, color=MUTED)
    ax.set_yticks(range(len(keys)))
    ax.set_yticklabels([f"{style_for(k[2])[2]}, {size_label(k[0], k[1])}" for k in keys], fontsize=8,
                       color=INK)
    ax.invert_yaxis()
    ax.set_xlim(0, 1)
    ax.xaxis.set_major_formatter(matplotlib.ticker.PercentFormatter(1.0))
    ax.tick_params(colors=MUTED, length=0)
    for side in ("top", "right", "left", "bottom"):
        ax.spines[side].set_visible(False)
    ax.legend(frameon=False, fontsize=8, labelcolor=INK, ncol=3, loc="upper left",
              bbox_to_anchor=(0, -0.04 * 14 / max(len(keys), 1)))
    fig.tight_layout()
    fig.savefig(out, facecolor=SURFACE)
    plt.close(fig)


def write_tables(results, breakdown, out):
    algos = ordered({r["algo"] for r in results})
    sizes = sorted({(int(r["width"]), int(r["height"])) for r in results}, key=lambda s: s[0] * s[1])
    res = {(int(r["width"]), int(r["height"]), r["algo"]): r for r in results}
    seams = results[0]["seams"] if results else "?"

    lines = ["## Running time (ms per seam)", "",
             "| Size | " + " | ".join(style_for(a)[2] for a in algos) + " |",
             "|---|" + "---:|" * len(algos)]
    for s in sizes:
        cells = [f"{float(res[(*s, a)]['ms_per_seam']):.3f}" if (*s, a) in res else "—" for a in algos]
        lines.append(f"| {size_label(*s)} | " + " | ".join(cells) + " |")

    lines += ["", f"## Total cost of the first {seams} seams", "",
              "| Size | " + " | ".join(style_for(a)[2] for a in algos) + " |",
              "|---|" + "---:|" * len(algos)]
    for s in sizes:
        cells = [f"{float(res[(*s, a)]['total_seam_cost']):,.1f}" if (*s, a) in res else "—" for a in algos]
        lines.append(f"| {size_label(*s)} | " + " | ".join(cells) + " |")

    if breakdown:
        lines += ["", "## Time per seam, split by step (ms)", "",
                  "| Algorithm | Size | Energy map | Seam finding | Seam removal |",
                  "|---|---|---:|---:|---:|"]
        for r in sorted(breakdown, key=lambda r: (int(r["width"]) * int(r["height"]),
                                                  ALGO_ORDER.index(r["algo"]) if r["algo"] in ALGO_ORDER else 99)):
            lines.append(f"| {style_for(r['algo'])[2]} | {size_label(int(r['width']), int(r['height']))} | "
                         f"{float(r['energy_ms_per_seam']):.3f} | {float(r['find_ms_per_seam']):.3f} | "
                         f"{float(r['remove_ms_per_seam']):.3f} |")
    lines.append("")
    with open(out, "w") as f:
        f.write("\n".join(lines))


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("results", nargs="?", default="results.csv")
    p.add_argument("breakdown", nargs="?", default="timing_breakdown.csv")
    p.add_argument("--outdir", default="figures")
    a = p.parse_args()

    os.makedirs(a.outdir, exist_ok=True)
    results = read_csv(a.results)
    breakdown = read_csv(a.breakdown) if os.path.exists(a.breakdown) else []

    plot_running_time(results, os.path.join(a.outdir, "running_time.png"))
    plot_seam_cost(results, os.path.join(a.outdir, "seam_cost.png"))
    if breakdown:
        plot_breakdown(breakdown, os.path.join(a.outdir, "time_breakdown.png"))
    write_tables(results, breakdown, os.path.join(a.outdir, "tables.md"))
    print("Wrote charts and tables.md to", a.outdir)


if __name__ == "__main__":
    main()
