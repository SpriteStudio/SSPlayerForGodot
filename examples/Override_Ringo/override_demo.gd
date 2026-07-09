extends SpriteStudioPlayer2D
## Override Layer API (Phase 2) demo — runs on the Ringo sample.
##
## Open this project and press Play. The Ringo animation keeps playing while the
## demo cycles through the three per-part runtime overrides and shows the current
## step on screen. Each override wins over the keyframed animation underneath.
##
## API used (all on SpriteStudioPlayer2D):
##   set_part_color_override(part, color, blend_op=0, priority=1)
##   set_part_visibility_override(part, force_hidden, cascade=false)
##   set_part_cell_override(part, cellmap, cell, priority=1)
##   clear_all_part_overrides()
## Cell names can be discovered from the resource:
##   ssab.get_cellmap_names() / ssab.get_cell_names(cellmap)

var _label: Label

func _ready() -> void:
	# Ensure the animation is playing: cell/frame overrides only become visible
	# while the player advances frames, so start playback here rather than relying
	# on the scene's autoplay flag being set.
	play()
	# Auto-place Ringo at a visible spot ONLY if the scene left it at the origin
	# (0,0) — which sits at the top-left corner at runtime. If you move it in the
	# editor (Transform), that position wins and the runtime won't override it.
	if position == Vector2.ZERO:
		position = get_viewport_rect().size * 0.5 + Vector2(0, 250)
	_make_label()
	_run_demo()

func _make_label() -> void:
	var layer := CanvasLayer.new()
	add_child(layer)
	_label = Label.new()
	_label.position = Vector2(16, 16)
	_label.add_theme_font_size_override("font_size", 22)
	_label.add_theme_color_override("font_color", Color.WHITE)
	_label.add_theme_color_override("font_outline_color", Color.BLACK)
	_label.add_theme_constant_override("outline_size", 6)
	layer.add_child(_label)

func _step(text: String, secs: float) -> void:
	_label.text = text
	print(text)
	await get_tree().create_timer(secs).timeout

func _run_demo() -> void:
	# Discover what the loaded SSAB offers (also handy as a reference in Output).
#	print("parts:    ", get_part_names())
	var cellmap: String = ssab.get_cellmap_names()[0] # "Ringo"
#	print("cellmap:  ", cellmap)
#	print("cells:    ", ssab.get_cell_names(cellmap))

	# Since the same part is accessed repeatedly, identify it by part-ID
	# rather than its name (accessing by ID is slightly faster).
	var part_id := get_part_index("apple")

	while true:
		clear_all_part_overrides()
		await _step("Ringo Override Demo — no overrides", 1.5)

		# 1) Color — tint the body red (Normal-only; priority 1 = until next animation,
		#    so it survives animation loops).

#		set_part_color_override("apple", Color(0.0, 0.2, 1.0, 0.75))			# Access by name.
		set_part_color_override_by_index(part_id, Color(0.0, 0.2, 1.0, 0.75))	# Access by id.
		await _step("1) Color override:  body -> red", 2.0)

		# 2) Visibility — force-hide the 'apple' and cascade to its children (the whole face).
		clear_all_part_overrides()
#		set_part_visibility_override("apple", true, true)			# Access by name.
		set_part_visibility_override_by_index(part_id, true, false)	# Access by id.
		set_part_visibility_override("heta", true, false)
		await _step("2) Visibility override:  hide 'apple' with cascade ('heta' disappears)", 2.0)

		# 3) Cell — swap the 'apple' sprite to the 'effect3' cell in the Ringo cellmap.
		clear_all_part_overrides()
#		set_part_cell_override("apple", cellmap, "effect3")				# Access by name.
		set_part_cell_override_by_index(part_id, cellmap, "effect3")	# Access by id.
		set_part_visibility_override("heta", true, false)
		await _step("3) Cell override:  body sprite -> 'effect3' cell", 2.0)

		clear_all_part_overrides()
		await _step("4) No override (Restored)", 2.0)
