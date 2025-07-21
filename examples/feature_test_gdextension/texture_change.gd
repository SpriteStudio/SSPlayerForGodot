extends GdNodeSsPlayer


# Declare member variables here. Examples:
# var a = 2
# var b = "text"

#class MyScript extends GdNodeSsPlayer
@export var to_texture: Texture2D
@export var change: bool = false

var org_texture: Texture2D
var prev_change = change

# Called when the node enters the scene tree for the first time.
func _ready():
	change = false
	print("to_texture:" + to_texture.resource_path)

	org_texture = self.res_player.res_project.get_cell_map_resource_from_name("common.ssce").texture
	print("org_texture:" + org_texture.resource_path)

	self.set_frame(0)
	self.set_loop(true)
	self.set_play(true)

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	if change != prev_change:
		var to_tex: Texture2D
		if change:
			to_tex = to_texture
		else:
			to_tex = org_texture
		self.res_player.res_project.get_cell_map_resource_from_name("common.ssce").texture = to_tex
		prev_change = change
	
