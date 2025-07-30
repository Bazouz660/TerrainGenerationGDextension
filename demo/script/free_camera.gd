extends Camera3D
class_name FreeCamera

# This script allows for free camera movement in a 3D space.

# Movement settings
@export var movement_speed: float = 5.0
@export var sprint_speed: float = 10.0
@export var mouse_sensitivity: float = 0.002
@export var smooth_movement: bool = true
@export var smooth_factor: float = 10.0

# Internal variables
var velocity: Vector3 = Vector3.ZERO
var mouse_captured: bool = false
var rotation_x: float = 0.0

func _ready():
	# Capture mouse on start
	capture_mouse()

func _input(event):
	# Handle mouse capture toggle
	if event.is_action_pressed("ui_cancel"):
		toggle_mouse_capture()

	# Handle mouse look
	if event is InputEventMouseMotion and mouse_captured:
		handle_mouse_look(event)

func _process(delta):
	# Handle movement
	handle_movement(delta)

func capture_mouse():
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
	mouse_captured = true

func release_mouse():
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	mouse_captured = false

func toggle_mouse_capture():
	if mouse_captured:
		release_mouse()
	else:
		capture_mouse()

func handle_mouse_look(event: InputEventMouseMotion):
	# Rotate horizontally
	rotate_y(-event.relative.x * mouse_sensitivity)

	# Rotate vertically
	rotation_x -= event.relative.y * mouse_sensitivity
	rotation_x = clamp(rotation_x, -PI / 2, PI / 2)
	rotation.x = rotation_x

func handle_movement(delta):
	var input_vector = Vector3.ZERO
	var current_speed = movement_speed

	# Check for sprint
	if Input.is_action_pressed("camera_sprint"): # Usually Space
		current_speed = sprint_speed

	# Get input directions
	if Input.is_action_pressed("camera_forward"): # W
		input_vector -= transform.basis.z
	if Input.is_action_pressed("camera_backward"): # S
		input_vector += transform.basis.z
	if Input.is_action_pressed("camera_left"): # A
		input_vector -= transform.basis.x
	if Input.is_action_pressed("camera_right"): # D
		input_vector += transform.basis.x

	# Add vertical movement
	if Input.is_action_pressed("camera_up"): # Usually Enter - move up
		input_vector += Vector3.UP
	if Input.is_action_pressed("camera_down") and mouse_captured: # Move down when mouse is captured
		input_vector -= Vector3.UP

	# Normalize input to prevent faster diagonal movement
	if input_vector.length() > 0:
		input_vector = input_vector.normalized()

	# Apply movement
	var target_velocity = input_vector * current_speed

	if smooth_movement:
		velocity = velocity.lerp(target_velocity, smooth_factor * delta)
	else:
		velocity = target_velocity

	# Move the camera
	position += velocity * delta
