//==========================================
// mesh_generator.cpp
//==========================================
#include "mesh_generator.h"
#include "river_generator.h"
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/array_mesh.hpp>

using namespace godot;

MeshGenerator::MeshGenerator(const TerrainConfig* terrain_config, const HeightSampler* sampler)
    : config(terrain_config), height_sampler(sampler) {
}

MeshInstance3D* MeshGenerator::generate_chunk_mesh(Vector2i position) {
    auto st = memnew(SurfaceTool);
    st->begin(Mesh::PRIMITIVE_TRIANGLES);

    float step = config->width / (float)config->segment_count;
    int extended_size = config->segment_count + 3;

    PackedFloat32Array height_data;
    height_data.resize(extended_size * extended_size);
    height_sampler->precompute_height_data(position, step, extended_size, height_data);

    generate_vertices(extended_size, height_data, step, st);
    generate_indices(st);

    auto mesh = st->commit();
    memdelete(st);

    MeshInstance3D *mesh_instance = memnew(MeshInstance3D);
    mesh_instance->set_mesh(mesh);

    float chunk_world_x = position.x * config->width;
    float chunk_world_z = position.y * config->width;
    mesh_instance->set_position(Vector3(chunk_world_x, 0, chunk_world_z));

    if (!config->terrain_material.is_null()) {
        mesh_instance->set_material_override(config->terrain_material);
    }

    return mesh_instance;
}

MeshInstance3D* MeshGenerator::generate_chunk_mesh_with_rivers(Vector2i position, const std::vector<RiverSegment>& river_segments) {
    auto st = memnew(SurfaceTool);
    st->begin(Mesh::PRIMITIVE_TRIANGLES);

    float step = config->width / (float)config->segment_count;
    int extended_size = config->segment_count + 3;

    PackedFloat32Array height_data;
    height_data.resize(extended_size * extended_size);
    
    // Use river-aware height sampling
    height_sampler->precompute_height_data_with_rivers(position, step, extended_size, height_data, river_segments);

    generate_vertices(extended_size, height_data, step, st);
    generate_indices(st);

    auto mesh = st->commit();
    memdelete(st);

    MeshInstance3D *mesh_instance = memnew(MeshInstance3D);
    mesh_instance->set_mesh(mesh);

    float chunk_world_x = position.x * config->width;
    float chunk_world_z = position.y * config->width;
    mesh_instance->set_position(Vector3(chunk_world_x, 0, chunk_world_z));

    if (!config->terrain_material.is_null()) {
        mesh_instance->set_material_override(config->terrain_material);
    }

    return mesh_instance;
}

void MeshGenerator::generate_vertices(int extended_size, const PackedFloat32Array& height_data, float step, SurfaceTool* st) const {
    float inv_segment_count = 1.0f / (float)config->segment_count;
    float width_inv_segment = config->width * inv_segment_count;
    float double_step = 2.0f * step;

    for (int z = 0; z <= config->segment_count; ++z) {
        float local_z = z * width_inv_segment;
        float uv_z = z * inv_segment_count;
        int base_idx = (z + 1) * extended_size;

        for (int x = 0; x <= config->segment_count; ++x) {
            float local_x = x * width_inv_segment;
            float uv_x = x * inv_segment_count;

            float height = height_data[base_idx + (x + 1)];

            float height_left = height_data[base_idx + x];
            float height_right = height_data[base_idx + (x + 2)];
            float height_up = height_data[base_idx - extended_size + (x + 1)];
            float height_down = height_data[base_idx + extended_size + (x + 1)];

            Vector3 tangent_x = Vector3(double_step, height_right - height_left, 0.0f);
            Vector3 tangent_z = Vector3(0.0f, height_down - height_up, double_step);
            Vector3 normal = tangent_z.cross(tangent_x).normalized();

            st->set_uv(Vector2(uv_x, uv_z));
            st->set_normal(normal);
            st->set_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
            st->add_vertex(Vector3(local_x, height, local_z));
        }
    }
}

void MeshGenerator::generate_indices(SurfaceTool* st) const {
    int vertices_per_row = config->segment_count + 1;

    for (int z = 0; z < config->segment_count; ++z) {
        int current_row = z * vertices_per_row;
        int next_row = current_row + vertices_per_row;

        for (int x = 0; x < config->segment_count; ++x) {
            int i0 = current_row + x;
            int i1 = i0 + 1;
            int i2 = next_row + x;
            int i3 = i2 + 1;

            st->add_index(i0);
            st->add_index(i1);
            st->add_index(i2);

            st->add_index(i1);
            st->add_index(i3);
            st->add_index(i2);
        }
    }
}
