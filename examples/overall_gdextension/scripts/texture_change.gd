extends Node2D

# Example script demonstrating how to change textures (CellMap Overrides) 
# at runtime in v2.x.

@export var to_texture: Texture2D
@export var change: bool = false

# Assuming this script is attached to a parent node of SpriteStudioPlayer2D
@onready var ss_player: SpriteStudioPlayer2D = $SpriteStudioPlayer2D

var prev_change = change
var target_cellmap = ""
var time_passed = 0.0

func _ready():
	change = false
	prev_change = false
	if to_texture:
		print("to_texture: " + to_texture.resource_path)
	
	if ss_player.ssab:
		var cellmaps = ss_player.ssab.get_cellmap_names()
		if cellmaps.size() > 0:
			target_cellmap = cellmaps[0]
			print("Target cellmap for override: ", target_cellmap)

	ss_player.set_frame(0)
	ss_player.set_loop_count(-1) # -1 = infinite loop
	ss_player.play()

func _process(delta):
	# Automatically toggle every 2 seconds for demonstration purposes
	time_passed += delta
	if time_passed > 2.0:
		time_passed = 0.0
		change = !change
		
	if change != prev_change:
		if change:
			if target_cellmap != "":
				ss_player.set_cellmap_texture(target_cellmap, to_texture)
				print("Texture changed to override.")
		else:
			if target_cellmap != "":
				ss_player.set_cellmap_texture(target_cellmap, null)
				print("Texture restored to original.")
			
		prev_change = change
