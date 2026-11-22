// Tetra3 lost-in-space plate solver, in C++.
//
// Each step follows Tetra3.solve_from_centroids so a solution here is the
// solution the Python package returns for the same centroids.

#include "tetra.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr uint64_t kMagicRand = 2654435761ull;

using Vec3 = std::array<double, 3>;
using Mat3 = std::array<double, 9>;
using Yx = std::array<double, 2>;

struct Database {
    uint32_t star_count = 0;
    uint32_t rows = 0;
    uint16_t pattern_size = 4;
    uint16_t pattern_bins = 50;
    float pattern_max_error = 0.005f;
    float max_fov = 0;
    float min_fov = 0;
    uint16_t epoch_equinox = 2000;
    uint16_t verification_stars = 30;
    bool presorted = true;
    std::vector<float> stars;        // star_count x 6
    std::vector<uint16_t> ids;       // star_count
    std::vector<uint16_t> table;     // rows x 4
    std::vector<float> largest_edge; // rows, milliradians

    Vec3 vector(uint32_t star) const {
        const float* row = &stars[static_cast<size_t>(star) * 6];
        return {row[2], row[3], row[4]};
    }
};

template <typename T>
bool read_block(const uint8_t*& at, const uint8_t* end, std::vector<T>& out, size_t count) {
    const size_t bytes = count * sizeof(T);
    if (static_cast<size_t>(end - at) < bytes) return false;
    out.resize(count);
    std::memcpy(out.data(), at, bytes);
    at += bytes;
    return true;
}

Database* parse(const uint8_t* bytes, uint64_t length) {
    if (bytes == nullptr || length < 64 || std::memcmp(bytes, "TETRA001", 8) != 0) return nullptr;
    auto* db = new (std::nothrow) Database();
    if (db == nullptr) return nullptr;
    std::memcpy(&db->star_count, bytes + 8, 4);
    std::memcpy(&db->rows, bytes + 12, 4);
    std::memcpy(&db->pattern_size, bytes + 16, 2);
    std::memcpy(&db->pattern_bins, bytes + 18, 2);
    std::memcpy(&db->pattern_max_error, bytes + 20, 4);
    std::memcpy(&db->max_fov, bytes + 24, 4);
    std::memcpy(&db->min_fov, bytes + 28, 4);
    std::memcpy(&db->epoch_equinox, bytes + 32, 2);
    std::memcpy(&db->verification_stars, bytes + 34, 2);
    db->presorted = bytes[36] != 0;
    const uint8_t* at = bytes + 64;
    const uint8_t* end = bytes + length;
    const bool ok = db->pattern_size == 4 && db->rows > 0 && db->star_count > 0 &&
                    read_block(at, end, db->stars, static_cast<size_t>(db->star_count) * 6) &&
                    read_block(at, end, db->ids, db->star_count) &&
                    read_block(at, end, db->table, static_cast<size_t>(db->rows) * 4) &&
                    read_block(at, end, db->largest_edge, db->rows);
    if (!ok) {
        delete db;
        return nullptr;
    }
    return db;
}

double dot(const Vec3& a, const Vec3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

double norm(const Vec3& a) { return std::sqrt(dot(a, a)); }

double edge_angle(const Vec3& a, const Vec3& b) {
    const Vec3 d{a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return 2 * std::asin(std::min(1.0, 0.5 * norm(d)));
}

Vec3 mul(const Mat3& m, const Vec3& v) {
    return {m[0] * v[0] + m[1] * v[1] + m[2] * v[2],
            m[3] * v[0] + m[4] * v[1] + m[5] * v[2],
            m[6] * v[0] + m[7] * v[1] + m[8] * v[2]};
}

Vec3 mul_transposed(const Mat3& m, const Vec3& v) {
    return {m[0] * v[0] + m[3] * v[1] + m[6] * v[2],
            m[1] * v[0] + m[4] * v[1] + m[7] * v[2],
            m[2] * v[0] + m[5] * v[1] + m[8] * v[2]};
}

// _compute_vectors: i is the boresight, j points left, k points up.
Vec3 compute_vector(const Yx& c, double height, double width, double fov) {
    const double scale = std::tan(fov / 2) / width * 2;
    Vec3 v{1.0, (width / 2 - c[1]) * scale, (height / 2 - c[0]) * scale};
    const double n = norm(v);
    return {v[0] / n, v[1] / n, v[2] / n};
}

// _undistort_centroids: r_u = r_d (1 - k' r_d^2) / (1 - k), k at width/2.
Yx undistort(const Yx& c, double height, double width, double k) {
    const double y = c[0] - height / 2;
    const double x = c[1] - width / 2;
    const double r = std::sqrt(y * y + x * x) / width * 2;
    const double scale = (1 - k * r * r) / (1 - k);
    return {y * scale + height / 2, x * scale + width / 2};
}

// One-sided Jacobi SVD of a 3x3 matrix: a = U diag(s) Vt.
void svd3(const Mat3& a, Mat3& u, Vec3& s, Mat3& vt) {
    double col[3][3] = {{a[0], a[3], a[6]}, {a[1], a[4], a[7]}, {a[2], a[5], a[8]}};
    double v[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    for (int sweep = 0; sweep < 60; ++sweep) {
        double off = 0;
        for (int p = 0; p < 2; ++p) {
            for (int q = p + 1; q < 3; ++q) {
                double pp = 0, qq = 0, pq = 0;
                for (int i = 0; i < 3; ++i) {
                    pp += col[p][i] * col[p][i];
                    qq += col[q][i] * col[q][i];
                    pq += col[p][i] * col[q][i];
                }
                if (std::fabs(pq) <= 1e-300 || std::fabs(pq) < 1e-16 * std::sqrt(pp * qq)) continue;
                off = std::max(off, std::fabs(pq) / std::sqrt(pp * qq));
                const double tau = (qq - pp) / (2 * pq);
                const double t = (tau >= 0 ? 1.0 : -1.0) / (std::fabs(tau) + std::sqrt(1 + tau * tau));
                const double c = 1 / std::sqrt(1 + t * t);
                const double sn = t * c;
                for (int i = 0; i < 3; ++i) {
                    const double cp = col[p][i];
                    const double cq = col[q][i];
                    col[p][i] = c * cp - sn * cq;
                    col[q][i] = sn * cp + c * cq;
                    const double vp = v[p][i];
                    const double vq = v[q][i];
                    v[p][i] = c * vp - sn * vq;
                    v[q][i] = sn * vp + c * vq;
                }
            }
        }
        if (off < 1e-15) break;
    }
    Vec3 basis[3];
    for (int k = 0; k < 3; ++k) {
        const Vec3 c{col[k][0], col[k][1], col[k][2]};
        s[k] = norm(c);
        basis[k] = s[k] > 1e-300 ? Vec3{c[0] / s[k], c[1] / s[k], c[2] / s[k]} : Vec3{0, 0, 0};
    }
    // A vanishing singular value leaves its column free; complete the basis.
    const double top = std::max({s[0], s[1], s[2]});
    for (int k = 0; k < 3; ++k) {
        if (s[k] > top * 1e-12) continue;
        const Vec3& a1 = basis[(k + 1) % 3];
        const Vec3& a2 = basis[(k + 2) % 3];
        basis[k] = {a1[1] * a2[2] - a1[2] * a2[1], a1[2] * a2[0] - a1[0] * a2[2],
                    a1[0] * a2[1] - a1[1] * a2[0]};
    }
    for (int row = 0; row < 3; ++row) {
        for (int k = 0; k < 3; ++k) {
            u[row * 3 + k] = basis[k][row];
            vt[k * 3 + row] = v[k][row];
        }
    }
}

// _find_rotation_matrix: H = image^T catalog, R = U Vt, so image = R catalog.
Mat3 find_rotation(const std::vector<Vec3>& image, const std::vector<Vec3>& catalog) {
    Mat3 h{};
    for (size_t n = 0; n < image.size(); ++n) {
        for (int a = 0; a < 3; ++a) {
            for (int b = 0; b < 3; ++b) h[a * 3 + b] += image[n][a] * catalog[n][b];
        }
    }
    Mat3 u{}, vt{};
    Vec3 s{};
    svd3(h, u, s, vt);
    Mat3 r{};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            r[i * 3 + j] = u[i * 3] * vt[j] + u[i * 3 + 1] * vt[3 + j] + u[i * 3 + 2] * vt[6 + j];
        }
    }
    return r;
}

// scipy.stats.binom.cdf(k, n, p).
double binom_cdf(double k, int n, double p) {
    if (!(p >= 0 && p <= 1)) return std::numeric_limits<double>::quiet_NaN();
    const double top = std::floor(k);
    if (top < 0) return 0;
    if (top >= n) return 1;
    if (p == 0) return 1;
    if (p == 1) return 0;
    const double lp = std::log(p);
    const double lq = std::log1p(-p);
    double sum = 0;
    for (int i = 0; i <= static_cast<int>(top); ++i) {
        const double log_choose = std::lgamma(n + 1.0) - std::lgamma(i + 1.0) - std::lgamma(n - i + 1.0);
        sum += std::exp(log_choose + i * lp + (n - i) * lq);
    }
    return std::min(1.0, sum);
}

// _get_table_index_from_hash with quadratic probing.
void probe(const Database& db, uint64_t hash, std::vector<uint32_t>& found) {
    found.clear();
    const uint64_t rows = db.rows;
    for (uint64_t c = 0;; ++c) {
        const uint64_t i = (hash + c * c) % rows;
        const uint16_t* row = &db.table[i * 4];
        if (row[0] == 0 && row[1] == 0 && row[2] == 0 && row[3] == 0) return;
        found.push_back(static_cast<uint32_t>(i));
        if (c > rows) return;
    }
}

uint64_t key_to_index(const std::array<int, 5>& key, uint64_t bins, uint64_t rows) {
    uint64_t sum = 0;
    uint64_t power = 1;
    for (int i = 0; i < 5; ++i) {
        sum += static_cast<uint64_t>(key[i]) * power;
        power *= bins;
    }
    return (sum * kMagicRand) % rows;
}

// Five sorted edge ratios and the largest edge, from four unit vectors.
void edge_ratios(const Vec3* v, std::array<double, 5>& ratios, double& largest) {
    double edges[6];
    int e = 0;
    for (int a = 0; a < 4; ++a) {
        for (int b = a + 1; b < 4; ++b) edges[e++] = edge_angle(v[a], v[b]);
    }
    std::sort(edges, edges + 6);
    largest = edges[5];
    for (int i = 0; i < 5; ++i) ratios[i] = edges[i] / largest;
}

// _find_centroid_matches: pairs inside r, first by image star, then by catalog star.
std::vector<std::array<int, 2>> centroid_matches(const std::vector<Yx>& image,
                                                 const std::vector<Yx>& catalog,
                                                 double r) {
    std::vector<std::array<int, 2>> by_image;
    for (size_t i = 0; i < image.size(); ++i) {
        for (size_t j = 0; j < catalog.size(); ++j) {
            const double dy = image[i][0] - catalog[j][0];
            const double dx = image[i][1] - catalog[j][1];
            if (std::sqrt(dy * dy + dx * dx) < r) {
                by_image.push_back({static_cast<int>(i), static_cast<int>(j)});
                break;
            }
        }
    }
    std::vector<std::array<int, 2>> out;
    for (size_t j = 0; j < catalog.size(); ++j) {
        for (const auto& pair : by_image) {
            if (pair[1] == static_cast<int>(j)) {
                out.push_back(pair);
                break;
            }
        }
    }
    return out;
}

void sort_by_radius(Vec3* v, int count) {
    Vec3 mean{0, 0, 0};
    for (int i = 0; i < count; ++i) {
        for (int a = 0; a < 3; ++a) mean[a] += v[i][a] / count;
    }
    std::array<std::pair<double, int>, 4> order{};
    for (int i = 0; i < count; ++i) {
        const Vec3 d{v[i][0] - mean[0], v[i][1] - mean[1], v[i][2] - mean[2]};
        order[i] = {norm(d), i};
    }
    std::stable_sort(order.begin(), order.begin() + count,
                     [](const auto& a, const auto& b) { return a.first < b.first; });
    Vec3 copy[4];
    for (int i = 0; i < count; ++i) copy[i] = v[order[i].second];
    for (int i = 0; i < count; ++i) v[i] = copy[i];
}

double now_ms() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

double wrap360(double deg) {
    double out = std::fmod(deg, 360.0);
    if (out < 0) out += 360.0;
    return out;
}

}  // namespace

TETRA_API void* tetra_alloc(uint64_t bytes) { return std::calloc(1, bytes == 0 ? 1 : bytes); }

TETRA_API void tetra_free(void* pointer) { std::free(pointer); }

TETRA_API void* tetra_open_memory(const uint8_t* bytes, uint64_t length) {
    return parse(bytes, length);
}

TETRA_API void* tetra_open_file(const char* path) {
    if (path == nullptr) return nullptr;
    std::FILE* file = std::fopen(path, "rb");
    if (file == nullptr) return nullptr;
    std::vector<uint8_t> bytes;
    uint8_t chunk[1 << 16];
    size_t got = 0;
    while ((got = std::fread(chunk, 1, sizeof(chunk), file)) > 0) {
        bytes.insert(bytes.end(), chunk, chunk + got);
    }
    std::fclose(file);
    return parse(bytes.data(), bytes.size());
}

TETRA_API void tetra_close(void* db) { delete static_cast<Database*>(db); }

TETRA_API int32_t tetra_star_count(void* db) {
    return db == nullptr ? 0 : static_cast<int32_t>(static_cast<Database*>(db)->star_count);
}

TETRA_API double tetra_max_fov(void* db) {
    return db == nullptr ? 0 : static_cast<Database*>(db)->max_fov;
}

TETRA_API int32_t tetra_solve(void* handle,
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
                              TetraMatch* matches_out,
                              int32_t match_capacity) {
    if (handle == nullptr || out == nullptr || centroids_yx == nullptr || count < 4 ||
        height < 2 || width < 2 || fov_estimate_deg <= 0) {
        return -1;
    }
    const Database& db = *static_cast<Database*>(handle);
    std::memset(out, 0, sizeof(*out));
    const double t0 = now_ms();

    const int n = std::min<int>(count, db.verification_stars);
    std::vector<Yx> image(n);
    for (int i = 0; i < n; ++i) image[i] = {centroids_yx[i * 2], centroids_yx[i * 2 + 1]};

    const double fov_estimate = fov_estimate_deg * kPi / 180;
    const double fov_initial = fov_estimate;
    const double fov_max_error = fov_max_error_deg * kPi / 180;
    const double num_patterns = static_cast<double>(db.rows / 2);
    const double threshold = match_threshold / num_patterns;
    const int bins = db.pattern_bins;
    const double max_err = db.pattern_max_error;

    const bool known = distortion_min == distortion_max;
    std::vector<double> range;
    std::vector<std::vector<Yx>> preundist;
    if (known) {
        for (auto& c : image) c = undistort(c, height, width, distortion_min);
    } else {
        const double lo = std::min(distortion_min, distortion_max);
        const double hi = std::max(distortion_min, distortion_max);
        const double span = std::round((hi - lo) * 1e6) / 1e6;
        const int steps = static_cast<int>(std::ceil(span * 10)) + 1;
        for (int i = 0; i < steps; ++i) range.push_back(steps == 1 ? lo : lo + (hi - lo) * i / (steps - 1));
        for (double k : range) {
            std::vector<Yx> shifted(n);
            for (int i = 0; i < n; ++i) shifted[i] = undistort(image[i], height, width, k);
            preundist.push_back(std::move(shifted));
        }
    }

    const int pool = std::min<int>(n, pattern_checking_stars);
    std::vector<uint32_t> found;
    std::vector<std::array<int, 5>> codes;
    bool timed_out = false;

    for (int a = 0; a < pool && !timed_out; ++a)
    for (int b = a + 1; b < pool && !timed_out; ++b)
    for (int c = b + 1; c < pool && !timed_out; ++c)
    for (int d = c + 1; d < pool; ++d) {
        if (timeout_ms > 0 && now_ms() - t0 > timeout_ms) {
            timed_out = true;
            break;
        }
        const int pick[4] = {a, b, c, d};
        std::array<double, 5> ratio_min{}, ratio_max{};
        double image_largest = 0;
        std::vector<std::array<double, 5>> ratio_pre;
        if (known) {
            Vec3 v[4];
            for (int i = 0; i < 4; ++i) v[i] = compute_vector(image[pick[i]], height, width, fov_initial);
            std::array<double, 5> ratios{};
            edge_ratios(v, ratios, image_largest);
            for (int i = 0; i < 5; ++i) {
                ratio_min[i] = ratios[i] - max_err;
                ratio_max[i] = ratios[i] + max_err;
            }
        } else {
            ratio_min.fill(std::numeric_limits<double>::infinity());
            ratio_max.fill(-std::numeric_limits<double>::infinity());
            for (size_t r = 0; r < range.size(); ++r) {
                Vec3 v[4];
                for (int i = 0; i < 4; ++i) {
                    v[i] = compute_vector(preundist[r][pick[i]], height, width, fov_initial);
                }
                std::array<double, 5> ratios{};
                edge_ratios(v, ratios, image_largest);
                ratio_pre.push_back(ratios);
                for (int i = 0; i < 5; ++i) {
                    ratio_min[i] = std::min(ratio_min[i], ratios[i]);
                    ratio_max[i] = std::max(ratio_max[i], ratios[i]);
                }
            }
        }

        int low[5], high[5];
        for (int i = 0; i < 5; ++i) {
            low[i] = static_cast<int>(std::max(0.0, ratio_min[i] * bins));
            high[i] = static_cast<int>(std::min(static_cast<double>(bins), ratio_max[i] * bins));
        }
        codes.clear();
        std::array<int, 5> code{};
        for (code[0] = low[0]; code[0] <= high[0]; ++code[0])
        for (code[1] = low[1]; code[1] <= high[1]; ++code[1])
        for (code[2] = low[2]; code[2] <= high[2]; ++code[2])
        for (code[3] = low[3]; code[3] <= high[3]; ++code[3])
        for (code[4] = low[4]; code[4] <= high[4]; ++code[4]) {
            std::array<int, 5> sorted = code;
            std::sort(sorted.begin(), sorted.end());
            codes.push_back(sorted);
        }
        std::sort(codes.begin(), codes.end());
        codes.erase(std::unique(codes.begin(), codes.end()), codes.end());

        for (const auto& key : codes) {
            probe(db, key_to_index(key, bins, db.rows), found);
            for (uint32_t row : found) {
                const double fov2 = db.largest_edge[row] / image_largest * fov_initial / 1000;
                if (!(std::fabs(fov2 - fov_estimate) < fov_max_error)) continue;

                Vec3 catalog_vectors[4];
                for (int i = 0; i < 4; ++i) catalog_vectors[i] = db.vector(db.table[row * 4 + i]);
                std::array<double, 5> catalog_ratios{};
                double catalog_largest = 0;
                edge_ratios(catalog_vectors, catalog_ratios, catalog_largest);
                bool valid = true;
                for (int i = 0; i < 5 && valid; ++i) {
                    valid = ratio_min[i] < catalog_ratios[i] && ratio_max[i] > catalog_ratios[i];
                }
                if (!valid) continue;

                std::vector<Yx> undist;
                const std::vector<Yx>* image_undist = &image;
                if (!known) {
                    size_t lo_i = 0, hi_i = 1;
                    std::vector<std::array<double, 5>> errors(range.size());
                    for (size_t r = 0; r < range.size(); ++r) {
                        for (int i = 0; i < 5; ++i) errors[r][i] = catalog_ratios[i] - ratio_pre[r][i];
                    }
                    if (range.size() > 2) {
                        std::vector<double> rms(range.size(), 0);
                        for (size_t r = 0; r < range.size(); ++r) {
                            for (int i = 0; i < 5; ++i) rms[r] += errors[r][i] * errors[r][i];
                        }
                        const size_t closest = std::min_element(rms.begin(), rms.end()) - rms.begin();
                        if (closest == 0) {
                            lo_i = 0;
                            hi_i = 1;
                        } else if (closest == range.size() - 1) {
                            lo_i = range.size() - 2;
                            hi_i = range.size() - 1;
                        } else if (rms[closest + 1] > rms[closest - 1]) {
                            lo_i = closest;
                            hi_i = closest + 1;
                        } else {
                            lo_i = closest - 1;
                            hi_i = closest;
                        }
                    }
                    if (range.size() < 2) {
                        lo_i = hi_i = 0;
                    }
                    double x = 0;
                    for (int i = 0; i < 5; ++i) {
                        x += errors[lo_i][i] / (errors[lo_i][i] - errors[hi_i][i]);
                    }
                    x /= 5;
                    const double k_est = range.size() < 2
                                             ? range[0]
                                             : range[lo_i] + x * (range[hi_i] - range[lo_i]);
                    if (!std::isfinite(k_est)) continue;
                    undist.resize(n);
                    for (int i = 0; i < n; ++i) undist[i] = undistort(image[i], height, width, k_est);
                    image_undist = &undist;
                }

                double fov = 0;
                if (known) {
                    fov = catalog_largest / image_largest * fov_initial;
                } else {
                    double longest = 0;
                    for (int i = 0; i < 4; ++i) {
                        for (int j = i + 1; j < 4; ++j) {
                            const Yx& p = (*image_undist)[pick[i]];
                            const Yx& q = (*image_undist)[pick[j]];
                            longest = std::max(longest, std::hypot(p[0] - q[0], p[1] - q[1]));
                        }
                    }
                    const double f = longest / 2 / std::tan(catalog_largest / 2);
                    fov = 2 * std::atan(width / 2 / f);
                }
                if (std::fabs(fov - fov_estimate) > fov_max_error) continue;

                Vec3 image_vectors[4];
                for (int i = 0; i < 4; ++i) {
                    image_vectors[i] = compute_vector((*image_undist)[pick[i]], height, width, fov);
                }
                sort_by_radius(image_vectors, 4);
                if (!db.presorted) sort_by_radius(catalog_vectors, 4);
                Mat3 rotation = find_rotation(
                    std::vector<Vec3>(image_vectors, image_vectors + 4),
                    std::vector<Vec3>(catalog_vectors, catalog_vectors + 4));

                // _get_nearby_stars around the boresight, diagonal half field.
                const Vec3 center{rotation[0], rotation[1], rotation[2]};
                const double fov_diagonal = fov * std::sqrt(width * width + height * height) / width;
                const double radius = fov_diagonal / 2;
                const double max_dist = 2 * std::sin(radius / 2);
                const double min_dot = std::cos(radius);
                std::vector<uint32_t> nearby;
                std::vector<Yx> nearby_centroids;
                std::vector<Vec3> nearby_vectors;
                const double scale = -width / 2 / std::tan(fov / 2);
                for (uint32_t s = 0; s < db.star_count; ++s) {
                    const Vec3 v = db.vector(s);
                    if (!(v[0] > center[0] - max_dist && v[0] < center[0] + max_dist)) continue;
                    if (!(v[1] > center[1] - max_dist && v[1] < center[1] + max_dist)) continue;
                    if (!(v[2] > center[2] - max_dist && v[2] < center[2] + max_dist)) continue;
                    if (!(dot(center, v) > min_dot)) continue;
                    const Vec3 derot = mul(rotation, v);
                    const double y = scale * derot[2] / derot[0] + height / 2;
                    const double x = scale * derot[1] / derot[0] + width / 2;
                    if (!(y > 0 && x > 0 && y < height && x < width)) continue;
                    if (static_cast<int>(nearby.size()) >= n) break;
                    nearby.push_back(s);
                    nearby_centroids.push_back({y, x});
                    nearby_vectors.push_back(v);
                }

                const auto matched = centroid_matches(*image_undist, nearby_centroids, width * match_radius);
                const int num_matches = static_cast<int>(matched.size());
                const double single = nearby_centroids.size() * match_radius * match_radius;
                const double prob = binom_cdf(n - (num_matches - 2), n, 1 - single);
                if (!(prob < threshold)) continue;

                // Accepted. Refit the rotation on every matched star.
                std::vector<Yx> matched_centroids;
                std::vector<Vec3> matched_image, matched_catalog;
                for (const auto& pair : matched) {
                    matched_centroids.push_back(image[pair[0]]);
                    matched_image.push_back(compute_vector(image[pair[0]], height, width, fov));
                    matched_catalog.push_back(nearby_vectors[pair[1]]);
                }
                rotation = find_rotation(matched_image, matched_catalog);
                const double ra = wrap360(std::atan2(rotation[1], rotation[0]) * 180 / kPi);
                const double dec = std::atan2(rotation[2], std::hypot(rotation[5], rotation[8])) * 180 / kPi;
                const double roll = wrap360(std::atan2(rotation[5], rotation[8]) * 180 / kPi);

                // Focal length and distortion from the angle off the boresight.
                double s_tt = 0, s_tr = 0, s_rr = 0, s_tb = 0, s_rb = 0;
                for (size_t m = 0; m < matched_centroids.size(); ++m) {
                    const Vec3 derot = mul(rotation, matched_catalog[m]);
                    const double tangent = std::hypot(derot[1], derot[2]) / derot[0];
                    const double dy = matched_centroids[m][0] - height / 2;
                    const double dx = matched_centroids[m][1] - width / 2;
                    const double r = std::sqrt(dy * dy + dx * dx) / width * 2;
                    const double r3 = r * r * r;
                    s_tt += tangent * tangent;
                    s_tr += tangent * r3;
                    s_rr += r3 * r3;
                    s_tb += tangent * r;
                    s_rb += r3 * r;
                }
                const double det = s_tt * s_rr - s_tr * s_tr;
                double f = 0, k = 0;
                if (std::fabs(det) > 1e-300) {
                    f = (s_tb * s_rr - s_tr * s_rb) / det;
                    k = (s_tt * s_rb - s_tr * s_tb) / det;
                } else if (s_tt > 0) {
                    f = s_tb / s_tt;
                }
                f = f / (1 - k);
                fov = 2 * std::atan(1 / f);

                double sum_sq = 0;
                for (size_t m = 0; m < matched_centroids.size(); ++m) {
                    const Yx corrected = undistort(matched_centroids[m], height, width, k);
                    const Vec3 sky = mul_transposed(rotation, compute_vector(corrected, height, width, fov));
                    const double angle = edge_angle(sky, matched_catalog[m]);
                    sum_sq += angle * angle;
                }
                const double residual =
                    matched.empty() ? 0 : std::sqrt(sum_sq / matched.size()) * 180 / kPi * 3600;

                out->solved = 1;
                out->timed_out = 0;
                out->matches = num_matches;
                out->epoch_equinox = db.epoch_equinox;
                out->ra_deg = ra;
                out->dec_deg = dec;
                out->roll_deg = roll;
                out->fov_deg = fov * 180 / kPi;
                out->distortion = k;
                out->rmse_arcsec = residual;
                out->prob = prob * num_patterns;
                out->t_solve_ms = now_ms() - t0;
                std::memcpy(out->rotation, rotation.data(), sizeof(out->rotation));
                if (matches_out != nullptr) {
                    const int keep = std::min<int>(match_capacity, num_matches);
                    for (int m = 0; m < keep; ++m) {
                        const uint32_t star = nearby[matched[m][1]];
                        const float* row = &db.stars[static_cast<size_t>(star) * 6];
                        matches_out[m].image_index = matched[m][0];
                        matches_out[m].cat_id = db.ids[star];
                        matches_out[m].ra_deg = row[0] * 180 / kPi;
                        matches_out[m].dec_deg = row[1] * 180 / kPi;
                        matches_out[m].mag = row[5];
                    }
                }
                return 1;
            }
        }
    }
    out->solved = 0;
    out->timed_out = timed_out ? 1 : 0;
    out->t_solve_ms = now_ms() - t0;
    return 0;
}
