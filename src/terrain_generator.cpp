//==========================================
// terrain_generator.cpp - Complete implementation
//==========================================
#include "terrain_generator.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void TerrainGenerator::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_width", "_width"), &TerrainGenerator::set_width);
    ClassDB::bind_method(D_METHOD("get_width"), &TerrainGenerator::get_width);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "width", PROPERTY_HINT_RANGE, "4,32, 1"), "set_width", "get_width");

    ClassDB::bind_method(D_METHOD("set_segment_count", "_segment_count"), &TerrainGenerator::set_segment_count);
    ClassDB::bind_method(D_METHOD("get_segment_count"), &TerrainGenerator::get_segment_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "segment_count", PROPERTY_HINT_RANGE, "1,64, 1"), "set_segment_count", "get_segment_count");

    ClassDB::bind_method(D_METHOD("set_origin_node_path", "_origin_node_path"), &TerrainGenerator::set_origin_node_path);
    ClassDB::bind_method(D_METHOD("get_origin_node_path"), &TerrainGenerator::get_origin_node_path);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "origin_node_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node3D"), "set_origin_node_path", "get_origin_node_path");

    ClassDB::bind_method(D_METHOD("set_continentalness_texture", "_continentalness_texture"), &TerrainGenerator::set_continentalness_texture);
    ClassDB::bind_method(D_METHOD("get_continentalness_texture"), &TerrainGenerator::get_continentalness_texture);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "continentalness_texture", PROPERTY_HINT_RESOURCE_TYPE, "NoiseTexture2D"), "set_continentalness_texture", "get_continentalness_texture");

    ClassDB::bind_method(D_METHOD("set_peaks_and_valleys_texture", "_peaks_and_valleys_texture"), &TerrainGenerator::set_peaks_and_valleys_texture);
    ClassDB::bind_method(D_METHOD("get_peaks_and_valleys_texture"), &TerrainGenerator::get_peaks_and_valleys_texture);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "peaks_and_valleys_texture", PROPERTY_HINT_RESOURCE_TYPE, "NoiseTexture2D"), "set_peaks_and_valleys_texture", "get_peaks_and_valleys_texture");

    ClassDB::bind_method(D_METHOD("set_erosion_texture", "_erosion_texture"), &TerrainGenerator::set_erosion_texture);
    ClassDB::bind_method(D_METHOD("get_erosion_texture"), &TerrainGenerator::get_erosion_texture);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "erosion_texture", PROPERTY_HINT_RESOURCE_TYPE, "NoiseTexture2D"), "set_erosion_texture", "get_erosion_texture");

    ClassDB::bind_method(D_METHOD("set_continentalness_curve", "_continentalness_curve"), &TerrainGenerator::set_continentalness_curve);
    ClassDB::bind_method(D_METHOD("get_continentalness_curve"), &TerrainGenerator::get_continentalness_curve);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "continentalness_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), "set_continentalness_curve", "get_continentalness_curve");

    ClassDB::bind_method(D_METHOD("set_peaks_and_valleys_curve", "_peaks_and_valleys_curve"), &TerrainGenerator::set_peaks_and_valleys_curve);
    ClassDB::bind_method(D_METHOD("get_peaks_and_valleys_curve"), &TerrainGenerator::get_peaks_and_valleys_curve);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "peaks_and_valleys_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), "set_peaks_and_valleys_curve", "get_peaks_and_valleys_curve");

    ClassDB::bind_method(D_METHOD("set_erosion_curve", "_erosion_curve"), &TerrainGenerator::set_erosion_curve);
    ClassDB::bind_method(D_METHOD("get_erosion_curve"), &TerrainGenerator::get_erosion_curve);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "erosion_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), "set_erosion_curve", "get_erosion_curve");

    ClassDB::bind_method(D_METHOD("set_height_scale", "_height_scale"), &TerrainGenerator::set_height_scale);
    ClassDB::bind_method(D_METHOD("get_height_scale"), &TerrainGenerator::get_height_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_scale", PROPERTY_HINT_RANGE, "0.1, 100.0, 0.01"), "set_height_scale", "get_height_scale");

    ClassDB::bind_method(D_METHOD("set_terrain_material", "_terrain_material"), &TerrainGenerator::set_terrain_material);
    ClassDB::bind_method(D_METHOD("get_terrain_material"), &TerrainGenerator::get_terrain_material);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "terrain_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_terrain_material", "get_terrain_material");

    ClassDB::bind_method(D_METHOD("set_view_distance", "_view_distance"), &TerrainGenerator::set_view_distance);
    ClassDB::bind_method(D_METHOD("get_view_distance"), &TerrainGenerator::get_view_distance);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "_view_distance", PROPERTY_HINT_RANGE, "1, 100, 1"), "set_view_distance", "get_view_distance");

    ClassDB::bind_method(D_METHOD("set_foliage_scene", "_foliage_scene"), &TerrainGenerator::set_foliage_scene);
    ClassDB::bind_method(D_METHOD("get_foliage_scene"), &TerrainGenerator::get_foliage_scene);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "foliage_scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_foliage_scene", "get_foliage_scene");

    // Debug method for monitoring chunk memory usage
    ClassDB::bind_method(D_METHOD("get_chunk_stats"), &TerrainGenerator::get_chunk_stats);

    // Method to reload all terrain chunks
    ClassDB::bind_method(D_METHOD("reload_chunks"), &TerrainGenerator::reload_chunks);
}

TerrainGenerator::TerrainGenerator()
    : height_sampler(nullptr), mesh_generator(nullptr),
      foliage_generator(nullptr), chunk_manager(nullptr) {
    recreate_components();
}

TerrainGenerator::~TerrainGenerator() {
    if (chunk_manager) {
        delete chunk_manager;
        chunk_manager = nullptr;
    }
    if (foliage_generator) {
        delete foliage_generator;
        foliage_generator = nullptr;
    }
    if (mesh_generator) {
        delete mesh_generator;
        mesh_generator = nullptr;
    }
    if (height_sampler) {
        delete height_sampler;
        height_sampler = nullptr;
    }
}

void TerrainGenerator::recreate_components() {
    // Clean up existing components
    if (chunk_manager) {
        delete chunk_manager;
        chunk_manager = nullptr;
    }
    if (foliage_generator) {
        delete foliage_generator;
        foliage_generator = nullptr;
    }
    if (mesh_generator) {
        delete mesh_generator;
        mesh_generator = nullptr;
    }
    if (height_sampler) {
        delete height_sampler;
        height_sampler = nullptr;
    }

    // Create new components
    height_sampler = new HeightSampler(&config);
    mesh_generator = new MeshGenerator(&config, height_sampler);
    foliage_generator = new FoliageGenerator(&config, height_sampler);
    chunk_manager = new ChunkManager(&config, mesh_generator, foliage_generator, this);
}

void TerrainGenerator::_ready() {
    if (chunk_manager) {
        chunk_manager->start_thread();
    }
}

void TerrainGenerator::_process(double delta) {
    Node3D *origin_node = get_node<Node3D>(origin_node_path);
    if (origin_node && chunk_manager) {
        chunk_manager->update_origin_cache(origin_node->get_global_position());
        chunk_manager->process_chunks();
    }
}

// Property setters - Some trigger component recreation, others just update config
void TerrainGenerator::set_width(int p_width) {
    if (config.width != p_width) {
        config.width = p_width;
        recreate_components();
    }
}

void TerrainGenerator::set_segment_count(int p_segment_count) {
    if (config.segment_count != p_segment_count) {
        config.segment_count = p_segment_count;
        recreate_components();
    }
}

void TerrainGenerator::set_continentalness_texture(Ref<NoiseTexture2D> p_noise_texture) {
    config.continentalness_texture = p_noise_texture;
}

void TerrainGenerator::set_peaks_and_valleys_texture(Ref<NoiseTexture2D> p_noise_texture) {
    config.peaks_and_valleys_texture = p_noise_texture;
}

void TerrainGenerator::set_erosion_texture(Ref<NoiseTexture2D> p_noise_texture) {
    config.erosion_texture = p_noise_texture;
}

void TerrainGenerator::set_continentalness_curve(Ref<Curve> p_curve) {
    config.continentalness_curve = p_curve;
}

void TerrainGenerator::set_peaks_and_valleys_curve(Ref<Curve> p_curve) {
    config.peaks_and_valleys_curve = p_curve;
}

void TerrainGenerator::set_erosion_curve(Ref<Curve> p_curve) {
    config.erosion_curve = p_curve;
}

void TerrainGenerator::set_height_scale(float p_height_scale) {
    config.height_scale = p_height_scale;
}

void TerrainGenerator::set_terrain_material(Ref<Material> p_material) {
    config.terrain_material = p_material;
}

void TerrainGenerator::set_view_distance(int p_distance) {
    if (config.view_distance != p_distance) {
        config.view_distance = p_distance;
        // View distance change doesn't require full recreation, just restart loading
        if (chunk_manager) {
            chunk_manager->reload_chunks();
        }
    }
}

void TerrainGenerator::set_foliage_scene(Ref<PackedScene> p_scene) {
    config.foliage_scene = p_scene;
}

void TerrainGenerator::reload_chunks() {
    if (chunk_manager) {
        chunk_manager->reload_chunks();
    }
}

Dictionary TerrainGenerator::get_chunk_stats() const {
    if (chunk_manager) {
        return chunk_manager->get_chunk_stats();
    }
    return Dictionary();
}

float TerrainGenerator::sample_height(float world_x, float world_z) const {
    if (height_sampler) {
        return height_sampler->sample_height(world_x, world_z);
    }
    return 0.0f;
}

Vector3 TerrainGenerator::sample_normal(float world_x, float world_z) const {
    if (height_sampler) {
        return height_sampler->sample_normal(world_x, world_z);
    }
    return Vector3(0, 1, 0);
}