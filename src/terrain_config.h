//==========================================
// terrain_config.h - Configuration and data structures
//==========================================
#ifndef TERRAIN_CONFIG_H
#define TERRAIN_CONFIG_H

#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/classes/noise_texture2d.hpp>
#include <godot_cpp/classes/curve.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/packed_scene.hpp>

namespace godot {

struct TerrainConfig {
    int width = 10;
    int segment_count = 10;
    float height_scale = 1.0f;
    int view_distance = 5;

    Ref<NoiseTexture2D> continentalness_texture;
    Ref<NoiseTexture2D> peaks_and_valleys_texture;
    Ref<NoiseTexture2D> erosion_texture;
    Ref<NoiseTexture2D> river_source_texture;

    Ref<Curve> continentalness_curve;
    Ref<Curve> peaks_and_valleys_curve;
    Ref<Curve> erosion_curve;

    Ref<Material> terrain_material;
    Ref<PackedScene> foliage_scene;

    // River carving parameters
    bool enable_river_carving = true;
    float river_carving_depth = 2.0f;          // Maximum depth to carve
    float river_carving_width_multiplier = 3.0f; // How wide the carving effect extends beyond river width
    float river_carving_smoothness = 2.0f;     // Controls the smoothness of the carving (higher = smoother)
    float river_uphill_carving_multiplier = 2.0f; // How much deeper to carve when going uphill (compensation factor)
    
    // River mesh generation parameters
    bool enable_river_mesh = true;             // Generate 3D river mesh instead of flat debug quads
    float river_mesh_depth_offset = -1.5f;    // How deep below carved terrain to place river surface (negative = below)
    float river_mesh_width_multiplier = 2.0f; // How much wider to make river mesh compared to river width
    float river_mesh_bank_safety = 0.3f;      // Extra depth below banks to prevent holes (safety margin)
    int river_mesh_subdivisions = 8;          // Number of width subdivisions for river mesh detail
    Ref<Material> river_material;             // Material to apply to river meshes (your water shader)
    
    // River flow parameters
    float river_max_turn_angle = 45.0f;        // Maximum turn angle per step in degrees
    float river_uphill_tolerance = 0.1f;       // How much uphill rivers can go when stuck
    int river_max_stuck_attempts = 5;          // How many times to try when stuck before giving up
    
    // Foliage parameters
    float foliage_river_exclusion_radius = 8.0f; // How far from rivers to exclude foliage
};

// Hash function for Vector2i
struct Vector2iHash {
    size_t operator()(const Vector2i &v) const {
        return std::hash<int32_t>()(v.x) ^ (std::hash<int32_t>()(v.y) << 1);
    }
};

}

#endif