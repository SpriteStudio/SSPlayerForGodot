extends GdNodeSsPlayer


# Declare member variables here. Examples:
# var a = 2
# var b = "text"


# Called when the node enters the scene tree for the first time.
func _ready():
	pass # Replace with function body.


# Called every frame. 'delta' is the elapsed time since the previous frame.
#func _process(delta):
#	pass


func _on_UserData_user_data(flag, int_value, rect_value, point_value, string_value):
	#pass # Replace with function body.
	print("on_user_data()")
	print("flag=" + "true" if flag else "false")
	print("int_value=" + str(int_value))
	print("rect_value=" + str(rect_value))
	print("point_value=" + str(point_value))
	print("string_value=" + string_value)

