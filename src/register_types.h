#ifndef TERRAIN_GENERATOR_REGISTER_TYPES_H
#define TERRAIN_GENERATOR_REGISTER_TYPES_H

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_terrain_generator_module(ModuleInitializationLevel p_level);
void uninitialize_terrain_generator_module(ModuleInitializationLevel p_level);

#endif // TERRAIN_GENERATOR_REGISTER_TYPES_H