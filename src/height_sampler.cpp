//==========================================
// height_sampler.cpp
//==========================================
#include "height_sampler.h"
#include <godot_cpp/classes/noise.hpp>

using namespace godot;

HeightSampler::HeightSampler(const TerrainConfig* terrain_config)
    : config(terrain_config) {
}

float HeightSampler::sample_height(float world_x, float world_z) const {
    if (config->continentalness_texture.is_null() ||
        config->peaks_and_valleys_texture.is_null() ||
        config->erosion_texture.is_null()) {
        return 0.0f;
    }

    return sample_combined_noise(world_x, world_z);
}

Vector3 HeightSampler::sample_normal(float world_x, float world_z) const {
    float step = config->width / (float)config->segment_count;
    float height = sample_height(world_x, world_z);
    float height_left = sample_height(world_x - step, world_z);
    float height_right = sample_height(world_x + step, world_z);
    float height_up = sample_height(world_x, world_z - step);
    float height_down = sample_height(world_x, world_z + step);

    Vector3 tangent_x = Vector3(step, height_right - height_left, 0.0f);
    Vector3 tangent_z = Vector3(0.0f, height_down - height_up, step);

    return tangent_z.cross(tangent_x).normalized();
}

void HeightSampler::precompute_height_data(Vector2i chunk_pos, float step, int extended_size, PackedFloat32Array& height_data) const {
    float chunk_world_x = chunk_pos.x * config->width;
    float chunk_world_z = chunk_pos.y * config->width;

    if (config->continentalness_texture.is_null()) {
        for (int i = 0; i < extended_size * extended_size; ++i) {
            height_data[i] = 0.0f;
        }
        return;
    }

    for (int z = -1; z <= config->segment_count + 1; ++z) {
        float world_z = chunk_world_z + z * step;
        int row_offset = (z + 1) * extended_size;

        for (int x = -1; x <= config->segment_count + 1; ++x) {
            float world_x = chunk_world_x + x * step;
            float height = sample_combined_noise(world_x, world_z);
            height_data[row_offset + (x + 1)] = height;
        }
    }
}

float HeightSampler::sample_combined_noise(float world_x, float world_z) const {
    float continentalness = config->continentalness_texture->get_noise()->get_noise_2d(world_x, world_z);
    float peaks_and_valleys = config->peaks_and_valleys_texture->get_noise()->get_noise_2d(world_x, world_z);
    float erosion = config->erosion_texture->get_noise()->get_noise_2d(world_x, world_z);

    // Normalize to 0-1 range
    continentalness = (continentalness + 1.0f) * 0.5f;
    peaks_and_valleys = (peaks_and_valleys + 1.0f) * 0.5f;
    erosion = (erosion + 1.0f) * 0.5f;

    // Apply curves if available
    if (!config->continentalness_curve.is_null()) {
        continentalness = config->continentalness_curve->sample(continentalness);
        peaks_and_valleys = config->peaks_and_valleys_curve->sample(peaks_and_valleys);
        erosion = config->erosion_curve->sample(erosion);
    }

    return (continentalness + peaks_and_valleys + erosion) * config->height_scale;
}