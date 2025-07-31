//==========================================
// foliage_generator.h - Handles foliage placement
//==========================================
#ifndef FOLIAGE_GENERATOR_H
#define FOLIAGE_GENERATOR_H

#include "terrain_config.h"
#include "height_sampler.h"
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <vector>

namespace godot {

// Forward declaration
struct RiverSegment;

class FoliageGenerator {
private:
    const TerrainConfig* config;
    const HeightSampler* height_sampler;

public:
    FoliageGenerator(const TerrainConfig* terrain_config, const HeightSampler* sampler);

    void populate_chunk_foliage(MeshInstance3D* chunk_mesh, Vector2i position);
    void populate_chunk_foliage_with_rivers(MeshInstance3D* chunk_mesh, Vector2i position, 
                                           const std::vector<RiverSegment>& river_segments);

private:
    bool is_suitable_for_foliage(float height, const Vector3& normal) const;
    bool is_near_river(float world_x, float world_z, const std::vector<RiverSegment>& river_segments) const;
    float distance_to_river_segment(float world_x, float world_z, const RiverSegment& segment) const;
};

}

#endif