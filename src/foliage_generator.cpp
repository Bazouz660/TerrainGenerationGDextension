//==========================================
// foliage_generator.cpp
//==========================================
#include "foliage_generator.h"
#include "river_generator.h"
#include "utils.h"
#include <cmath>

using namespace godot;

FoliageGenerator::FoliageGenerator(const TerrainConfig* terrain_config, const HeightSampler* sampler)
    : config(terrain_config), height_sampler(sampler) {
}

void FoliageGenerator::populate_chunk_foliage(MeshInstance3D* chunk_mesh, Vector2i position) {
    // Call the river-aware version with an empty river list
    std::vector<RiverSegment> empty_rivers;
    populate_chunk_foliage_with_rivers(chunk_mesh, position, empty_rivers);
}

void FoliageGenerator::populate_chunk_foliage_with_rivers(MeshInstance3D* chunk_mesh, Vector2i position, 
                                                         const std::vector<RiverSegment>& river_segments) {
    if (!chunk_mesh || !config->foliage_scene.is_valid()) {
        return;
    }

    std::vector<Vector2> foliage_positions = poisson_disc_sample(config->width, config->width, 10.0f, 1.0f);

    for (const Vector2& pos : foliage_positions) {
        if (config->foliage_scene->can_instantiate()) {
            float random_shift_x = random_float(position * config->width + pos * 0.5f) * 5.0f;
            float random_shift_y = random_float(position * config->width + pos) * 5.0f;

            Vector2 shifted_pos = Vector2(pos.x + random_shift_x, pos.y + random_shift_y);

            float world_x = position.x * config->width + shifted_pos.x;
            float world_z = position.y * config->width + shifted_pos.y;

            // Check if this position is near a river
            if (is_near_river(world_x, world_z, river_segments)) {
                continue; // Skip foliage placement near rivers
            }

            float height = height_sampler->sample_height(world_x, world_z);
            Vector3 normal = height_sampler->sample_normal(world_x, world_z);

            if (!is_suitable_for_foliage(height, normal)) {
                continue;
            }

            float random_value = random_float(shifted_pos);
            float rotation = random_value * Math_PI * 2.0f;

            Node3D* foliage_instance = static_cast<Node3D*>(config->foliage_scene->instantiate());
            foliage_instance->set_position(Vector3(shifted_pos.x, height, shifted_pos.y));
            foliage_instance->set_rotation(Vector3(0, rotation, 0));
            chunk_mesh->add_child(foliage_instance);
        }
    }
}

bool FoliageGenerator::is_suitable_for_foliage(float height, const Vector3& normal) const {
    const float min_height = -12.0f;
    const float min_normal_y = 0.7f;

    return height >= min_height && normal.y >= min_normal_y;
}

bool FoliageGenerator::is_near_river(float world_x, float world_z, const std::vector<RiverSegment>& river_segments) const {
    const float exclusion_radius = config->foliage_river_exclusion_radius;

    for (const RiverSegment& segment : river_segments) {
        float distance = distance_to_river_segment(world_x, world_z, segment);
        
        // Calculate effective exclusion radius based on river width
        float effective_radius = exclusion_radius + segment.width * 0.5f;
        
        if (distance <= effective_radius) {
            return true; // Too close to river
        }
    }

    return false; // Not near any river
}

float FoliageGenerator::distance_to_river_segment(float world_x, float world_z, const RiverSegment& segment) const {
    Vector2 point(world_x, world_z);
    Vector2 line_start = segment.start;
    Vector2 line_end = segment.end;
    
    // Vector from start to end of segment
    Vector2 segment_vec = line_end - line_start;
    float segment_length_sq = segment_vec.length_squared();
    
    if (segment_length_sq < 0.0001f) {
        // Degenerate segment, treat as point
        return point.distance_to(line_start);
    }
    
    // Project point onto line segment
    Vector2 point_vec = point - line_start;
    float t = std::max(0.0f, std::min(1.0f, point_vec.dot(segment_vec) / segment_length_sq));
    
    // Find closest point on segment
    Vector2 closest_point = line_start + t * segment_vec;
    return point.distance_to(closest_point);
}