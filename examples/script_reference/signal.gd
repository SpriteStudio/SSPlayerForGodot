extends SpriteStudioPlayer2D

# Example script demonstrating how to connect and handle signals 
# from a SpriteStudio animation in v2.x.

func _ready():
	# Connect to the "signal" signal emitted by SpriteStudioPlayer2D
	# Note: 'signal' is a keyword in GDScript, so we must use string-based connect
	self.connect("signal", _on_ss_signal)

func _on_ss_signal(command: String, value: Dictionary):
	print("on_signal()")
	print("command=" + command)
	print("value=" + str(value))
