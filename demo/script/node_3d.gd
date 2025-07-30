extends Node

var debug_view_mode: int = 0

var debug_view_mode_dict = {
    0: Viewport.DEBUG_DRAW_DISABLED,
    1: Viewport.DEBUG_DRAW_NORMAL_BUFFER,
    2: Viewport.DEBUG_DRAW_OVERDRAW,
    3: Viewport.DEBUG_DRAW_OCCLUDERS,
    4: Viewport.DEBUG_DRAW_WIREFRAME
}

@export var terrain_generator: TerrainGenerator

func _input(_event):
    if Input.is_action_just_pressed("cycle_debug_view"):
        cycle_debug_view_mode()
    if Input.is_action_just_pressed("reload_chunks"):
        print("Reloading chunks...")
        terrain_generator.reload_chunks()

func cycle_debug_view_mode():
    debug_view_mode += 1
    if debug_view_mode >= debug_view_mode_dict.size():
        debug_view_mode = 0
    get_viewport().debug_draw = debug_view_mode_dict[debug_view_mode]
