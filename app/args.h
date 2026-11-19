#pragma once
#include "common/types.h"

#include <memory>
#include <string>

// Parsed command-line options for the app.
// width / height == -1 means "use the original image size for that dimension".
struct Args {
    std::string in;              // --in  <path>   (required)
    std::string out = "out.png"; // --out <path>
    int width = -1;              // --width  <px>  (-1 -> original width)
    int height = -1;             // --height <px>  (-1 -> original height)
    std::string algo = "dp";     // --algo dp|greedy|graphcut|stub
    bool step = false;           // --step        (wait for key each seam)
    bool interactive = false;    // --interactive (slider UI)
    std::string framesDir;       // --frames <dir> (dump frame_NNNN.png)
    bool showHelp = false;       // --help / -h
};

// Parse argv into Args. Throws std::invalid_argument with a human-readable
// message on an unknown flag, a missing value, or a bad integer.
Args parseArgs(int argc, char** argv);

// One-line reminder of the accepted options.
std::string usage();

// The single place in the project that knows every algorithm class name.
// Returns the stub finder for an unknown name so the pipeline always runs;
// B, C and D each add one line here as their finder merges.
std::unique_ptr<ISeamFinder> makeFinder(const std::string& name);
