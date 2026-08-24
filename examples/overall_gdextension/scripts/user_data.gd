extends SpriteStudioPlayer2D

# Example script demonstrating how to connect and handle user data 
# from a SpriteStudio animation in v2.x.

func _ready():
	# Connect to the "user_data" signal emitted by SpriteStudioPlayer2D
	self.user_data.connect(_on_ss_user_data)

func _on_ss_user_data(payload: Dictionary):
	print("on_user_data()")
	
	# The origin is always present; the authored values only when they were set.
	print("part=" + str(payload["part_name"]) + " frame=" + str(payload["frame_no"]))

	# In v2.x, user data is packed into a Dictionary
	if payload.has("integer"):
		print("int_value=" + str(payload["integer"]))
	if payload.has("rect"):
		print("rect_value=" + str(payload["rect"]))
	if payload.has("point"):
		print("point_value=" + str(payload["point"]))
	if payload.has("string"):
		print("string_value=" + payload["string"])
