tool
extends EditorImportPlugin

func get_preset_count():
	return 0

func get_preset_name(preset):
	return ""

func get_import_options(preset):
	return []

func get_option_visibility(option, options):
	return false

func import(source_file, save_path, options, r_platform_variants, r_gen_files):
	var doc = GdResourceSsDocument.new()
	doc.load_from_file(source_file,source_file)

	return ResourceSaver.save("%s.%s" % [save_path, get_save_extension()], doc)

func get_importer_name():
	return "ssae_importer"

func get_visible_name():
	return "ssae_importer"

func get_recognized_extensions():
	return ["ssae"]

func get_save_extension():
	return "gdssae"
	
func get_resource_type():
	return "GdResourceSsAnimePack"
