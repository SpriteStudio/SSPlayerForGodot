extends SpriteStudioPlayer2D

# Example script demonstrating how to connect and handle signals 
# from a SpriteStudio animation in v2.x.

func _ready():
	# Connect to the "signal_emitted" signal of SpriteStudioPlayer2D
	self.signal_emitted.connect(_on_ss_signal)

func _on_ss_signal(command: String, value: Dictionary):
	print("on_signal()")
	print("command=" + command)
	print("value=" + str(value))
