//==========================================
// foliage_generator.h - Handles foliage placement
//==========================================
#ifndef FOLIAGE_GENERATOR_H
#define FOLIAGE_GENERATOR_H

#include "terrain_config.h"
#include "height_sampler.h"
#include <godot_cpp/classes/mesh_instance3d.hpp>

namespace godot {

class FoliageGenerator {
private:
    const TerrainConfig* config;
    const HeightSampler* height_sampler;

public:
    FoliageGenerator(const TerrainConfig* terrain_config, const HeightSampler* sampler);

    void populate_chunk_foliage(MeshInstance3D* chunk_mesh, Vector2i position);

private:
    bool is_suitable_for_foliage(float height, const Vector3& normal) const;
};

}

#endif