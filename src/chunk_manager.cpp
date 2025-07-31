//==========================================
// chunk_manager.cpp
//==========================================
#include "chunk_manager.h"
#include "terrain_generator.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <algorithm>
#include <chrono>

using namespace godot;

ChunkManager::ChunkManager(const TerrainConfig* terrain_config, MeshGenerator* mesh_gen,
                          FoliageGenerator* foliage_gen, RiverGenerator* river_gen, TerrainGenerator* terrain)
    : config(terrain_config), mesh_generator(mesh_gen), foliage_generator(foliage_gen),
      river_generator(river_gen), terrain_node(terrain), cached_origin_position(Vector3(0, 0, 0)),
      origin_position_valid(false), should_stop_thread(false), thread_running(false) {
}

ChunkManager::~ChunkManager() {
    stop_thread();
    clear_chunks();
}

void ChunkManager::start_thread() {
    if (thread_running) {
        return; // Thread already running
    }

    should_stop_thread.store(false);
    chunk_loader_thread = std::jthread([this](std::stop_token stop_token) {
        chunk_loader_thread_function(stop_token);
    });
}

void ChunkManager::stop_thread() {
    if (chunk_loader_thread.joinable()) {
        chunk_loader_thread.request_stop();
        should_stop_thread.store(true);
        chunk_loader_thread.join();
    }
}

void ChunkManager::update_origin_cache(Vector3 origin_position) {
    cached_origin_position.store(origin_position);
    origin_position_valid.store(true);
}

void ChunkManager::process_chunks() {
    load_chunks();
    unload_chunks();

    // Handle thread restart if needed
    if (should_stop_thread.load() && !thread_running) {
        should_stop_thread.store(false);
        clear_chunks();
        start_thread();
    }
}

void ChunkManager::reload_chunks() {
    if (!thread_running) {
        return;
    }

    chunk_loader_thread.request_stop();
    should_stop_thread.store(true);

    // Reset chunk process state so loading/unloading starts from center again
    chunk_state.load_candidates.clear();
    chunk_state.load_index = 0;
    chunk_state.unload_candidates.clear();
    chunk_state.unload_index = 0;
}

Dictionary ChunkManager::get_chunk_stats() const {
    Dictionary stats;
    stats["loaded"] = loaded_chunks.size();
    stats["loading"] = loading_chunks.size();
    stats["unloading"] = unloading_chunks.size();
    stats["queued"] = chunk_add_queue.size();
    return stats;
}

void ChunkManager::chunk_loader_thread_function(std::stop_token stop_token) {
    thread_running = true;
    print_line("Chunk loader thread started.");

    while (!stop_token.stop_requested()) {
        Vector3 origin_pos = cached_origin_position.load();
        bool position_valid = origin_position_valid.load();

        if (position_valid) {
            add_chunks_to_load(origin_pos);
            add_chunks_to_unload(origin_pos);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    print_line("Chunk loader thread stopped.");
    thread_running = false;
}

void ChunkManager::add_chunks_to_load(Vector3 origin_position) {
    int origin_chunk_x = (int)round(origin_position.x / config->width);
    int origin_chunk_z = (int)round(origin_position.z / config->width);

    // If origin changed or candidates exhausted, recalculate
    if (chunk_state.load_candidates.empty() ||
        origin_chunk_x != chunk_state.last_origin_chunk_x ||
        origin_chunk_z != chunk_state.last_origin_chunk_z ||
        chunk_state.load_index >= chunk_state.load_candidates.size()) {

        chunk_state.load_candidates.clear();
        chunk_state.load_index = 0;
        chunk_state.last_origin_chunk_x = origin_chunk_x;
        chunk_state.last_origin_chunk_z = origin_chunk_z;

        int view_dist_sq = config->view_distance * config->view_distance;

        for (int z = -config->view_distance; z <= config->view_distance; z++) {
            for (int x = -config->view_distance; x <= config->view_distance; x++) {
                int dist_sq = x * x + z * z;
                if (dist_sq > view_dist_sq) continue;

                Vector2i chunk_pos(origin_chunk_x + x, origin_chunk_z + z);

                if (!loaded_chunks.contains(chunk_pos) &&
                    !loading_chunks.contains(chunk_pos) &&
                    !unloading_chunks.contains(chunk_pos)) {
                    chunk_state.load_candidates.push_back(chunk_pos);
                }
            }
        }

        // Sort by distance from origin
        std::sort(chunk_state.load_candidates.begin(), chunk_state.load_candidates.end(),
                 [origin_chunk_x, origin_chunk_z](const Vector2i& a, const Vector2i& b) {
            int da = (a.x - origin_chunk_x) * (a.x - origin_chunk_x) +
                    (a.y - origin_chunk_z) * (a.y - origin_chunk_z);
            int db = (b.x - origin_chunk_x) * (b.x - origin_chunk_x) +
                    (b.y - origin_chunk_z) * (b.y - origin_chunk_z);
            return da < db;
        });
    }

    // Process only one chunk per cycle
    if (chunk_state.load_index < chunk_state.load_candidates.size()) {
        const Vector2i& chunk_pos = chunk_state.load_candidates[chunk_state.load_index];

        // Get river segments that affect this chunk (for both carving and foliage exclusion)
        std::vector<RiverSegment> carving_river_segments;
        std::vector<RiverSegment> foliage_river_segments;
        
        if (river_generator) {
            if (config->enable_river_carving) {
                carving_river_segments = river_generator->get_river_segments_for_carving(chunk_pos);
            }
            // Always get river segments for foliage exclusion (separate from carving)
            foliage_river_segments = river_generator->get_river_segments_for_foliage(chunk_pos);
        }

        // Generate the mesh with river carving if enabled, otherwise use standard generation
        MeshInstance3D *chunk_mesh;
        if (config->enable_river_carving && !carving_river_segments.empty()) {
            chunk_mesh = mesh_generator->generate_chunk_mesh_with_rivers(chunk_pos, carving_river_segments);
        } else {
            chunk_mesh = mesh_generator->generate_chunk_mesh(chunk_pos);
        }

        // Add foliage to the chunk, excluding river areas
        if (!foliage_river_segments.empty()) {
            foliage_generator->populate_chunk_foliage_with_rivers(chunk_mesh, chunk_pos, foliage_river_segments);
        } else {
            foliage_generator->populate_chunk_foliage(chunk_mesh, chunk_pos);
        }
        
        // Add river sources debug markers to the chunk
        if (river_generator) {
            river_generator->add_debug_sources_to_chunk(chunk_mesh, chunk_pos);
            
            // Add either proper river meshes or debug river segments
            if (config->enable_river_mesh) {
                river_generator->add_river_meshes_to_chunk(chunk_mesh, chunk_pos);
            } else {
                river_generator->add_debug_rivers_to_chunk(chunk_mesh, chunk_pos);
            }
        }

        loading_chunks.insert_or_assign(chunk_pos, chunk_mesh);

        if (should_stop_thread.load()) {
            print_line("Stopping thread during chunk addition.");
            return;
        }

        chunk_add_queue.enqueue(chunk_mesh);
        chunk_state.load_index++;
    }
}

void ChunkManager::add_chunks_to_unload(Vector3 origin_position) {
    int origin_chunk_x = (int)round(origin_position.x / config->width);
    int origin_chunk_z = (int)round(origin_position.z / config->width);

    auto loaded_keys = loaded_chunks.keys();

    // If origin changed or candidates exhausted, recalculate
    if (chunk_state.unload_candidates.empty() ||
        origin_chunk_x != chunk_state.last_origin_chunk_x ||
        origin_chunk_z != chunk_state.last_origin_chunk_z ||
        chunk_state.unload_index >= chunk_state.unload_candidates.size()) {

        chunk_state.unload_candidates.clear();
        chunk_state.unload_index = 0;
        chunk_state.last_origin_chunk_x = origin_chunk_x;
        chunk_state.last_origin_chunk_z = origin_chunk_z;

        int view_dist_sq = config->view_distance * config->view_distance;

        for (const Vector2i& chunk_pos : loaded_keys) {
            auto chunk_mesh_opt = loaded_chunks.get(chunk_pos);
            if (!chunk_mesh_opt.has_value() || !chunk_mesh_opt.value()) {
                loaded_chunks.erase(chunk_pos);
                continue;
            }

            int dx = chunk_pos.x - origin_chunk_x;
            int dz = chunk_pos.y - origin_chunk_z;
            int dist_sq = dx * dx + dz * dz;

            if (dist_sq > view_dist_sq) {
                chunk_state.unload_candidates.push_back({chunk_pos, dist_sq});
            }
        }

        // Sort by distance (furthest first)
        std::sort(chunk_state.unload_candidates.begin(), chunk_state.unload_candidates.end(),
                 [](const std::pair<Vector2i, int>& a, const std::pair<Vector2i, int>& b) {
            return a.second > b.second;
        });
    }

    // Process only one chunk per cycle
    if (chunk_state.unload_index < chunk_state.unload_candidates.size()) {
        const Vector2i& chunk_pos = chunk_state.unload_candidates[chunk_state.unload_index].first;
        auto chunk_mesh_opt = loaded_chunks.get(chunk_pos);

        if (chunk_mesh_opt.has_value()) {
            MeshInstance3D *chunk_mesh = chunk_mesh_opt.value();
            unloading_chunks.insert_or_assign(chunk_pos, chunk_mesh);
            loaded_chunks.erase(chunk_pos);
        }

        chunk_state.unload_index++;
    }
}

void ChunkManager::load_chunks() {
    while (!chunk_add_queue.empty()) {
        MeshInstance3D *chunk_mesh = chunk_add_queue.dequeue();
        if (chunk_mesh) {
            auto chunk_pos_opt = loading_chunks.get_key(chunk_mesh);
            if (chunk_pos_opt.has_value()) {
                Vector2i chunk_pos = chunk_pos_opt.value();
                terrain_node->add_child(chunk_mesh);
                loaded_chunks.insert_or_assign(chunk_pos, chunk_mesh);
                loading_chunks.erase(chunk_pos);
            } else {
                // Clean up orphaned chunk if not found in loading chunks
                memdelete(chunk_mesh);
            }
        }
    }
}

void ChunkManager::unload_chunks() {
    // Process all chunks in unloading state
    auto unloading_keys = unloading_chunks.keys();
    for (const Vector2i& chunk_pos : unloading_keys) {
        auto chunk_mesh_opt = unloading_chunks.get(chunk_pos);
        if (chunk_mesh_opt.has_value()) {
            MeshInstance3D* chunk_mesh = chunk_mesh_opt.value();
            if (chunk_mesh) {
                if (chunk_mesh->get_parent()) {
                    terrain_node->remove_child(chunk_mesh);
                }
                memdelete(chunk_mesh);
            }
        }
        unloading_chunks.erase(chunk_pos);
    }
}

void ChunkManager::clear_chunks() {
    print_line("Clearing all chunks...");

    // Helper lambda to clean up chunk maps
    auto cleanup_chunk_map = [this](auto& chunk_map) {
        auto keys = chunk_map.keys();
        for (const Vector2i& chunk_pos : keys) {
            auto chunk_mesh = chunk_map.get(chunk_pos);
            if (chunk_mesh.has_value() && chunk_mesh.value()) {
                if (chunk_mesh.value()->get_parent()) {
                    terrain_node->remove_child(chunk_mesh.value());
                }
                memdelete(chunk_mesh.value());
            }
        }
        chunk_map.clear();
    };

    cleanup_chunk_map(loaded_chunks);
    cleanup_chunk_map(loading_chunks);
    cleanup_chunk_map(unloading_chunks);

    // Clear the chunk add queue
    while (!chunk_add_queue.empty()) {
        MeshInstance3D* chunk_mesh = chunk_add_queue.try_dequeue().value_or(nullptr);
        if (chunk_mesh) {
            memdelete(chunk_mesh);
        } else {
            break;
        }
    }

    print_line("All chunks cleared.");
}