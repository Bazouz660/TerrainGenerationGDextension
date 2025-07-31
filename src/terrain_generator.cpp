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

    ClassDB::bind_method(D_METHOD("set_river_source_texture", "_river_source_texture"), &TerrainGenerator::set_river_source_texture);
    ClassDB::bind_method(D_METHOD("get_river_source_texture"), &TerrainGenerator::get_river_source_texture);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "river_source_texture", PROPERTY_HINT_RESOURCE_TYPE, "NoiseTexture2D"), "set_river_source_texture", "get_river_source_texture");

    ClassDB::bind_method(D_METHOD("set_enable_river_carving", "_enable_river_carving"), &TerrainGenerator::set_enable_river_carving);
    ClassDB::bind_method(D_METHOD("get_enable_river_carving"), &TerrainGenerator::get_enable_river_carving);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enable_river_carving"), "set_enable_river_carving", "get_enable_river_carving");

    ClassDB::bind_method(D_METHOD("set_river_carving_depth", "_river_carving_depth"), &TerrainGenerator::set_river_carving_depth);
    ClassDB::bind_method(D_METHOD("get_river_carving_depth"), &TerrainGenerator::get_river_carving_depth);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "river_carving_depth", PROPERTY_HINT_RANGE, "0.1, 10.0, 0.1"), "set_river_carving_depth", "get_river_carving_depth");

    ClassDB::bind_method(D_METHOD("set_river_carving_width_multiplier", "_river_carving_width_multiplier"), &TerrainGenerator::set_river_carving_width_multiplier);
    ClassDB::bind_method(D_METHOD("get_river_carving_width_multiplier"), &TerrainGenerator::get_river_carving_width_multiplier);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "river_carving_width_multiplier", PROPERTY_HINT_RANGE, "1.0, 10.0, 0.1"), "set_river_carving_width_multiplier", "get_river_carving_width_multiplier");

    ClassDB::bind_method(D_METHOD("set_river_carving_smoothness", "_river_carving_smoothness"), &TerrainGenerator::set_river_carving_smoothness);
    ClassDB::bind_method(D_METHOD("get_river_carving_smoothness"), &TerrainGenerator::get_river_carving_smoothness);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "river_carving_smoothness", PROPERTY_HINT_RANGE, "0.5, 5.0, 0.1"), "set_river_carving_smoothness", "get_river_carving_smoothness");

    ClassDB::bind_method(D_METHOD("set_river_uphill_carving_multiplier", "_river_uphill_carving_multiplier"), &TerrainGenerator::set_river_uphill_carving_multiplier);
    ClassDB::bind_method(D_METHOD("get_river_uphill_carving_multiplier"), &TerrainGenerator::get_river_uphill_carving_multiplier);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "river_uphill_carving_multiplier", PROPERTY_HINT_RANGE, "1.0, 5.0, 0.1"), "set_river_uphill_carving_multiplier", "get_river_uphill_carving_multiplier");

    ClassDB::bind_method(D_METHOD("set_river_max_turn_angle", "_river_max_turn_angle"), &TerrainGenerator::set_river_max_turn_angle);
    ClassDB::bind_method(D_METHOD("get_river_max_turn_angle"), &TerrainGenerator::get_river_max_turn_angle);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "river_max_turn_angle", PROPERTY_HINT_RANGE, "15.0, 90.0, 5.0"), "set_river_max_turn_angle", "get_river_max_turn_angle");

    ClassDB::bind_method(D_METHOD("set_river_uphill_tolerance", "_river_uphill_tolerance"), &TerrainGenerator::set_river_uphill_tolerance);
    ClassDB::bind_method(D_METHOD("get_river_uphill_tolerance"), &TerrainGenerator::get_river_uphill_tolerance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "river_uphill_tolerance", PROPERTY_HINT_RANGE, "0.0, 1.0, 0.05"), "set_river_uphill_tolerance", "get_river_uphill_tolerance");

    ClassDB::bind_method(D_METHOD("set_river_max_stuck_attempts", "_river_max_stuck_attempts"), &TerrainGenerator::set_river_max_stuck_attempts);
    ClassDB::bind_method(D_METHOD("get_river_max_stuck_attempts"), &TerrainGenerator::get_river_max_stuck_attempts);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "river_max_stuck_attempts", PROPERTY_HINT_RANGE, "3, 10, 1"), "set_river_max_stuck_attempts", "get_river_max_stuck_attempts");

    ClassDB::bind_method(D_METHOD("set_foliage_river_exclusion_radius", "_foliage_river_exclusion_radius"), &TerrainGenerator::set_foliage_river_exclusion_radius);
    ClassDB::bind_method(D_METHOD("get_foliage_river_exclusion_radius"), &TerrainGenerator::get_foliage_river_exclusion_radius);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "foliage_river_exclusion_radius", PROPERTY_HINT_RANGE, "0.0, 20.0, 0.5"), "set_foliage_river_exclusion_radius", "get_foliage_river_exclusion_radius");

    // River mesh generation properties
    ClassDB::bind_method(D_METHOD("set_enable_river_mesh", "_enable_river_mesh"), &TerrainGenerator::set_enable_river_mesh);
    ClassDB::bind_method(D_METHOD("get_enable_river_mesh"), &TerrainGenerator::get_enable_river_mesh);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enable_river_mesh"), "set_enable_river_mesh", "get_enable_river_mesh");

    ClassDB::bind_method(D_METHOD("set_river_mesh_depth_offset", "_river_mesh_depth_offset"), &TerrainGenerator::set_river_mesh_depth_offset);
    ClassDB::bind_method(D_METHOD("get_river_mesh_depth_offset"), &TerrainGenerator::get_river_mesh_depth_offset);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "river_mesh_depth_offset", PROPERTY_HINT_RANGE, "-5.0, 2.0, 0.1"), "set_river_mesh_depth_offset", "get_river_mesh_depth_offset");

    ClassDB::bind_method(D_METHOD("set_river_mesh_width_multiplier", "_river_mesh_width_multiplier"), &TerrainGenerator::set_river_mesh_width_multiplier);
    ClassDB::bind_method(D_METHOD("get_river_mesh_width_multiplier"), &TerrainGenerator::get_river_mesh_width_multiplier);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "river_mesh_width_multiplier", PROPERTY_HINT_RANGE, "0.5, 5.0, 0.1"), "set_river_mesh_width_multiplier", "get_river_mesh_width_multiplier");

    ClassDB::bind_method(D_METHOD("set_river_mesh_bank_safety", "_river_mesh_bank_safety"), &TerrainGenerator::set_river_mesh_bank_safety);
    ClassDB::bind_method(D_METHOD("get_river_mesh_bank_safety"), &TerrainGenerator::get_river_mesh_bank_safety);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "river_mesh_bank_safety", PROPERTY_HINT_RANGE, "0.0, 1.0, 0.05"), "set_river_mesh_bank_safety", "get_river_mesh_bank_safety");

    ClassDB::bind_method(D_METHOD("set_river_mesh_subdivisions", "_river_mesh_subdivisions"), &TerrainGenerator::set_river_mesh_subdivisions);
    ClassDB::bind_method(D_METHOD("get_river_mesh_subdivisions"), &TerrainGenerator::get_river_mesh_subdivisions);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "river_mesh_subdivisions", PROPERTY_HINT_RANGE, "4, 20, 1"), "set_river_mesh_subdivisions", "get_river_mesh_subdivisions");

    ClassDB::bind_method(D_METHOD("set_river_material", "_river_material"), &TerrainGenerator::set_river_material);
    ClassDB::bind_method(D_METHOD("get_river_material"), &TerrainGenerator::get_river_material);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "river_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_river_material", "get_river_material");

    // Debug method for monitoring chunk memory usage
    ClassDB::bind_method(D_METHOD("get_chunk_stats"), &TerrainGenerator::get_chunk_stats);

    // Method to reload all terrain chunks
    ClassDB::bind_method(D_METHOD("reload_chunks"), &TerrainGenerator::reload_chunks);
}

TerrainGenerator::TerrainGenerator()
    : height_sampler(nullptr), mesh_generator(nullptr),
      foliage_generator(nullptr), river_generator(nullptr), chunk_manager(nullptr) {
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
    if (river_generator) {
        delete river_generator;
        river_generator = nullptr;
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
    if (river_generator) {
        delete river_generator;
        river_generator = nullptr;
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
    river_generator = new RiverGenerator(&config, height_sampler);
    
    chunk_manager = new ChunkManager(&config, mesh_generator, foliage_generator, river_generator, this);
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

void TerrainGenerator::set_enable_river_carving(bool p_enable) {
    if (config.enable_river_carving != p_enable) {
        config.enable_river_carving = p_enable;
        // River carving setting change requires reloading chunks
        if (chunk_manager) {
            chunk_manager->reload_chunks();
        }
    }
}

void TerrainGenerator::set_river_carving_depth(float p_depth) {
    if (config.river_carving_depth != p_depth) {
        config.river_carving_depth = p_depth;
        // River carving parameter change requires reloading chunks
        if (chunk_manager) {
            chunk_manager->reload_chunks();
        }
    }
}

void TerrainGenerator::set_river_carving_width_multiplier(float p_multiplier) {
    if (config.river_carving_width_multiplier != p_multiplier) {
        config.river_carving_width_multiplier = p_multiplier;
        // River carving parameter change requires reloading chunks
        if (chunk_manager) {
            chunk_manager->reload_chunks();
        }
    }
}

void TerrainGenerator::set_river_carving_smoothness(float p_smoothness) {
    if (config.river_carving_smoothness != p_smoothness) {
        config.river_carving_smoothness = p_smoothness;
        // River carving parameter change requires reloading chunks
        if (chunk_manager) {
            chunk_manager->reload_chunks();
        }
    }
}

float TerrainGenerator::get_river_carving_smoothness() const {
    return config.river_carving_smoothness;
}

void TerrainGenerator::set_river_uphill_carving_multiplier(float p_multiplier) {
    if (config.river_uphill_carving_multiplier != p_multiplier) {
        config.river_uphill_carving_multiplier = p_multiplier;
        // River carving parameter change requires reloading chunks
        if (chunk_manager) {
            chunk_manager->reload_chunks();
        }
    }
}

float TerrainGenerator::get_river_uphill_carving_multiplier() const {
    return config.river_uphill_carving_multiplier;
}

void TerrainGenerator::set_river_max_turn_angle(float p_angle) {
    if (config.river_max_turn_angle != p_angle) {
        config.river_max_turn_angle = p_angle;
        // River flow parameter change requires reloading chunks
        if (chunk_manager) {
            chunk_manager->reload_chunks();
        }
    }
}

void TerrainGenerator::set_river_uphill_tolerance(float p_tolerance) {
    if (config.river_uphill_tolerance != p_tolerance) {
        config.river_uphill_tolerance = p_tolerance;
        // River flow parameter change requires reloading chunks
        if (chunk_manager) {
            chunk_manager->reload_chunks();
        }
    }
}

void TerrainGenerator::set_river_max_stuck_attempts(int p_attempts) {
    if (config.river_max_stuck_attempts != p_attempts) {
        config.river_max_stuck_attempts = p_attempts;
        // River flow parameter change requires reloading chunks
        if (chunk_manager) {
            chunk_manager->reload_chunks();
        }
    }
}

void TerrainGenerator::set_foliage_river_exclusion_radius(float p_radius) {
    if (config.foliage_river_exclusion_radius != p_radius) {
        config.foliage_river_exclusion_radius = p_radius;
        // Foliage parameter change requires reloading chunks
        if (chunk_manager) {
            chunk_manager->reload_chunks();
        }
    }
}

// River mesh generation setters
void TerrainGenerator::set_enable_river_mesh(bool p_enable) {
    if (config.enable_river_mesh != p_enable) {
        config.enable_river_mesh = p_enable;
        // River mesh setting change requires reloading chunks
        if (chunk_manager) {
            chunk_manager->reload_chunks();
        }
    }
}

void TerrainGenerator::set_river_mesh_depth_offset(float p_offset) {
    if (config.river_mesh_depth_offset != p_offset) {
        config.river_mesh_depth_offset = p_offset;
        // River mesh parameter change requires reloading chunks
        if (chunk_manager) {
            chunk_manager->reload_chunks();
        }
    }
}

void TerrainGenerator::set_river_mesh_width_multiplier(float p_multiplier) {
    if (config.river_mesh_width_multiplier != p_multiplier) {
        config.river_mesh_width_multiplier = p_multiplier;
        // River mesh parameter change requires reloading chunks
        if (chunk_manager) {
            chunk_manager->reload_chunks();
        }
    }
}

void TerrainGenerator::set_river_mesh_bank_safety(float p_safety) {
    if (config.river_mesh_bank_safety != p_safety) {
        config.river_mesh_bank_safety = p_safety;
        // River mesh parameter change requires reloading chunks
        if (chunk_manager) {
            chunk_manager->reload_chunks();
        }
    }
}

void TerrainGenerator::set_river_mesh_subdivisions(int p_subdivisions) {
    if (config.river_mesh_subdivisions != p_subdivisions) {
        config.river_mesh_subdivisions = p_subdivisions;
        // River mesh parameter change requires reloading chunks
        if (chunk_manager) {
            chunk_manager->reload_chunks();
        }
    }
}

void TerrainGenerator::set_river_material(const Ref<Material>& p_material) {
    if (config.river_material != p_material) {
        config.river_material = p_material;
        // River material change requires reloading chunks
        if (chunk_manager) {
            chunk_manager->reload_chunks();
        }
    }
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