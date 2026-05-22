extends Node2D

# Example script demonstrating how to change textures (CellMap Overrides) 
# at runtime in v2.x.

@export var to_texture: Texture2D
@export var change: bool = false

# Assuming this script is attached to a parent node of SpriteStudioPlayer2D
@onready var ss_player: SpriteStudioPlayer2D = $SpriteStudioPlayer2D

var prev_change = change

func _ready():
	change = false
	if to_texture:
		print("to_texture: " + to_texture.resource_path)
	
	ss_player.set_frame(0)
	ss_player.set_loop_count(-1) # -1 = infinite loop
	ss_player.play()

func _process(delta):
	if change != prev_change:
		if change:
			# Override the texture for a specific cellmap.
			# The cellmap name is usually the filename (e.g., "common.ssce") 
			# or the name defined in the SpriteStudio project.
			ss_player.set_cellmap_texture("common.ssce", to_texture)
			print("Texture changed to override.")
		else:
			# Pass null to clear the override and restore the original texture.
			ss_player.set_cellmap_texture("common.ssce", null)
			print("Texture restored to original.")
			
		prev_change = change
