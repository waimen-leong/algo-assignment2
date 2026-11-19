# SeamDetect.cmake — finds the team's headers wherever they live in the repo,
# and works out which seam finders have been merged so far.
#
# Why: the tests and benchmark should compile on `main` at every stage of the
# week. If greedy has merged but graph cut hasn't, the graph cut tests are
# simply skipped instead of breaking the build.
#
# After including this file you get:
#   SEAM_DETECT_DEFINITIONS  — a list of compile definitions to pass to a target:
#       SEAM_TYPES_HEADER="/path/to/types.h"   (and ENERGY, CARVER likewise)
#       SEAM_HAVE_DP=1 + SEAM_DP_HEADER="..."  (only if DPSeamFinder exists)
#       SEAM_HAVE_DP_LOWMEM=1 + ...            (only if DPSeamFinderLowMem exists)
#       SEAM_HAVE_GREEDY=1 + ...               (only if GreedySeamFinder exists)
#       SEAM_HAVE_GRAPHCUT=1 + ...             (only if GraphCutSeamFinder exists)

file(GLOB_RECURSE _seam_all_headers
    "${PROJECT_SOURCE_DIR}/*.h" "${PROJECT_SOURCE_DIR}/*.hpp")
# Ignore build output, our own folders and anything third-party.
list(FILTER _seam_all_headers EXCLUDE REGEX "/(build[^/]*|tests|bench|_deps|third_party|external)/")

set(SEAM_DETECT_DEFINITIONS "")

# --- Core headers, found by file name -------------------------------------
function(_seam_find_file_named out_var filename)
    set(${out_var} "" PARENT_SCOPE)
    foreach(h IN LISTS _seam_all_headers)
        get_filename_component(_name "${h}" NAME)
        if(_name STREQUAL "${filename}")
            set(${out_var} "${h}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()

foreach(_pair "TYPES;types.h" "ENERGY;energy.h" "CARVER;carver.h")
    list(GET _pair 0 _key)
    list(GET _pair 1 _file)
    _seam_find_file_named(_path "${_file}")
    if(_path)
        list(APPEND SEAM_DETECT_DEFINITIONS "SEAM_${_key}_HEADER=\"${_path}\"")
        message(STATUS "SeamDetect: ${_file} -> ${_path}")
    else()
        message(WARNING "SeamDetect: could not find ${_file}. Edit tests/seam_api.h to include the right header.")
    endif()
endforeach()

# --- Seam finders, found by class name -------------------------------------
function(_seam_find_class out_var cls)
    set(${out_var} "" PARENT_SCOPE)
    foreach(h IN LISTS _seam_all_headers)
        file(STRINGS "${h}" _hits REGEX "(class|struct)[ \t]+${cls}([ \t:{]|$)")
        if(_hits)
            set(${out_var} "${h}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()

foreach(_pair "DP;DPSeamFinder" "DP_LOWMEM;DPSeamFinderLowMem"
              "GREEDY;GreedySeamFinder" "GRAPHCUT;GraphCutSeamFinder")
    list(GET _pair 0 _key)
    list(GET _pair 1 _cls)
    _seam_find_class(_path "${_cls}")
    if(_path)
        list(APPEND SEAM_DETECT_DEFINITIONS "SEAM_HAVE_${_key}=1" "SEAM_${_key}_HEADER=\"${_path}\"")
        message(STATUS "SeamDetect: found ${_cls} in ${_path}")
    else()
        message(STATUS "SeamDetect: ${_cls} not merged yet — its tests are skipped")
    endif()
endforeach()
