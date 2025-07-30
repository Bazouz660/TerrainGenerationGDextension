//==========================================
// height_sampler.h - Handles all noise sampling
//==========================================
#ifndef HEIGHT_SAMPLER_H
#define HEIGHT_SAMPLER_H

#include "terrain_config.h"
#include <godot_cpp/variant/packed_float32_array.hpp>

namespace godot {

class HeightSampler {
private:
    const TerrainConfig* config;

public:
    HeightSampler(const TerrainConfig* terrain_config);

    float sample_height(float world_x, float world_z) const;
    Vector3 sample_normal(float world_x, float world_z) const;
    void precompute_height_data(Vector2i chunk_pos, float step, int extended_size, PackedFloat32Array& height_data) const;

private:
    float sample_combined_noise(float world_x, float world_z) const;
};

}

#endif