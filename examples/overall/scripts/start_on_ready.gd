extends SpriteStudioPlayer2D

# Example script demonstrating how to start playing an animation automatically 
# as soon as the node is ready.

func _ready():
	# Optionally set the speed scale and loop count
	self.set_speed_scale(1.0)
	self.set_loop_count(-1) # -1 = infinite loop
	
	# Start playing from the beginning
	self.play()
