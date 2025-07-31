//==========================================
// height_sampler.h - Handles all noise sampling
//==========================================
#ifndef HEIGHT_SAMPLER_H
#define HEIGHT_SAMPLER_H

#include "terrain_config.h"
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <vector>

namespace godot {

// Forward declarations
struct RiverSegment;

class HeightSampler {
private:
    const TerrainConfig* config;

public:
    HeightSampler(const TerrainConfig* terrain_config);

    float sample_height(float world_x, float world_z) const;
    float sample_height_with_rivers(float world_x, float world_z, const std::vector<RiverSegment>& river_segments) const;
    Vector3 sample_normal(float world_x, float world_z) const;
    void precompute_height_data(Vector2i chunk_pos, float step, int extended_size, PackedFloat32Array& height_data) const;
    
    // River carving methods
    void precompute_height_data_with_rivers(Vector2i chunk_pos, float step, int extended_size, 
                                           PackedFloat32Array& height_data, 
                                           const std::vector<RiverSegment>& river_segments) const;

private:
    float sample_combined_noise(float world_x, float world_z) const;
    float calculate_river_carving_effect(float world_x, float world_z, 
                                       const std::vector<RiverSegment>& river_segments) const;
    float smooth_carving_falloff(float distance, float river_width) const;
};

}

#endif