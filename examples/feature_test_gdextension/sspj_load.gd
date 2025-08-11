extends Node


# Declare member variables here. Examples:
# var a = 2
# var b = "text"

@onready var anime_player = $AnimationPlayer

@onready var ssnode = $by_script
var cur_frame = 0
var sspj_index = 0;
var sspj_files = ["ssdata/v6_all.sspj", "basedir/BaseDir.sspj"]
#var anime_pack_names = ["Basic.ssae", "NewAnimation.ssae"]

# Called when the node enters the scene tree for the first time.
func _ready():
	anime_player.play("SsPlayer");

	# sspj ファイル読み込み
	var sspj: GdResourceSsProject = ResourceLoader.load("ssdata/v6_all.sspj")
	if sspj == null:
		print("Failed to load sspj file")
		return
 
	ssnode.res_player.res_project = sspj
	
	# ssae 列挙
	var anime_pack_names = sspj.get_anime_pack_names()
	var anime_names
	for anime_pack_name in anime_pack_names:
		print("anime_pack_name:" + anime_pack_name)
		# AnimePack リソースを取得
		var anime_pack_resource = sspj.get_anime_pack_resource(anime_pack_name)
		# アニメーション名を列挙
		anime_names = anime_pack_resource.get_animation_names()
		for anime_name in anime_names:
			print("anime_name:" + anime_name)

	# １つめのSSAEの1つ目のアニメを再生
	ssnode.set_anime_pack(anime_pack_names[0])
	var anime_name = anime_names[0]
	if anime_name == "Setup":
		anime_name = anime_names[1]
	print("play anime:" + anime_name + " in " + anime_pack_names[0])
	ssnode.set_animation(anime_name)
	
	# 開始・終了フレームの取得
	print("start_frame:" + str(ssnode.get_start_frame()))
	print("end_frame:" + str(ssnode.get_end_frame()))
		
	# アニメーション終了時コールバックの設定
	ssnode.connect("animation_finished", Callable(self, "_on_animation_finished"))

	ssnode.set_loop(true)
	ssnode.play()

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	pass

func _on_animation_finished(name):
	print("SIGNAL _on_animation_finished from " + name)
	# 別のsspjに変更
	if true:
		sspj_index += 1
		if sspj_index >= len(sspj_files): sspj_index = 0
		ssnode.res_player.res_project = ResourceLoader.load(sspj_files[sspj_index])
		var anime_pack_names = ssnode.res_player.res_project.get_anime_pack_names()
		ssnode.set_anime_pack(anime_pack_names[0])
		ssnode.set_animation("anime_1")
