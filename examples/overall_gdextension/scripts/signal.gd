extends SpriteStudioPlayer2D

# Example script demonstrating how to connect and handle signals 
# from a SpriteStudio animation in v2.x.

func _ready():
	# Connect to the "signal_emitted" signal of SpriteStudioPlayer2D
	self.signal_emitted.connect(_on_ss_signal)

func _on_ss_signal(command: String, value: Dictionary, info: Dictionary):
	print("on_signal()")
	print("command=" + command)
	# `value` holds the authored parameters, keyed by parameter id.
	print("value=" + str(value))
	# `info` holds the event's origin: part_index / part_name / frame_no.
	print("part=" + str(info["part_name"]) + " frame=" + str(info["frame_no"]))
