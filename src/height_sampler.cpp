//==========================================
// height_sampler.cpp
//==========================================
#include "height_sampler.h"
#include "river_generator.h"
#include <godot_cpp/classes/noise.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

float HeightSampler::sample_height_with_rivers(float world_x, float world_z, const std::vector<RiverSegment>& river_segments) const {
    float base_height = sample_height(world_x, world_z);
    
    if (!config->enable_river_carving || river_segments.empty()) {
        return base_height;
    }
    
    float carving_effect = calculate_river_carving_effect(world_x, world_z, river_segments);
    return base_height - carving_effect;
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

    // Generate the terrain heights
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

void HeightSampler::precompute_height_data_with_rivers(Vector2i chunk_pos, float step, int extended_size, 
                                                      PackedFloat32Array& height_data, 
                                                      const std::vector<RiverSegment>& river_segments) const {
    float chunk_world_x = chunk_pos.x * config->width;
    float chunk_world_z = chunk_pos.y * config->width;

    if (config->continentalness_texture.is_null()) {
        for (int i = 0; i < extended_size * extended_size; ++i) {
            height_data[i] = 0.0f;
        }
        return;
    }

    // Generate the base terrain heights
    for (int z = -1; z <= config->segment_count + 1; ++z) {
        float world_z = chunk_world_z + z * step;
        int row_offset = (z + 1) * extended_size;

        for (int x = -1; x <= config->segment_count + 1; ++x) {
            float world_x = chunk_world_x + x * step;
            float base_height = sample_combined_noise(world_x, world_z);
            
            // Apply river carving if enabled
            float final_height = base_height;
            if (config->enable_river_carving && !river_segments.empty()) {
                float carving_effect = calculate_river_carving_effect(world_x, world_z, river_segments);
                final_height = base_height - carving_effect;
            }
            
            height_data[row_offset + (x + 1)] = final_height;
        }
    }
}

float HeightSampler::calculate_river_carving_effect(float world_x, float world_z, 
                                                   const std::vector<RiverSegment>& river_segments) const {
    float total_carving = 0.0f;
    
    for (const RiverSegment& segment : river_segments) {
        // Calculate distance from point to line segment
        Vector2 point(world_x, world_z);
        Vector2 line_start = segment.start;
        Vector2 line_end = segment.end;
        
        // Vector from start to end of segment
        Vector2 segment_vec = line_end - line_start;
        float segment_length_sq = segment_vec.length_squared();
        
        if (segment_length_sq < 0.0001f) {
            // Degenerate segment, treat as point
            float distance = point.distance_to(line_start);
            float carving = smooth_carving_falloff(distance, segment.width);
            
            // Calculate depth for this segment with progressive uphill compensation
            float river_depth = config->river_carving_depth;
            if (segment.uphill_amount > 0.0f) {
                // For point segments, apply maximum compensation since we can't interpolate
                float base_compensation = (segment.uphill_amount / config->height_scale) * config->river_uphill_carving_multiplier;
                float progressive_multiplier = 1.0f + base_compensation; // Full compensation for point sources
                river_depth *= progressive_multiplier;
                river_depth = std::min(river_depth, config->river_carving_depth * 5.0f);
            }
            
            carving *= river_depth;
            
            // Add to total (rivers can overlap and combine their effects)
            total_carving += carving;
            continue;
        }
        
        // Project point onto line segment
        Vector2 point_vec = point - line_start;
        float t = std::max(0.0f, std::min(1.0f, point_vec.dot(segment_vec) / segment_length_sq));
        
        // Find closest point on segment
        Vector2 closest_point = line_start + t * segment_vec;
        float distance = point.distance_to(closest_point);
        
        // Calculate carving effect based on distance and river properties
        float carving = smooth_carving_falloff(distance, segment.width);
        
        if (carving > 0.0f) {  // Only process if there's an effect
            // Base river depth
            float river_depth = config->river_carving_depth;
            
            // Interpolate depth based on height difference along the river
            if (segment.start_height != segment.end_height) {
                float height_at_point = segment.start_height + t * (segment.end_height - segment.start_height);
                // Deeper carving at lower elevations (rivers get deeper as they flow down)
                river_depth *= (1.0f + 0.5f * (segment.start_height - height_at_point) / config->height_scale);
                river_depth = std::max(0.1f, river_depth); // Minimum depth
            }
            
            // Progressive uphill carving compensation - carve more at higher elevations
            if (segment.uphill_amount > 0.0f) {
                // For uphill segments, apply stronger carving toward the end (higher elevation)
                // t=0 is start (lower), t=1 is end (higher)
                float uphill_progress = t; // How far along the uphill segment we are
                
                // Calculate base compensation
                float base_compensation = (segment.uphill_amount / config->height_scale) * config->river_uphill_carving_multiplier;
                
                // Apply progressive scaling - more carving at higher elevations
                // Use quadratic scaling to concentrate carving at the peak
                float progressive_multiplier = 1.0f + base_compensation * (0.3f + 0.7f * uphill_progress * uphill_progress);
                
                river_depth *= progressive_multiplier;
                // Cap the maximum compensation to prevent excessive carving
                river_depth = std::min(river_depth, config->river_carving_depth * 5.0f);
            }
            
            carving *= river_depth;
            
            // Add to total carving (additive blending for overlapping rivers)
            total_carving += carving;
        }
    }
    
    // Apply a soft maximum to prevent excessive carving from overlapping rivers
    // This uses a smooth saturation function instead of a hard max
    float max_allowed_carving = config->river_carving_depth * 2.0f; // Allow up to 2x the base depth
    return max_allowed_carving * (total_carving / (total_carving + max_allowed_carving));
}

float HeightSampler::smooth_carving_falloff(float distance, float river_width) const {
    // Calculate the effective carving radius
    float carving_radius = river_width * config->river_carving_width_multiplier;
    
    if (distance >= carving_radius) {
        return 0.0f; // No effect outside the carving radius
    }
    
    // Normalize distance to 0-1 range
    float normalized_distance = distance / carving_radius;
    
    // Create smooth falloff using a combination of cosine and exponential curves
    // This creates a natural-looking river bed shape
    float cosine_falloff = 0.5f * (1.0f + std::cos(normalized_distance * M_PI));
    
    // Apply smoothness parameter - higher values create smoother transitions
    float smoothness = config->river_carving_smoothness;
    float smooth_falloff = std::pow(cosine_falloff, smoothness);
    
    // Add a slight exponential decay for more natural looking banks
    float exp_factor = std::exp(-normalized_distance * 2.0f);
    
    // Blend the two falloff curves
    return smooth_falloff * (0.7f + 0.3f * exp_factor);
}