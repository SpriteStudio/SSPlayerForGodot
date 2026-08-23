extends Node2D

@onready var player: SpriteStudioPlayer2D = $Ringo
@onready var label: Label = $Label

var anims = ["attack1", "walk", "wait"]
var current_anim_index = 0

func _ready() -> void:
	# Connect to SpriteStudioPlayer2D signals to demonstrate script control
	player.animation_started.connect(_on_animation_started)
	player.animation_finished.connect(_on_animation_finished)
	player.animation_looped.connect(_on_animation_looped)
	player.signal_emitted.connect(_on_signal_emitted)
	player.user_data.connect(_on_user_data)
	
	play_current()
	update_label()

func _input(event: InputEvent) -> void:
	# Change animation on Left Click or Enter / Space
	if event.is_action_pressed("ui_accept") or (event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT):
		current_anim_index = (current_anim_index + 1) % anims.size()
		play_current()
		update_label()
		
	# Flip horizontally on Right Arrow
	elif event.is_action_pressed("ui_right"):
		player.scale.x *= -1
		
	# Adjust playback speed on Up/Down Arrows
	elif event.is_action_pressed("ui_up"):
		player.speed_scale += 0.5
		update_label()
	elif event.is_action_pressed("ui_down"):
		player.speed_scale = max(0.1, player.speed_scale - 0.5)
		update_label()

func play_current() -> void:
	var anim_name = anims[current_anim_index]
	player.animation = anim_name
	player.play()

func update_label() -> void:
	if label:
		label.text = "--- SpriteStudioPlayer2D GDScript Example ---\n\n"
		label.text += "Click or Press Enter/Space: Change Animation\n"
		label.text += "Up/Down Arrows: Change Speed\n"
		label.text += "Right Arrow: Flip Horizontal\n\n"
		label.text += "Current Animation: " + player.animation + "\n"
		label.text += "Speed Scale: " + str(player.speed_scale) + "\n"

func _on_animation_started(anim_name: String) -> void:
	print("Animation started: ", anim_name)

func _on_animation_finished(anim_name: String) -> void:
	print("Animation finished: ", anim_name)

func _on_animation_looped(anim_name: String) -> void:
	print("Animation looped: ", anim_name)

func _on_signal_emitted(command: String, value: Dictionary, info: Dictionary) -> void:
	# `value` is keyed by the parameter ids authored on the Signal key; `info`
	# says which part fired it and on which frame.
	print("Signal emitted: ", command, " value: ", value, " from part: ", info["part_name"])

func _on_user_data(payload: Dictionary) -> void:
	print("User data: ", payload)

