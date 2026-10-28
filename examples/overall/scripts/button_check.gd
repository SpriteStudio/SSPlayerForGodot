extends Control

# Visual check for SpriteStudioButton. Builds its own buttons and HUD, so the
# scene only has to hold this script. Run the scene, move the pointer over the
# two Ringos and use the keys the HUD lists. The grey frames are the buttons'
# rects, laid out by containers that follow the window; the red outlines are
# the parts the left button takes the pointer on, as get_part_quad() reports
# them.

const RINGO := "res://ssab_generated/Ringo/Ringo.ssab"
const CLIPS := {
	"normal": {"animation": "wait1"},
	"hover": {"animation": "walk"},
	"pressed": {"animation": "attack1"},
	"disabled": {"animation": "dead"},
	"focused": {"animation": "jump_wait"},
	"selected_normal": {"animation": "joy"},
	"selected_hover": {"animation": "dance"},
}
const HIT_AREAS := [
	["the whole rect", [], false],
	["any part", [], true],
	["the apple", ["apple"], false],
	["head and body", ["head", "body"], false],
]
const STRETCH_NAMES := ["Fit", "Cover", "Keep"]

var press_button: SpriteStudioButton
var toggle_button: SpriteStudioButton
var hud: Label
var overlay: Control
var hit_index := 0
var presses := 0
var toggles := 0


func _ready() -> void:
	if not ResourceLoader.exists(RINGO):
		push_error("%s is not here -- run scripts/deploy-examples first." % RINGO)
		return

	var column := VBoxContainer.new()
	column.set_anchors_and_offsets_preset(PRESET_FULL_RECT)
	add_child(column)
	hud = Label.new()
	column.add_child(hud)
	var row := HBoxContainer.new()
	row.size_flags_vertical = SIZE_EXPAND_FILL
	column.add_child(row)

	press_button = _make_button("Press")
	press_button.pressed.connect(func(): presses += 1)
	row.add_child(press_button)
	toggle_button = _make_button("Toggle")
	toggle_button.toggle_mode = true
	toggle_button.toggled.connect(func(_on): toggles += 1)
	row.add_child(toggle_button)

	overlay = Control.new()
	overlay.set_anchors_and_offsets_preset(PRESET_FULL_RECT)
	overlay.mouse_filter = MOUSE_FILTER_IGNORE
	overlay.draw.connect(_draw_overlay)
	add_child(overlay)
	# Redrawn once the frame's poses are final, so the outlines do not trail
	# the artwork by a frame.
	press_button.get_child(0).frame_updated.connect(func(_frame): overlay.queue_redraw())
	_apply_hit_area()


func _make_button(label: String) -> SpriteStudioButton:
	var button := SpriteStudioButton.new()
	button.name = label
	button.accessibility_name = label
	button.size_flags_horizontal = SIZE_EXPAND_FILL
	button.size_flags_vertical = SIZE_EXPAND_FILL
	button.state_clips = CLIPS
	var player := SpriteStudioPlayer2D.new()
	player.set_ssab_resource(load(RINGO))
	player.set_animation("wait1")
	button.add_child(player)
	return button


func _apply_hit_area() -> void:
	press_button.hit_parts = PackedStringArray(HIT_AREAS[hit_index][1])
	press_button.hit_on_artwork = HIT_AREAS[hit_index][2]


func _unhandled_input(event: InputEvent) -> void:
	var key := event as InputEventKey
	if key == null or not key.pressed or key.echo:
		return
	match key.keycode:
		KEY_H:
			hit_index = (hit_index + 1) % HIT_AREAS.size()
			_apply_hit_area()
		KEY_D:
			for button in [press_button, toggle_button]:
				button.disabled = not button.disabled
		KEY_R:
			for button in [press_button, toggle_button]:
				button.reduced_motion = not button.reduced_motion
		KEY_C:
			for button in [press_button, toggle_button]:
				button.clip_contents = not button.clip_contents
		KEY_S:
			for button in [press_button, toggle_button]:
				button.stretch_mode = (button.stretch_mode + 1) % STRETCH_NAMES.size()


func _process(_delta: float) -> void:
	if hud == null:
		return
	hud.text = "\n".join([
		"H hit area: %s   D disabled: %s   R reduced motion: %s   C clip contents: %s   S stretch: %s   Tab / Space: focus and press" % [
			HIT_AREAS[hit_index][0], press_button.disabled, press_button.reduced_motion,
			press_button.clip_contents, STRETCH_NAMES[press_button.stretch_mode]],
		"Press: %s   pressed %d times" % [_playing(press_button), presses],
		"Toggle (%s): %s   toggled %d times" % ["on" if toggle_button.button_pressed else "off", _playing(toggle_button), toggles],
	])


func _playing(button: SpriteStudioButton) -> String:
	var player: SpriteStudioPlayer2D = button.get_child(0)
	return "%s%s frame %d" % [player.get_current_animation(), "" if player.is_playing() else " (held)", int(player.get_frame_no())]


func _draw_overlay() -> void:
	for button in [press_button, toggle_button]:
		overlay.draw_rect(Rect2(button.global_position - overlay.global_position, button.size), Color(1, 1, 1, 0.35), false)
	var player: SpriteStudioPlayer2D = press_button.get_child(0)
	var parts: PackedStringArray = press_button.hit_parts
	if parts.is_empty() and press_button.hit_on_artwork:
		parts = player.get_part_names()
	var to_overlay := overlay.get_global_transform().affine_inverse() * player.get_global_transform()
	for part in parts:
		var quad := player.get_part_quad(part)
		if quad.size() == 4:
			quad.append(quad[0])
			overlay.draw_polyline(to_overlay * quad, Color.RED, 1.0)
