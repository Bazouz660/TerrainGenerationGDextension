//==========================================
// chunk_manager.h - Manages chunk loading/unloading
//==========================================
#ifndef CHUNK_MANAGER_H
#define CHUNK_MANAGER_H

#include "terrain_config.h"
#include "mesh_generator.h"
#include "foliage_generator.h"
#include "safe_queue.h"
#include "safe_unordered_map.h"
#include <thread>
#include <atomic>

namespace godot {

class TerrainGenerator; // Forward declaration

class ChunkManager {
private:
    struct ChunkProcessState {
        std::vector<Vector2i> load_candidates;
        size_t load_index = 0;
        int last_origin_chunk_x = 0;
        int last_origin_chunk_z = 0;
        std::vector<std::pair<Vector2i, int>> unload_candidates;
        size_t unload_index = 0;
    };

    const TerrainConfig* config;
    MeshGenerator* mesh_generator;
    FoliageGenerator* foliage_generator;
    TerrainGenerator* terrain_node; // For adding/removing children

    SafeUnorderedMap<Vector2i, MeshInstance3D*, Vector2iHash> loading_chunks;
    SafeUnorderedMap<Vector2i, MeshInstance3D*, Vector2iHash> loaded_chunks;
    SafeUnorderedMap<Vector2i, MeshInstance3D*, Vector2iHash> unloading_chunks;
    SafeQueue<MeshInstance3D*> chunk_add_queue;

    std::jthread chunk_loader_thread;
    std::atomic<Vector3> cached_origin_position{Vector3(0, 0, 0)};
    std::atomic<bool> origin_position_valid{false};
    std::atomic<bool> should_stop_thread{false};  // FIXED: renamed from stop_thread
    bool thread_running = false;

    ChunkProcessState chunk_state;

public:
    ChunkManager(const TerrainConfig* terrain_config, MeshGenerator* mesh_gen,
                 FoliageGenerator* foliage_gen, TerrainGenerator* terrain);
    ~ChunkManager();

    void start_thread();
    void stop_thread();  // This method name is fine
    void update_origin_cache(Vector3 origin_position);
    void process_chunks(); // Called from main thread
    void reload_chunks();
    void clear_chunks();

    // Debug/stats
    Dictionary get_chunk_stats() const;

private:
    void chunk_loader_thread_function(std::stop_token stop_token);
    void add_chunks_to_load(Vector3 origin_position);
    void add_chunks_to_unload(Vector3 origin_position);
    void load_chunks();
    void unload_chunks();
};

}

#endif