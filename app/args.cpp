#include "app/args.h"
#include "algos/stub_seam.h"

// Real finders appear as B, C and D merge. Each teammate adds their header
// here and defines the matching HAVE_* macro (or we detect __has_include).
// Until then every name falls back to the stub so the whole app still runs.
#if defined(__has_include)
#  if __has_include("algos/dp_seam.h")
#    include "algos/dp_seam.h"
#    define HAVE_DP 1
#  endif
#  if __has_include("algos/greedy_seam.h")
#    include "algos/greedy_seam.h"
#    define HAVE_GREEDY 1
#  endif
#  if __has_include("algos/graphcut_seam.h")
#    include "algos/graphcut_seam.h"
#    define HAVE_GRAPHCUT 1
#  endif
#endif

#include <stdexcept>
#include <string>

namespace {
int parseInt(const std::string& flag, const std::string& val) {
    try {
        size_t pos = 0;
        int n = std::stoi(val, &pos);
        if (pos != val.size()) throw std::invalid_argument("trailing chars");
        return n;
    } catch (const std::exception&) {
        throw std::invalid_argument(flag + " expects an integer, got '" + val + "'");
    }
}
}  // namespace

std::string usage() {
    return "usage: seamcarve --in <path> [--out <path>] [--width <px>] "
           "[--height <px>]\n"
           "                 [--algo dp|greedy|graphcut|stub] [--step] "
           "[--interactive]\n"
           "                 [--frames <dir>]\n"
           "defaults: width/height = original size, algo = dp";
}

Args parseArgs(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::string key = argv[i];

        if (key == "--help" || key == "-h") { a.showHelp = true; return a; }
        if (key == "--step")        { a.step = true; continue; }
        if (key == "--interactive") { a.interactive = true; continue; }

        // Everything else needs a value.
        if (i + 1 >= argc)
            throw std::invalid_argument(key + " expects a value");
        std::string val = argv[++i];

        if      (key == "--in")     a.in = val;
        else if (key == "--out")    a.out = val;
        else if (key == "--algo")   a.algo = val;
        else if (key == "--frames") a.framesDir = val;
        else if (key == "--width")  a.width = parseInt(key, val);
        else if (key == "--height") a.height = parseInt(key, val);
        else throw std::invalid_argument("unknown option " + key);
    }

    if (a.width != -1 && a.width < 1)
        throw std::invalid_argument("--width must be >= 1");
    if (a.height != -1 && a.height < 1)
        throw std::invalid_argument("--height must be >= 1");

    return a;
}

std::unique_ptr<ISeamFinder> makeFinder(const std::string& name) {
    if (name == "stub") return std::make_unique<StubSeamFinder>();
#ifdef HAVE_DP
    if (name == "dp") return std::make_unique<DPSeamFinder>();
#endif
#ifdef HAVE_GREEDY
    if (name == "greedy") return std::make_unique<GreedySeamFinder>();
#endif
#ifdef HAVE_GRAPHCUT
    if (name == "graphcut") return std::make_unique<GraphCutSeamFinder>();
#endif
    // Unknown or not-yet-merged algorithm: fall back to the stub so the
    // pipeline still runs. main() prints a note when this happens.
    return std::make_unique<StubSeamFinder>();
}
