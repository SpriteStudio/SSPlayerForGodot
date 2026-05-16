extends SpriteStudioPlayer2D

# Example script demonstrating how to start playing an animation automatically 
# as soon as the node is ready.

func _ready():
	# Optionally set the speed and loop count
	self.set_speed(1.0)
	self.set_loop(0) # 0 = infinite loop
	
	# Start playing from the beginning
	self.play()
