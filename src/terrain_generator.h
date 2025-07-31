//==========================================
// terrain_generator.h - Simplified main class
//==========================================
#ifndef TERRAIN_GENERATOR_H
#define TERRAIN_GENERATOR_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include "terrain_config.h"
#include "height_sampler.h"
#include "mesh_generator.h"
#include "foliage_generator.h"
#include "chunk_manager.h"
#include "river_generator.h"

namespace godot {

class TerrainGenerator : public Node3D {
    GDCLASS(TerrainGenerator, Node3D)

private:
    TerrainConfig config;
    NodePath origin_node_path;

    // Components
    HeightSampler* height_sampler;
    MeshGenerator* mesh_generator;
    FoliageGenerator* foliage_generator;
    RiverGenerator* river_generator;
    ChunkManager* chunk_manager;

    // Property setters/getters
    void set_width(int p_width);
    int get_width() const { return config.width; }

    void set_segment_count(int p_segment_count);
    int get_segment_count() const { return config.segment_count; }

    void set_origin_node_path(NodePath p_origin_path) { origin_node_path = p_origin_path; }
    NodePath get_origin_node_path() const { return origin_node_path; }

    void set_continentalness_texture(Ref<NoiseTexture2D> p_noise_texture);
    Ref<NoiseTexture2D> get_continentalness_texture() const { return config.continentalness_texture; }

    void set_peaks_and_valleys_texture(Ref<NoiseTexture2D> p_noise_texture);
    Ref<NoiseTexture2D> get_peaks_and_valleys_texture() const { return config.peaks_and_valleys_texture; }

    void set_erosion_texture(Ref<NoiseTexture2D> p_noise_texture);
    Ref<NoiseTexture2D> get_erosion_texture() const { return config.erosion_texture; }

    void set_continentalness_curve(Ref<Curve> p_curve);
    Ref<Curve> get_continentalness_curve() const { return config.continentalness_curve; }

    void set_peaks_and_valleys_curve(Ref<Curve> p_curve);
    Ref<Curve> get_peaks_and_valleys_curve() const { return config.peaks_and_valleys_curve; }

    void set_erosion_curve(Ref<Curve> p_curve);
    Ref<Curve> get_erosion_curve() const { return config.erosion_curve; }

    void set_height_scale(float p_height_scale);
    float get_height_scale() const { return config.height_scale; }

    void set_terrain_material(Ref<Material> p_material);
    Ref<Material> get_terrain_material() const { return config.terrain_material; }

    void set_view_distance(int p_distance);
    int get_view_distance() const { return config.view_distance; }

    void set_foliage_scene(Ref<PackedScene> p_scene);
    Ref<PackedScene> get_foliage_scene() const { return config.foliage_scene; }

    void set_river_source_texture(Ref<NoiseTexture2D> p_texture) { config.river_source_texture = p_texture; }
    Ref<NoiseTexture2D> get_river_source_texture() const { return config.river_source_texture; }

    void set_enable_river_carving(bool p_enable);
    bool get_enable_river_carving() const { return config.enable_river_carving; }

    void set_river_carving_depth(float p_depth);
    float get_river_carving_depth() const { return config.river_carving_depth; }

    void set_river_carving_width_multiplier(float p_multiplier);
    float get_river_carving_width_multiplier() const { return config.river_carving_width_multiplier; }

    void set_river_carving_smoothness(float p_smoothness);
    float get_river_carving_smoothness() const;

    void set_river_uphill_carving_multiplier(float p_multiplier);
    float get_river_uphill_carving_multiplier() const;

    void set_river_max_turn_angle(float p_angle);
    float get_river_max_turn_angle() const { return config.river_max_turn_angle; }

    void set_river_uphill_tolerance(float p_tolerance);
    float get_river_uphill_tolerance() const { return config.river_uphill_tolerance; }

    void set_river_max_stuck_attempts(int p_attempts);
    int get_river_max_stuck_attempts() const { return config.river_max_stuck_attempts; }

    void set_foliage_river_exclusion_radius(float p_radius);
    float get_foliage_river_exclusion_radius() const { return config.foliage_river_exclusion_radius; }

    // River mesh generation properties
    void set_enable_river_mesh(bool p_enable);
    bool get_enable_river_mesh() const { return config.enable_river_mesh; }

    void set_river_mesh_depth_offset(float p_offset);
    float get_river_mesh_depth_offset() const { return config.river_mesh_depth_offset; }

    void set_river_mesh_width_multiplier(float p_multiplier);
    float get_river_mesh_width_multiplier() const { return config.river_mesh_width_multiplier; }

    void set_river_mesh_bank_safety(float p_safety);
    float get_river_mesh_bank_safety() const { return config.river_mesh_bank_safety; }

    void set_river_mesh_subdivisions(int p_subdivisions);
    int get_river_mesh_subdivisions() const { return config.river_mesh_subdivisions; }

    void set_river_material(const Ref<Material>& p_material);
    Ref<Material> get_river_material() const { return config.river_material; }

    void recreate_components();

protected:
    static void _bind_methods();

public:
    TerrainGenerator();
    ~TerrainGenerator();

    void _ready() override;
    void _process(double delta) override;

    void reload_chunks();
    Dictionary get_chunk_stats() const;

    // Public interface for components to access
    float sample_height(float world_x, float world_z) const;
    Vector3 sample_normal(float world_x, float world_z) const;
};

}

#endif