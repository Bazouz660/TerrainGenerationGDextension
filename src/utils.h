#ifndef TERRAIN_GENERATOR_UTILS_H
#define TERRAIN_GENERATOR_UTILS_H


#include <vector>
#include <random>
#include <cstdint>
#include <cmath>

#include <godot_cpp/variant/vector2.hpp>

namespace godot {


// Utility function: Poisson disc sampling for 2D point generation
// Returns a vector of Vector2 points distributed with minimum distance 'radius' within a rectangle (width x height)
// Parameters:
//   width  - Width of the sampling area (X axis)
//   height - Height of the sampling area (Y axis)
//   radius - Minimum allowed distance between points
//   k      - Number of attempts per active point (higher = denser fill, default 30)
//   seed   - Random seed for reproducible results (default 0)
inline std::vector<Vector2> poisson_disc_sample(float width, float height, float radius, int k = 30, uint32_t seed = 0) {
    // Based on Bridson's algorithm
    float cell_size = radius / std::sqrt(2.0f);
    int grid_width = (int)std::ceil(width / cell_size);
    int grid_height = (int)std::ceil(height / cell_size);
    std::vector<std::vector<int>> grid(grid_width * grid_height);
    std::vector<Vector2> points;
    std::vector<Vector2> active;

    auto grid_index = [&](const Vector2& p) {
        int gx = (int)(p.x / cell_size);
        int gy = (int)(p.y / cell_size);
        return gy * grid_width + gx;
    };

    // Random generator
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> dist_x(0, width);
    std::uniform_real_distribution<float> dist_y(0, height);
    std::uniform_real_distribution<float> dist_angle(0, 2 * 3.1415926535f);
    std::uniform_real_distribution<float> dist_radius(radius, 2 * radius);

    // Initial point
    Vector2 first(dist_x(gen), dist_y(gen));
    points.push_back(first);
    active.push_back(first);
    grid[grid_index(first)].push_back(0);

    while (!active.empty()) {
        std::uniform_int_distribution<int> dist_active(0, (int)active.size() - 1);
        int idx = dist_active(gen);
        Vector2 center = active[idx];
        bool found = false;
        for (int i = 0; i < k; ++i) {
            float angle = dist_angle(gen);
            float r = dist_radius(gen);
            Vector2 candidate(center.x + r * std::cos(angle), center.y + r * std::sin(angle));
            if (candidate.x < 0 || candidate.x >= width || candidate.y < 0 || candidate.y >= height)
                continue;
            int cgx = (int)(candidate.x / cell_size);
            int cgy = (int)(candidate.y / cell_size);
            bool ok = true;
            for (int gx = std::max(0, cgx - 2); gx <= std::min(grid_width - 1, cgx + 2); ++gx) {
                for (int gy = std::max(0, cgy - 2); gy <= std::min(grid_height - 1, cgy + 2); ++gy) {
                    int gi = gy * grid_width + gx;
                    for (int pi : grid[gi]) {
                        if (candidate.distance_to(points[pi]) < radius) {
                            ok = false;
                            break;
                        }
                    }
                    if (!ok) break;
                }
                if (!ok) break;
            }
            if (ok) {
                points.push_back(candidate);
                active.push_back(candidate);
                grid[grid_index(candidate)].push_back((int)points.size() - 1);
                found = true;
                break;
            }
        }
        if (!found) {
            active.erase(active.begin() + idx);
        }
    }
    return points;
}

// Utility function: Generate a random float in the range [0.0, 1.0) using a seed and position
inline float random_float(const Vector2& pos, uint32_t seed = 0) {
    // Convert floats to fixed-point for consistent hashing
    uint32_t x = static_cast<uint32_t>(std::round(pos.x * 1000.0f));
    uint32_t y = static_cast<uint32_t>(std::round(pos.y * 1000.0f));

    // Combine seed and coordinates with good mixing
    uint32_t hash = seed;
    hash ^= x + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= y + 0x9e3779b9 + (hash << 6) + (hash >> 2);

    // Wang hash for excellent avalanche properties
    hash = (hash ^ 61) ^ (hash >> 16);
    hash *= 9;
    hash = hash ^ (hash >> 4);
    hash *= 0x27d4eb2d;
    hash = hash ^ (hash >> 15);

    // Convert to float [0.0, 1.0)
    return static_cast<float>(hash) / static_cast<float>(UINT32_MAX);
}

}

#endif // TERRAIN_GENERATOR_UTILS_H