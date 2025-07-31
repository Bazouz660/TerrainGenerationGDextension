//==========================================
// river_generator.cpp - Step 1: Source Generation & Step 2: River Tracing
//==========================================
#include "river_generator.h"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/surface_tool.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/noise_texture2d.hpp>
#include <godot_cpp/classes/noise.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace godot;

RiverGenerator::RiverGenerator(const TerrainConfig* terrain_config, const HeightSampler* sampler)
    : config(terrain_config), height_sampler(sampler) {
}

std::vector<RiverSource> RiverGenerator::get_river_sources_for_chunk(Vector2i chunk_pos) const {
    // Search in a wider radius to catch sources that might affect this chunk
    int search_radius = 3; // Search 3 chunks in each direction
    return find_river_sources_in_region(chunk_pos, search_radius);
}

void RiverGenerator::add_debug_sources_to_chunk(MeshInstance3D* chunk_mesh, Vector2i chunk_pos) const {
    if (!chunk_mesh) return;

    std::vector<RiverSource> sources = get_river_sources_for_chunk(chunk_pos);

    float chunk_world_x = chunk_pos.x * config->width;
    float chunk_world_z = chunk_pos.y * config->width;

    for (const RiverSource& source : sources) {
        // Check if source is actually within or near this chunk for visualization
        float local_x = source.world_position.x - chunk_world_x;
        float local_z = source.world_position.y - chunk_world_z;

        // Only show sources that are reasonably close to this chunk
        if (local_x >= -config->width && local_x <= config->width * 2 &&
            local_z >= -config->width && local_z <= config->width * 2) {

            MeshInstance3D* debug_marker = create_debug_source_marker(source);
            debug_marker->set_position(Vector3(local_x, source.height + 2.0f, local_z));
            chunk_mesh->add_child(debug_marker);
        }
    }
}

std::vector<RiverSource> RiverGenerator::find_river_sources_in_region(Vector2i center_chunk, int search_radius) const {
    std::vector<RiverSource> sources;

    // Skip if no noise texture is set
    if (config->river_source_texture.is_null() || config->river_source_texture->get_noise().is_null()) {
        return sources;
    }

    // Calculate world bounds for the search region
    float region_size = config->width * (2 * search_radius + 1);
    float start_x = (center_chunk.x - search_radius) * config->width;
    float start_z = (center_chunk.y - search_radius) * config->width;
    float end_x = start_x + region_size;
    float end_z = start_z + region_size;

    // Grid size for source placement - ensures sources are well-spaced
    const float GRID_SIZE = 250.0f; // Increased minimum distance between sources
    const int SAMPLES_PER_GRID = 6; // More samples within each grid cell to find best spot

    int source_id = 0;

    // Use a global grid system - align grid to world coordinates, not search region
    // This ensures consistency across different chunk requests
    int grid_start_x = static_cast<int>(std::floor(start_x / GRID_SIZE));
    int grid_start_z = static_cast<int>(std::floor(start_z / GRID_SIZE));
    int grid_end_x = static_cast<int>(std::ceil(end_x / GRID_SIZE));
    int grid_end_z = static_cast<int>(std::ceil(end_z / GRID_SIZE));

    for (int grid_z = grid_start_z; grid_z <= grid_end_z; grid_z++) {
        for (int grid_x = grid_start_x; grid_x <= grid_end_x; grid_x++) {
            // Calculate the world bounds for this global grid cell
            float cell_start_x = grid_x * GRID_SIZE;
            float cell_start_z = grid_z * GRID_SIZE;
            float cell_end_x = cell_start_x + GRID_SIZE;
            float cell_end_z = cell_start_z + GRID_SIZE;

            // Only process cells that overlap with our search region
            if (cell_end_x < start_x || cell_start_x > end_x ||
                cell_end_z < start_z || cell_start_z > end_z) {
                continue;
            }

            // Use a deterministic sampling pattern based on grid coordinates
            // This ensures the same source is always generated for this grid cell
            Vector2 best_position;
            float best_score = -1.0f;
            bool found_valid_spot = false;

            // Sample multiple points within the grid cell to find the best one
            float sample_step = GRID_SIZE / SAMPLES_PER_GRID;
            for (int sample_z = 0; sample_z < SAMPLES_PER_GRID; sample_z++) {
                for (int sample_x = 0; sample_x < SAMPLES_PER_GRID; sample_x++) {
                    float x = cell_start_x + (sample_x + 0.5f) * sample_step;
                    float z = cell_start_z + (sample_z + 0.5f) * sample_step;
                    Vector2 test_pos(x, z);

                    if (should_place_river_source(test_pos)) {
                        float height = height_sampler->sample_height(x, z);

                        if (height >= MIN_SOURCE_HEIGHT) {
                            // Check if this position has a valid downhill path before placing a source
                            Vector2 test_direction = find_downhill_direction(test_pos, height);
                            if (test_direction.length_squared() > 0.001f) {
                                // Score based on both noise value and height (prefer higher locations)
                                float noise_value = config->river_source_texture->get_noise()->get_noise_2d(x, z);
                                noise_value = (noise_value + 1.0f) * 0.5f; // Normalize to [0,1]
                                float score = noise_value * 0.7f + (height / 100.0f) * 0.3f; // Weight noise more than height

                                if (score > best_score) {
                                    best_score = score;
                                    best_position = test_pos;
                                    found_valid_spot = true;
                                }
                            }
                        }
                    }
                }
            }

            // If we found a valid spot in this grid cell, add it as a source
            if (found_valid_spot) {
                float height = height_sampler->sample_height(best_position.x, best_position.y);
                // Use grid coordinates to create a deterministic source ID
                int deterministic_id = grid_x * 10000 + grid_z;
                sources.push_back({best_position, height, deterministic_id});
            }
        }
    }

    return sources;
}


bool RiverGenerator::should_place_river_source(Vector2 world_pos) const {
    if (config->river_source_texture.is_null() || config->river_source_texture->get_noise().is_null()) {
        return false;
    }

    // Sample the noise texture at this world position
    float noise_value = config->river_source_texture->get_noise()->get_noise_2d(world_pos.x, world_pos.y);

    // Normalize from [-1, 1] to [0, 1]
    noise_value = (noise_value + 1.0f) * 0.5f;

    // Place source if noise value exceeds threshold
    return noise_value > SOURCE_THRESHOLD;
}


MeshInstance3D* RiverGenerator::create_debug_source_marker(const RiverSource& source) const {
    auto st = memnew(SurfaceTool);
    st->begin(Mesh::PRIMITIVE_TRIANGLES);

    // Create a simple pyramid/cone shape for the debug marker
    float size = 1.0f;

    // Bottom vertices (square base)
    st->set_color(Color(0.2f, 0.6f, 1.0f, 0.8f)); // Blue color for water sources
    st->set_normal(Vector3(0, -1, 0));
    st->add_vertex(Vector3(-size, 0, -size));
    st->add_vertex(Vector3(size, 0, -size));
    st->add_vertex(Vector3(size, 0, size));
    st->add_vertex(Vector3(-size, 0, size));

    // Top vertex (pyramid peak)
    st->set_color(Color(1.0f, 1.0f, 1.0f, 1.0f)); // White peak
    st->set_normal(Vector3(0, 1, 0));
    st->add_vertex(Vector3(0, size * 2, 0));

    // Pyramid faces
    // Face 1
    st->add_index(0); st->add_index(1); st->add_index(4);
    // Face 2
    st->add_index(1); st->add_index(2); st->add_index(4);
    // Face 3
    st->add_index(2); st->add_index(3); st->add_index(4);
    // Face 4
    st->add_index(3); st->add_index(0); st->add_index(4);

    // Base (optional, for better visibility)
    st->add_index(0); st->add_index(2); st->add_index(1);
    st->add_index(0); st->add_index(3); st->add_index(2);

    auto mesh = st->commit();
    memdelete(st);

    MeshInstance3D* mesh_instance = memnew(MeshInstance3D);
    mesh_instance->set_mesh(mesh);

    // Create a simple material for the debug marker
    Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);
    material->set_albedo(Color(0.2f, 0.6f, 1.0f, 0.8f));
    mesh_instance->set_material_override(material);

    return mesh_instance;
}

//==========================================
// Step 2: River Tracing Implementation
//==========================================

std::vector<RiverSegment> RiverGenerator::get_river_segments_for_chunk(Vector2i chunk_pos) const {
    std::vector<RiverSegment> segments;

    // Get all sources that might affect this chunk (larger radius for river tracing)
    int search_radius = 5; // Wider search since rivers can be long
    std::vector<RiverSource> sources = find_river_sources_in_region(chunk_pos, search_radius);

    for (const RiverSource& source : sources) {
        // Trace the river from this source
        RiverPath river = trace_river_from_source(source);

        // Extract segments that pass through this chunk
        std::vector<RiverSegment> chunk_segments = extract_segments_for_chunk(river, chunk_pos);
        segments.insert(segments.end(), chunk_segments.begin(), chunk_segments.end());
    }

    return segments;
}

std::vector<RiverSegment> RiverGenerator::get_river_segments_for_carving(Vector2i chunk_pos) const {
    std::vector<RiverSegment> segments;

    // Use a much larger search radius for carving to ensure consistent results across chunks
    // The radius should be large enough to capture all rivers that could possibly affect the chunk
    float max_carving_distance = BASE_RIVER_WIDTH * config->river_carving_width_multiplier;
    int chunks_per_carving_distance = static_cast<int>(std::ceil(max_carving_distance / config->width));
    int search_radius = std::max(10, chunks_per_carving_distance + 5); // At least 10 chunks, plus buffer

    std::vector<RiverSource> sources = find_river_sources_in_region(chunk_pos, search_radius);

    for (const RiverSource& source : sources) {
        // Trace the river from this source
        RiverPath river = trace_river_from_source(source);

        // Extract segments that could potentially affect this chunk (wider search)
        std::vector<RiverSegment> chunk_segments = extract_segments_for_carving(river, chunk_pos);
        segments.insert(segments.end(), chunk_segments.begin(), chunk_segments.end());
    }

    return segments;
}

std::vector<RiverSegment> RiverGenerator::get_river_segments_for_foliage(Vector2i chunk_pos) const {
    std::vector<RiverSegment> segments;

    // Use a moderate search radius for foliage exclusion - larger than regular chunk search
    // but smaller than carving search since foliage exclusion distance is typically smaller
    int search_radius = 8; // Should cover most foliage exclusion scenarios

    std::vector<RiverSource> sources = find_river_sources_in_region(chunk_pos, search_radius);

    for (const RiverSource& source : sources) {
        // Trace the river from this source
        RiverPath river = trace_river_from_source(source);

        // Extract segments that could potentially affect foliage in this chunk
        std::vector<RiverSegment> chunk_segments = extract_segments_for_carving(river, chunk_pos);
        segments.insert(segments.end(), chunk_segments.begin(), chunk_segments.end());
    }

    return segments;
}

RiverPath RiverGenerator::trace_river_from_source(const RiverSource& source) const {
    RiverPath river;
    river.source_id = source.source_id;
    river.reaches_sea_level = false;

    Vector2 current_pos = source.world_position;
    float current_height = source.height;
    float current_width = BASE_RIVER_WIDTH;
    Vector2 last_direction(0, 0); // Track direction for turn limiting

    // Add the source as the first point
    river.points.push_back({current_pos, current_height, current_width});

    int trace_count = 0;
    int stuck_count = 0;
    float current_search_radius = SEARCH_RADIUS;
    float tolerance_height_gain = 0.0f; // How much uphill we can tolerate

    while (trace_count < MAX_TRACE_POINTS && current_height > SEA_LEVEL) {
        // Find the best direction with current constraints
        Vector2 next_direction = find_best_river_direction(current_pos, current_height, current_search_radius, 
                                                          tolerance_height_gain, last_direction);

        // If no direction found, gradually relax constraints
        if (next_direction.length_squared() < 0.001f) {
            stuck_count++;
            
            // Progressively relax constraints
            if (stuck_count == 1) {
                // First attempt: increase search radius
                current_search_radius *= 2.0f;
            } else if (stuck_count == 2) {
                // Second attempt: allow very slight uphill
                tolerance_height_gain = config->river_uphill_tolerance * 0.3f;
            } else if (stuck_count == 3) {
                // Third attempt: allow more uphill but still within tolerance
                tolerance_height_gain = config->river_uphill_tolerance * 0.7f;
            } else if (stuck_count == 4) {
                // Fourth attempt: allow full uphill tolerance and larger search
                tolerance_height_gain = config->river_uphill_tolerance;
                current_search_radius *= 1.5f;
            } else if (stuck_count == 5) {
                // Fifth attempt: allow more uphill but with penalty
                tolerance_height_gain = config->river_uphill_tolerance * 1.5f;
            } else if (stuck_count >= config->river_max_stuck_attempts) {
                // Give up after configured attempts
                break;
            }
            
            // Try again with relaxed constraints
            next_direction = find_best_river_direction(current_pos, current_height, current_search_radius, 
                                                      tolerance_height_gain, last_direction);
            
            if (next_direction.length_squared() < 0.001f) {
                continue; // Try next iteration with further relaxed constraints
            }
        } else {
            // Success - reset constraints gradually
            stuck_count = 0;
            current_search_radius = std::max(SEARCH_RADIUS, current_search_radius * 0.95f);
            tolerance_height_gain = std::max(0.0f, tolerance_height_gain * 0.9f);
        }

        // Move to next position
        Vector2 next_pos = current_pos + next_direction.normalized() * TRACE_STEP;
        float next_height = height_sampler->sample_height(next_pos.x, next_pos.y);

        // Increase river width as it flows
        current_width += WIDTH_GROWTH_RATE * TRACE_STEP;

        // Add the new point
        river.points.push_back({next_pos, next_height, current_width});

        // Update for next iteration
        current_pos = next_pos;
        current_height = next_height;
        last_direction = next_direction.normalized();
        trace_count++;
    }

    // Check if we reached sea level
    if (current_height <= SEA_LEVEL) {
        river.reaches_sea_level = true;
    }

    return river;
}

Vector2 RiverGenerator::find_downhill_direction(Vector2 current_pos, float current_height) const {
    Vector2 best_direction(0, 0);
    float best_height_drop = 0.0f;
    const float MIN_MEANINGFUL_DROP = MIN_HEIGHT_DROP * 0.5f; // Require at least half the minimum drop

    // Sample points in a circle around current position
    const int NUM_SAMPLES = 8;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        float angle = (i * 2.0f * M_PI) / NUM_SAMPLES;
        Vector2 direction(cos(angle), sin(angle));
        Vector2 test_pos = current_pos + direction * SEARCH_RADIUS;

        float test_height = height_sampler->sample_height(test_pos.x, test_pos.y);
        float height_drop = current_height - test_height;

        // Only consider directions with meaningful downhill slope
        if (height_drop > best_height_drop && height_drop > MIN_MEANINGFUL_DROP) {
            best_height_drop = height_drop;
            best_direction = direction;
        }
    }

    return best_direction;
}

Vector2 RiverGenerator::find_downhill_direction_adaptive(Vector2 current_pos, float current_height, float search_radius) const {
    Vector2 best_direction(0, 0);
    float best_height_drop = 0.0f;
    const float MIN_MEANINGFUL_DROP = MIN_HEIGHT_DROP * 0.5f; // Require at least half the minimum drop

    // Calculate number of samples based on search radius (more samples for larger radius)
    int num_samples = std::max(8, static_cast<int>(search_radius / SEARCH_RADIUS * 8));
    num_samples = std::min(num_samples, 24); // Cap at 24 samples for performance

    // Sample points in a circle around current position
    for (int i = 0; i < num_samples; i++) {
        float angle = (i * 2.0f * M_PI) / num_samples;
        Vector2 direction(cos(angle), sin(angle));
        Vector2 test_pos = current_pos + direction * search_radius;

        float test_height = height_sampler->sample_height(test_pos.x, test_pos.y);
        float height_drop = current_height - test_height;

        // Only consider directions with meaningful downhill slope
        if (height_drop > best_height_drop && height_drop > MIN_MEANINGFUL_DROP) {
            best_height_drop = height_drop;
            best_direction = direction;
        }
    }

    // If no downhill direction found with circular sampling, try grid sampling with closer points
    if (best_height_drop <= MIN_MEANINGFUL_DROP && search_radius > SEARCH_RADIUS) {
        const float grid_step = search_radius / 4.0f; // Smaller steps for more precise sampling
        const int grid_range = 3; // Check 3 steps in each direction

        for (int z = -grid_range; z <= grid_range; z++) {
            for (int x = -grid_range; x <= grid_range; x++) {
                if (x == 0 && z == 0) continue; // Skip current position

                Vector2 test_pos = current_pos + Vector2(x * grid_step, z * grid_step);
                float test_height = height_sampler->sample_height(test_pos.x, test_pos.y);
                float height_drop = current_height - test_height;

                // Only consider meaningful downhill slopes
                if (height_drop > best_height_drop && height_drop > MIN_MEANINGFUL_DROP) {
                    best_height_drop = height_drop;
                    best_direction = Vector2(x, z).normalized();
                }
            }
        }
    }

    return best_direction;
}

Vector2 RiverGenerator::find_best_river_direction(Vector2 current_pos, float current_height, float search_radius, 
                                                 float tolerance_height_gain, Vector2 last_direction) const {
    Vector2 best_direction(0, 0);
    float best_score = -1000.0f; // Use scoring instead of just height drop
    const float MIN_MEANINGFUL_DROP = MIN_HEIGHT_DROP * 0.3f; // More lenient
    const float MAX_TURN_ANGLE = config->river_max_turn_angle * M_PI / 180.0f; // Convert to radians

    // Calculate number of samples based on search radius
    int num_samples = std::max(16, static_cast<int>(search_radius / SEARCH_RADIUS * 16));
    num_samples = std::min(num_samples, 32); // Cap for performance

    // Sample points in a circle around current position
    for (int i = 0; i < num_samples; i++) {
        float angle = (i * 2.0f * M_PI) / num_samples;
        Vector2 direction(cos(angle), sin(angle));
        
        // Check turn angle constraint
        if (last_direction.length_squared() > 0.001f) {
            float turn_angle = std::acos(std::max(-1.0f, std::min(1.0f, direction.dot(last_direction))));
            if (turn_angle > MAX_TURN_ANGLE) {
                continue; // Skip directions that would cause sharp turns
            }
        }
        
        Vector2 test_pos = current_pos + direction * search_radius;
        float test_height = height_sampler->sample_height(test_pos.x, test_pos.y);
        float height_change = current_height - test_height;

        // Calculate score based on multiple factors
        float score = 0.0f;
        
        // Prefer downhill, but allow some uphill within tolerance
        if (height_change >= -tolerance_height_gain) {
            if (height_change > 0) {
                // Downhill is best
                score += height_change * 10.0f;
            } else if (height_change >= -tolerance_height_gain) {
                // Uphill within tolerance - penalize but allow
                // More penalty for steeper uphill slopes
                float uphill_penalty = std::abs(height_change) / tolerance_height_gain;
                score -= uphill_penalty * 5.0f; // Stronger penalty for uphill
            }
            
            // Bonus for continuing in similar direction (momentum)
            if (last_direction.length_squared() > 0.001f) {
                float alignment = direction.dot(last_direction);
                score += alignment * 2.0f;
            }
            
            // Bonus for steeper downhill slopes
            if (height_change > MIN_MEANINGFUL_DROP) {
                score += 5.0f;
            }
            
            if (score > best_score) {
                best_score = score;
                best_direction = direction;
            }
        }
    }

    // If circular sampling failed, try grid sampling with more lenient constraints
    if (best_score < 0.0f && search_radius > SEARCH_RADIUS) {
        const float grid_step = search_radius / 5.0f;
        const int grid_range = 3;

        for (int z = -grid_range; z <= grid_range; z++) {
            for (int x = -grid_range; x <= grid_range; x++) {
                if (x == 0 && z == 0) continue;

                Vector2 direction = Vector2(x, z).normalized();
                
                // Check turn angle constraint
                if (last_direction.length_squared() > 0.001f) {
                    float turn_angle = std::acos(std::max(-1.0f, std::min(1.0f, direction.dot(last_direction))));
                    if (turn_angle > MAX_TURN_ANGLE) {
                        continue;
                    }
                }

                Vector2 test_pos = current_pos + Vector2(x * grid_step, z * grid_step);
                float test_height = height_sampler->sample_height(test_pos.x, test_pos.y);
                float height_change = current_height - test_height;

                // More lenient scoring for grid search
                float score = 0.0f;
                if (height_change >= -tolerance_height_gain) {
                    score = height_change;
                    
                    // Momentum bonus
                    if (last_direction.length_squared() > 0.001f) {
                        score += direction.dot(last_direction);
                    }
                    
                    if (score > best_score) {
                        best_score = score;
                        best_direction = direction;
                    }
                }
            }
        }
    }

    return best_direction;
}

std::vector<RiverSegment> RiverGenerator::extract_segments_for_chunk(const RiverPath& river, Vector2i chunk_pos) const {
    std::vector<RiverSegment> segments;

    if (river.points.size() < 2) {
        return segments;
    }

    // Convert consecutive points into segments and check if they intersect the chunk
    for (size_t i = 0; i < river.points.size() - 1; i++) {
        const RiverPoint& start_point = river.points[i];
        const RiverPoint& end_point = river.points[i + 1];

        RiverSegment segment;
        segment.start = start_point.world_position;
        segment.end = end_point.world_position;
        segment.start_height = start_point.height;
        segment.end_height = end_point.height;
        segment.width = (start_point.width + end_point.width) * 0.5f; // Average width
        segment.source_id = river.source_id;
        
        // Calculate uphill amount (positive if going uphill)
        float height_difference = end_point.height - start_point.height;
        segment.uphill_amount = std::max(0.0f, height_difference);

        // Check if this segment passes through or near the chunk
        if (segment_intersects_chunk(segment, chunk_pos)) {
            segments.push_back(segment);
        }
    }

    return segments;
}

std::vector<RiverSegment> RiverGenerator::extract_segments_for_carving(const RiverPath& river, Vector2i chunk_pos) const {
    std::vector<RiverSegment> segments;

    if (river.points.size() < 2) {
        return segments;
    }

    // Convert consecutive points into segments and check if they could affect carving in the chunk
    for (size_t i = 0; i < river.points.size() - 1; i++) {
        const RiverPoint& start_point = river.points[i];
        const RiverPoint& end_point = river.points[i + 1];

        RiverSegment segment;
        segment.start = start_point.world_position;
        segment.end = end_point.world_position;
        segment.start_height = start_point.height;
        segment.end_height = end_point.height;
        segment.width = (start_point.width + end_point.width) * 0.5f; // Average width
        segment.source_id = river.source_id;
        
        // Calculate uphill amount (positive if going uphill)
        float height_difference = end_point.height - start_point.height;
        segment.uphill_amount = std::max(0.0f, height_difference);

        // Check if this segment could potentially affect carving in the chunk
        if (segment_affects_chunk_carving(segment, chunk_pos)) {
            segments.push_back(segment);
        }
    }

    return segments;
}

bool RiverGenerator::segment_intersects_chunk(const RiverSegment& segment, Vector2i chunk_pos) const {
    // Calculate chunk boundaries in world coordinates
    float chunk_world_x = chunk_pos.x * config->width;
    float chunk_world_z = chunk_pos.y * config->width;
    float chunk_end_x = chunk_world_x + config->width;
    float chunk_end_z = chunk_world_z + config->width;

    // Add some margin for river width
    float margin = segment.width * 2.0f;
    chunk_world_x -= margin;
    chunk_world_z -= margin;
    chunk_end_x += margin;
    chunk_end_z += margin;

    // Simple bounding box intersection test
    float seg_min_x = std::min(segment.start.x, segment.end.x);
    float seg_max_x = std::max(segment.start.x, segment.end.x);
    float seg_min_z = std::min(segment.start.y, segment.end.y);
    float seg_max_z = std::max(segment.start.y, segment.end.y);

    return !(seg_max_x < chunk_world_x || seg_min_x > chunk_end_x ||
             seg_max_z < chunk_world_z || seg_min_z > chunk_end_z);
}

bool RiverGenerator::segment_affects_chunk_carving(const RiverSegment& segment, Vector2i chunk_pos) const {
    // Calculate chunk boundaries in world coordinates
    float chunk_world_x = chunk_pos.x * config->width;
    float chunk_world_z = chunk_pos.y * config->width;
    float chunk_end_x = chunk_world_x + config->width;
    float chunk_end_z = chunk_world_z + config->width;

    // Calculate the maximum carving distance for this segment
    float max_carving_distance = segment.width * config->river_carving_width_multiplier;
    
    // Expand chunk boundaries by the carving distance
    chunk_world_x -= max_carving_distance;
    chunk_world_z -= max_carving_distance;
    chunk_end_x += max_carving_distance;
    chunk_end_z += max_carving_distance;

    // Bounding box intersection test with carving distance
    float seg_min_x = std::min(segment.start.x, segment.end.x);
    float seg_max_x = std::max(segment.start.x, segment.end.x);
    float seg_min_z = std::min(segment.start.y, segment.end.y);
    float seg_max_z = std::max(segment.start.y, segment.end.y);

    return !(seg_max_x < chunk_world_x || seg_min_x > chunk_end_x ||
             seg_max_z < chunk_world_z || seg_min_z > chunk_end_z);
}

void RiverGenerator::add_debug_rivers_to_chunk(MeshInstance3D* chunk_mesh, Vector2i chunk_pos) const {
    if (!chunk_mesh) return;

    std::vector<RiverSegment> segments = get_river_segments_for_chunk(chunk_pos);

    float chunk_world_x = chunk_pos.x * config->width;
    float chunk_world_z = chunk_pos.y * config->width;

    for (const RiverSegment& segment : segments) {
        // Convert to local chunk coordinates
        Vector2 local_start(segment.start.x - chunk_world_x, segment.start.y - chunk_world_z);
        Vector2 local_end(segment.end.x - chunk_world_x, segment.end.y - chunk_world_z);

        // Create debug visualization for this segment
        MeshInstance3D* debug_segment = create_debug_river_segment(segment);

        // Position at the midpoint of the segment
        Vector2 midpoint = (local_start + local_end) * 0.5f;
        float avg_height = (segment.start_height + segment.end_height) * 0.5f;
        debug_segment->set_position(Vector3(midpoint.x, avg_height + 1.0f, midpoint.y));

        chunk_mesh->add_child(debug_segment);
    }
}

MeshInstance3D* RiverGenerator::create_debug_river_segment(const RiverSegment& segment) const {
    auto st = memnew(SurfaceTool);
    st->begin(Mesh::PRIMITIVE_TRIANGLES);

    // Create a simple cylinder/capsule shape for the river segment
    Vector2 direction = (segment.end - segment.start).normalized();
    Vector2 perpendicular(-direction.y, direction.x);

    float length = (segment.end - segment.start).length();
    float half_width = segment.width * 0.5f;

    // Create a simple quad for the river segment
    st->set_color(Color(0.3f, 0.7f, 1.0f, 0.6f)); // Light blue for rivers
    st->set_normal(Vector3(0, 1, 0));

    // Quad vertices (in local space relative to segment start)
    Vector2 v1 = -perpendicular * half_width;
    Vector2 v2 = perpendicular * half_width;
    Vector2 v3 = direction * length + perpendicular * half_width;
    Vector2 v4 = direction * length - perpendicular * half_width;

    st->add_vertex(Vector3(v1.x, 0, v1.y));
    st->add_vertex(Vector3(v2.x, 0, v2.y));
    st->add_vertex(Vector3(v3.x, 0, v3.y));
    st->add_vertex(Vector3(v4.x, 0, v4.y));

    // Two triangles to form the quad (counter-clockwise winding)
    st->add_index(0); st->add_index(2); st->add_index(1);
    st->add_index(0); st->add_index(3); st->add_index(2);

    auto mesh = st->commit();
    memdelete(st);

    MeshInstance3D* mesh_instance = memnew(MeshInstance3D);
    mesh_instance->set_mesh(mesh);

    // Create a material for the river segment
    Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);
    material->set_albedo(Color(0.3f, 0.7f, 1.0f, 0.6f));
    material->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
    mesh_instance->set_material_override(material);

    return mesh_instance;
}

// ========== Advanced River Mesh Generation ==========

void RiverGenerator::add_river_meshes_to_chunk(MeshInstance3D* chunk_mesh, Vector2i chunk_pos) const {
    if (!chunk_mesh || !config->enable_river_mesh) return;

    // Find all river sources in a much larger area to catch rivers that pass through this chunk
    // but originate elsewhere
    std::vector<RiverSource> sources = find_river_sources_in_region(chunk_pos, 8); // Increased search radius
    
    float chunk_world_x = chunk_pos.x * config->width;
    float chunk_world_z = chunk_pos.y * config->width;
    float chunk_end_x = chunk_world_x + config->width;
    float chunk_end_z = chunk_world_z + config->width;

    for (const RiverSource& source : sources) {
        // Trace the complete river path for this source
        RiverPath river = trace_river_from_source(source);
        if (river.points.size() < 2) continue;

        // Check if this river passes through or near this chunk
        // Use larger margin to catch rivers that affect the chunk
        float influence_margin = config->width; // Full chunk width as margin
        
        bool river_affects_chunk = false;
        for (const auto& point : river.points) {
            if (point.world_position.x >= chunk_world_x - influence_margin && 
                point.world_position.x <= chunk_end_x + influence_margin &&
                point.world_position.y >= chunk_world_z - influence_margin && 
                point.world_position.y <= chunk_end_z + influence_margin) {
                river_affects_chunk = true;
                break;
            }
        }
        
        if (!river_affects_chunk) {
            continue; // This river doesn't affect this chunk
        }

        // Create one continuous mesh for the entire river path
        MeshInstance3D* river_mesh = create_continuous_river_mesh(river, chunk_pos);
        if (river_mesh) {
            // Position at chunk origin since we're using world coordinates
            river_mesh->set_position(Vector3(-chunk_world_x, 0.0f, -chunk_world_z));
            chunk_mesh->add_child(river_mesh);
        }
    }
}

MeshInstance3D* RiverGenerator::create_river_mesh_for_segment(const RiverSegment& segment) const {
    Ref<ArrayMesh> river_mesh = generate_river_geometry(segment);
    if (river_mesh.is_null()) {
        return nullptr;
    }

    MeshInstance3D* mesh_instance = memnew(MeshInstance3D);
    mesh_instance->set_mesh(river_mesh);

    // Apply water material if available, otherwise use default
    if (!config->river_material.is_null()) {
        mesh_instance->set_material_override(config->river_material);
    } else {
        // Create a basic water-like material as fallback
        Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);
        material->set_albedo(Color(0.2f, 0.6f, 0.9f, 0.8f));
        material->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
        material->set_roughness(0.1f);
        material->set_metallic(0.0f);
        mesh_instance->set_material_override(material);
    }

    return mesh_instance;
}

Ref<ArrayMesh> RiverGenerator::generate_river_geometry(const RiverSegment& segment) const {
    if (segment.start.distance_to(segment.end) < 0.1f) {
        return Ref<ArrayMesh>(); // Skip degenerate segments
    }

    auto st = memnew(SurfaceTool);
    st->begin(Mesh::PRIMITIVE_TRIANGLES);

    // Calculate segment properties
    Vector2 direction = (segment.end - segment.start).normalized();
    Vector2 perpendicular(-direction.y, direction.x);
    float length = segment.start.distance_to(segment.end);
    
    // Calculate water surface heights (flat along each segment)
    float start_terrain_height = height_sampler->sample_height(segment.start.x, segment.start.y);
    float end_terrain_height = height_sampler->sample_height(segment.end.x, segment.end.y);
    
    // Water surface sits at carved terrain height + offset
    float start_water_height = start_terrain_height + config->river_mesh_depth_offset;
    float end_water_height = end_terrain_height + config->river_mesh_depth_offset;
    
    // Create simple flat water surface
    int subdivisions = config->river_mesh_subdivisions;
    float step_length = length / subdivisions;
    
    // Generate vertices for flat water surface
    PackedVector3Array vertices;
    PackedVector3Array normals;
    PackedVector2Array uvs;
    PackedInt32Array indices;
    
    // Simple approach: create a flat ribbon following the river path
    for (int i = 0; i <= subdivisions; i++) {
        float t = (float)i / subdivisions;
        Vector2 pos_along_river = segment.start + direction * (t * length);
        
        // Interpolate water height along the segment (allows for gentle slope)
        float water_height = start_water_height + t * (end_water_height - start_water_height);
        
        // Create two vertices across the river width (left and right bank)
        float half_width = segment.width * 0.5f;
        
        Vector2 left_pos = pos_along_river + perpendicular * half_width;
        Vector2 right_pos = pos_along_river - perpendicular * half_width;
        
        // Convert to local coordinates (relative to segment start)
        Vector3 left_vertex = Vector3(left_pos.x - segment.start.x, water_height, left_pos.y - segment.start.y);
        Vector3 right_vertex = Vector3(right_pos.x - segment.start.x, water_height, right_pos.y - segment.start.y);
        
        vertices.push_back(left_vertex);
        vertices.push_back(right_vertex);
        
        // Water surface normals point straight up
        normals.push_back(Vector3(0, 1, 0));
        normals.push_back(Vector3(0, 1, 0));
        
        // UV coordinates for water shader (flow direction along x, width along y)
        uvs.push_back(Vector2(t, 0.0f));  // Left bank
        uvs.push_back(Vector2(t, 1.0f));  // Right bank
    }
    
    // Generate triangles for the flat water ribbon
    for (int i = 0; i < subdivisions; i++) {
        int base_idx = i * 2;
        int next_base_idx = (i + 1) * 2;
        
        // Create two triangles for each quad
        // Triangle 1: left-bottom, right-bottom, left-top
        indices.push_back(base_idx);          // left current
        indices.push_back(base_idx + 1);      // right current  
        indices.push_back(next_base_idx);     // left next
        
        // Triangle 2: right-bottom, right-top, left-top
        indices.push_back(base_idx + 1);      // right current
        indices.push_back(next_base_idx + 1); // right next
        indices.push_back(next_base_idx);     // left next
    }
    
    // Add geometry to surface tool
    for (int i = 0; i < vertices.size(); i++) {
        st->set_normal(normals[i]);
        st->set_uv(uvs[i]);
        st->add_vertex(vertices[i]);
    }
    
    for (int i = 0; i < indices.size(); i++) {
        st->add_index(indices[i]);
    }
    
    auto mesh = st->commit();
    memdelete(st);
    
    return mesh;
}

MeshInstance3D* RiverGenerator::create_continuous_river_mesh(const RiverPath& river, Vector2i chunk_pos) const {
    Ref<ArrayMesh> river_mesh = generate_continuous_river_geometry(river, chunk_pos);
    if (river_mesh.is_null()) {
        return nullptr;
    }

    MeshInstance3D* mesh_instance = memnew(MeshInstance3D);
    mesh_instance->set_mesh(river_mesh);

    // Apply water material if available, otherwise use default
    if (!config->river_material.is_null()) {
        mesh_instance->set_material_override(config->river_material);
    } else {
        // Create a basic water-like material as fallback
        Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);
        material->set_albedo(Color(0.2f, 0.6f, 0.9f, 0.8f));
        material->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
        material->set_roughness(0.1f);
        material->set_metallic(0.0f);
        mesh_instance->set_material_override(material);
    }

    return mesh_instance;
}

Ref<ArrayMesh> RiverGenerator::generate_continuous_river_geometry(const RiverPath& river, Vector2i chunk_pos) const {
    if (river.points.size() < 2) {
        return Ref<ArrayMesh>(); // Need at least 2 points
    }

    auto st = memnew(SurfaceTool);
    st->begin(Mesh::PRIMITIVE_TRIANGLES);

    // Calculate chunk boundaries for reference (but don't use for strict clipping)
    float chunk_world_x = chunk_pos.x * config->width;
    float chunk_world_z = chunk_pos.y * config->width;
    float chunk_end_x = chunk_world_x + config->width;
    float chunk_end_z = chunk_world_z + config->width;
    
    // Use a much larger margin to ensure continuous rivers
    float margin = config->width * 2.0f; // Much larger margin for better continuity
    float extended_start_x = chunk_world_x - margin;
    float extended_start_z = chunk_world_z - margin;
    float extended_end_x = chunk_end_x + margin;
    float extended_end_z = chunk_end_z + margin;

    PackedVector3Array vertices;
    PackedVector3Array normals;
    PackedVector2Array uvs;
    PackedInt32Array indices;

    int vertex_count = 0;
    float total_length = 0.0f;

    // Generate vertices along the entire river path - don't skip points
    for (size_t i = 0; i < river.points.size(); i++) {
        const RiverPoint& point = river.points[i];
        
        // Only check if ANY part of this river segment could affect the chunk
        // This is much more lenient than before
        bool should_include_point = false;
        
        // Include if point is within extended bounds
        if (point.world_position.x >= extended_start_x && point.world_position.x <= extended_end_x &&
            point.world_position.y >= extended_start_z && point.world_position.y <= extended_end_z) {
            should_include_point = true;
        }
        
        // Also include if the river segment could potentially affect the chunk
        if (!should_include_point && i > 0) {
            const RiverPoint& prev_point = river.points[i - 1];
            // Check if line segment between prev_point and point intersects with chunk area
            float seg_min_x = std::min(prev_point.world_position.x, point.world_position.x);
            float seg_max_x = std::max(prev_point.world_position.x, point.world_position.x);
            float seg_min_z = std::min(prev_point.world_position.y, point.world_position.y);
            float seg_max_z = std::max(prev_point.world_position.y, point.world_position.y);
            
            // Check intersection with chunk bounds (with margin)
            if (!(seg_max_x < chunk_world_x - margin || seg_min_x > chunk_end_x + margin ||
                  seg_max_z < chunk_world_z - margin || seg_min_z > chunk_end_z + margin)) {
                should_include_point = true;
            }
        }
        
        if (!should_include_point) {
            continue; // Skip this point but don't break continuity tracking
        }

        // Calculate direction for this point
        Vector2 direction;
        if (i == 0 && i + 1 < river.points.size()) {
            // First point - use direction to next point
            direction = (river.points[i + 1].world_position - point.world_position).normalized();
        } else if (i == river.points.size() - 1 && i > 0) {
            // Last point - use direction from previous point
            direction = (point.world_position - river.points[i - 1].world_position).normalized();
        } else if (i > 0 && i + 1 < river.points.size()) {
            // Middle point - average of incoming and outgoing directions
            Vector2 dir_in = (point.world_position - river.points[i - 1].world_position).normalized();
            Vector2 dir_out = (river.points[i + 1].world_position - point.world_position).normalized();
            direction = (dir_in + dir_out).normalized();
        }

        Vector2 perpendicular(-direction.y, direction.x);
        
        // Use configurable river mesh width multiplier
        float river_width = point.width * config->river_mesh_width_multiplier;
        float half_width = river_width * 0.5f;

        // Sample the carved terrain height at multiple points for better alignment
        float terrain_height = height_sampler->sample_height(point.world_position.x, point.world_position.y);
        
        // Also sample at the left and right edges to get a better sense of the carved area
        Vector2 left_sample_pos = point.world_position + perpendicular * (half_width * 0.8f);
        Vector2 right_sample_pos = point.world_position - perpendicular * (half_width * 0.8f);
        float left_terrain_height = height_sampler->sample_height(left_sample_pos.x, left_sample_pos.y);
        float right_terrain_height = height_sampler->sample_height(right_sample_pos.x, right_sample_pos.y);
        
        // Use the minimum height to ensure water doesn't float above any carved area
        float min_terrain_height = std::min({terrain_height, left_terrain_height, right_terrain_height});
        
        // Place water surface below the lowest carved terrain point with safety margin
        float water_height = min_terrain_height + config->river_mesh_depth_offset - config->river_mesh_bank_safety;

        // Create left and right bank vertices
        Vector2 left_pos = point.world_position + perpendicular * half_width;
        Vector2 right_pos = point.world_position - perpendicular * half_width;

        // Add vertices (in world coordinates)
        vertices.push_back(Vector3(left_pos.x, water_height, left_pos.y));
        vertices.push_back(Vector3(right_pos.x, water_height, right_pos.y));

        // Water surface normals point straight up
        normals.push_back(Vector3(0, 1, 0));
        normals.push_back(Vector3(0, 1, 0));

        // UV coordinates - flow along the river, width across
        float u_coord = total_length / 100.0f; // Scale UV for tiling
        uvs.push_back(Vector2(u_coord, 0.0f));  // Left bank
        uvs.push_back(Vector2(u_coord, 1.0f));  // Right bank

        // Create triangles connecting to previous point (only if we have previous vertices)
        if (vertex_count > 0) {
            int current_left = vertex_count;
            int current_right = vertex_count + 1;
            int prev_left = vertex_count - 2;
            int prev_right = vertex_count - 1;

            // Fix winding order for correct face orientation (counter-clockwise when viewed from above)
            // Triangle 1: prev_left, prev_right, current_left
            indices.push_back(prev_left);
            indices.push_back(prev_right);
            indices.push_back(current_left);

            // Triangle 2: prev_right, current_right, current_left
            indices.push_back(prev_right);
            indices.push_back(current_right);
            indices.push_back(current_left);
        }

        vertex_count += 2;
        
        // Calculate distance for UV mapping
        if (i > 0) {
            total_length += point.world_position.distance_to(river.points[i - 1].world_position);
        }
    }

    if (vertices.size() < 4) {
        memdelete(st);
        return Ref<ArrayMesh>(); // Need at least 2 points (4 vertices) for a valid mesh
    }

    // Add geometry to surface tool
    for (int i = 0; i < vertices.size(); i++) {
        st->set_normal(normals[i]);
        st->set_uv(uvs[i]);
        st->add_vertex(vertices[i]);
    }

    for (int i = 0; i < indices.size(); i++) {
        st->add_index(indices[i]);
    }

    auto mesh = st->commit();
    memdelete(st);

    return mesh;
}