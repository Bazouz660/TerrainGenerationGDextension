extends MeshInstance3D

@export var camera: Camera3D

func _process(_delta):
    if camera:
        var camera_position = camera.global_transform.origin
        position.x = camera_position.x
        position.z = camera_position.z