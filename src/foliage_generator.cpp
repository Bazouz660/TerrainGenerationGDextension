//==========================================
// foliage_generator.cpp
//==========================================
#include "foliage_generator.h"
#include "utils.h"

using namespace godot;

FoliageGenerator::FoliageGenerator(const TerrainConfig* terrain_config, const HeightSampler* sampler)
    : config(terrain_config), height_sampler(sampler) {
}

void FoliageGenerator::populate_chunk_foliage(MeshInstance3D* chunk_mesh, Vector2i position) {
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