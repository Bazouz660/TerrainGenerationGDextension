//==========================================
// mesh_generator.h - Handles mesh creation
//==========================================
#ifndef MESH_GENERATOR_H
#define MESH_GENERATOR_H

#include "terrain_config.h"
#include "height_sampler.h"
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <vector>

namespace godot {

// Forward declaration
struct RiverSegment;

class MeshGenerator {
private:
    const TerrainConfig* config;
    const HeightSampler* height_sampler;

public:
    MeshGenerator(const TerrainConfig* terrain_config, const HeightSampler* sampler);

    MeshInstance3D* generate_chunk_mesh(Vector2i position);
    MeshInstance3D* generate_chunk_mesh_with_rivers(Vector2i position, const std::vector<RiverSegment>& river_segments);

private:
    void generate_vertices(int extended_size, const PackedFloat32Array& height_data,
                          float step, class SurfaceTool* st) const;
    void generate_indices(class SurfaceTool* st) const;
};

}

#endif