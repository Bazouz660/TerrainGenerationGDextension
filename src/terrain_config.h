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

    Ref<Curve> continentalness_curve;
    Ref<Curve> peaks_and_valleys_curve;
    Ref<Curve> erosion_curve;

    Ref<Material> terrain_material;
    Ref<PackedScene> foliage_scene;
};

// Hash function for Vector2i
struct Vector2iHash {
    size_t operator()(const Vector2i &v) const {
        return std::hash<int32_t>()(v.x) ^ (std::hash<int32_t>()(v.y) << 1);
    }
};

}

#endif