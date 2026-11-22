// Tetra3 lost-in-space plate solver, in C++.
//
// A port of Tetra3.solve_from_centroids (ESA, Apache 2.0) for the flat
// database written by tool/tetra_flat.py. Centroids are (y, x) pixels from
// the top-left corner, brightest first.
#pragma once

#include <cstdint>

#if defined(_WIN32)
#define TETRA_API extern "C" __declspec(dllexport)
#else
#define TETRA_API extern "C" __attribute__((visibility("default")))
#endif

struct TetraSolution {
    int32_t solved;
    int32_t timed_out;
    int32_t matches;
    int32_t epoch_equinox;
    double ra_deg;
    double dec_deg;
    // Tetra3's roll: atan2(R[1][2], R[2][2]), 0..360.
    double roll_deg;
    // Horizontal field.
    double fov_deg;
    double distortion;
    double rmse_arcsec;
    double prob;
    double t_solve_ms;
    // camera = R * sky. Row 0 is the boresight, row 1 points to image left,
    // row 2 to image up.
    double rotation[9];
};

struct TetraMatch {
    // Index into the centroids that were passed.
    int32_t image_index;
    // Bright Star Catalogue number.
    int32_t cat_id;
    double ra_deg;
    double dec_deg;
    double mag;
};

// Memory for callers without an allocator of their own (Dart FFI).
TETRA_API void* tetra_alloc(uint64_t bytes);
TETRA_API void tetra_free(void* pointer);

TETRA_API void* tetra_open_file(const char* path);
TETRA_API void* tetra_open_memory(const uint8_t* bytes, uint64_t length);
TETRA_API void tetra_close(void* db);
TETRA_API int32_t tetra_star_count(void* db);
TETRA_API double tetra_max_fov(void* db);

// distortion_min == distortion_max is a known distortion. Otherwise the range
// is searched, as Tetra3 does for a (min, max) tuple. Returns 1 when solved,
// 0 when not, -1 when the arguments are not usable.
TETRA_API int32_t tetra_solve(void* db,
                              const double* centroids_yx,
                              int32_t count,
                              double height,
                              double width,
                              double fov_estimate_deg,
                              double fov_max_error_deg,
                              int32_t pattern_checking_stars,
                              double match_radius,
                              double match_threshold,
                              double timeout_ms,
                              double distortion_min,
                              double distortion_max,
                              TetraSolution* out,
                              TetraMatch* matches,
                              int32_t match_capacity);
