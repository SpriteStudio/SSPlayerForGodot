extends SpriteStudioPlayer2D

# Example script demonstrating how to dynamically load an .ssab resource,
# list its animations, and handle the animation_finished signal.

var ssab_paths = ["res://path/to/first.ssab", "res://path/to/second.ssab"]
var current_index = 0

func _ready():
	# 1. Load an .ssab file dynamically
	load_and_play_current()
	
	# 2. Set up the finish callback
	self.animation_finished.connect(_on_animation_finished)

func load_and_play_current():
	if current_index >= ssab_paths.size():
		current_index = 0
		
	var ssab: SSABResource = ResourceLoader.load(ssab_paths[current_index])
	if ssab == null:
		print("Failed to load: " + ssab_paths[current_index])
		return
		
	# Assign the resource to the player
	self.set_ssab_resource(ssab)
	
	# Enumerate animations
	var anim_names = ssab.get_animation_names()
	print("Available animations: ", anim_names)
	
	if anim_names.size() > 0:
		# Play the first animation
		var target_anim = anim_names[0]
		self.set_animation(target_anim)
		self.set_loop_count(1) # Play once to trigger the finish signal
		self.play()
		print("Playing: " + target_anim)

func _on_animation_finished(anim_name: String):
	print("Finished: " + anim_name)
	
	# Switch to the next .ssab file
	current_index += 1
	load_and_play_current()
