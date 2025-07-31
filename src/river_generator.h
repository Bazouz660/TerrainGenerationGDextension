//==========================================
// river_generator.h - Step 1: Source Generation & Step 2: River Tracing
//==========================================
#ifndef RIVER_GENERATOR_H
#define RIVER_GENERATOR_H

#include "terrain_config.h"
#include "height_sampler.h"
#include <vector>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>

namespace godot {

struct RiverSource {
    Vector2 world_position;
    float height;
    int source_id;
};

struct RiverPoint {
    Vector2 world_position;
    float height;
    float width;  // River width at this point
};

struct RiverPath {
    int source_id;
    std::vector<RiverPoint> points;
    bool reaches_sea_level;
};

struct RiverSegment {
    Vector2 start;
    Vector2 end;
    float start_height;
    float end_height;
    float width;
    int source_id;
    float uphill_amount;  // How much this segment goes uphill (0 = downhill, positive = uphill)
};

class RiverGenerator {
private:
    const TerrainConfig* config;
    const HeightSampler* height_sampler;

    // River source generation parameters
    static constexpr float MIN_SOURCE_HEIGHT = 10.0f;  // Only place sources above this height
    static constexpr float SOURCE_THRESHOLD = 0.7f;    // Noise threshold for source placement (0-1)
    static constexpr float SOURCE_SAMPLE_STEP = 30.0f;  // Distance between sample points

    // River tracing parameters
    static constexpr float SEA_LEVEL = -15.0f;         // Height considered "sea level" (match your ocean)
    static constexpr float TRACE_STEP = 3.0f;          // Distance between river trace points (smaller for better pathfinding)
    static constexpr float SEARCH_RADIUS = 30.0f;      // Radius to search for downhill direction (larger for better pathfinding)
    static constexpr int MAX_TRACE_POINTS = 2000;      // Maximum points in a river path (increased)
    static constexpr float MIN_HEIGHT_DROP = 0.001f;    // Minimum height difference to continue tracing (smaller)
    static constexpr float BASE_RIVER_WIDTH = 2.0f;    // Starting river width
    static constexpr float WIDTH_GROWTH_RATE = 0.01f;  // How much river grows per trace step (slower growth)

public:
    RiverGenerator(const TerrainConfig* terrain_config, const HeightSampler* sampler);

    // Main interface for chunk generation
    std::vector<RiverSource> get_river_sources_for_chunk(Vector2i chunk_pos) const;
    std::vector<RiverSegment> get_river_segments_for_chunk(Vector2i chunk_pos) const;
    std::vector<RiverSegment> get_river_segments_for_carving(Vector2i chunk_pos) const;  // Larger search radius for carving
    std::vector<RiverSegment> get_river_segments_for_foliage(Vector2i chunk_pos) const;  // Optimized search for foliage exclusion

    // Debug visualization
    void add_debug_sources_to_chunk(MeshInstance3D* chunk_mesh, Vector2i chunk_pos) const;
    void add_debug_rivers_to_chunk(MeshInstance3D* chunk_mesh, Vector2i chunk_pos) const;

private:
    // Source generation helpers
    std::vector<RiverSource> find_river_sources_in_region(Vector2i center_chunk, int search_radius) const;
    bool should_place_river_source(Vector2 world_pos) const;

    // River tracing helpers
    RiverPath trace_river_from_source(const RiverSource& source) const;
    Vector2 find_downhill_direction(Vector2 current_pos, float current_height) const;
    Vector2 find_downhill_direction_adaptive(Vector2 current_pos, float current_height, float search_radius) const;
    Vector2 find_best_river_direction(Vector2 current_pos, float current_height, float search_radius, 
                                     float tolerance_height_gain, Vector2 last_direction) const;
    std::vector<RiverSegment> extract_segments_for_chunk(const RiverPath& river, Vector2i chunk_pos) const;
    std::vector<RiverSegment> extract_segments_for_carving(const RiverPath& river, Vector2i chunk_pos) const;
    bool segment_intersects_chunk(const RiverSegment& segment, Vector2i chunk_pos) const;
    bool segment_affects_chunk_carving(const RiverSegment& segment, Vector2i chunk_pos) const;

    // Debug mesh creation
    MeshInstance3D* create_debug_source_marker(const RiverSource& source) const;
    MeshInstance3D* create_debug_river_segment(const RiverSegment& segment) const;

public:
    // Advanced river mesh generation
    void add_river_meshes_to_chunk(MeshInstance3D* chunk_mesh, Vector2i chunk_pos) const;

private:
    // River mesh generation helpers
    MeshInstance3D* create_river_mesh_for_segment(const RiverSegment& segment) const;
    MeshInstance3D* create_continuous_river_mesh(const RiverPath& river, Vector2i chunk_pos) const;
    Ref<ArrayMesh> generate_river_geometry(const RiverSegment& segment) const;
    Ref<ArrayMesh> generate_continuous_river_geometry(const RiverPath& river, Vector2i chunk_pos) const;
};

}

#endif