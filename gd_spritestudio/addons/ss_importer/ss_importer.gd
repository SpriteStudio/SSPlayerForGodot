tool
extends EditorPlugin

var importer_sspj
var importer_ssae
var importer_ssce
var importer_ssee

func _enter_tree():
	importer_sspj = preload("ss_importer_sspj.gd").new()
	importer_ssae = preload("ss_importer_ssae.gd").new()
	importer_ssce = preload("ss_importer_ssce.gd").new()
	importer_ssee = preload("ss_importer_ssee.gd").new()
	add_import_plugin(importer_sspj)
	add_import_plugin(importer_ssae)
	add_import_plugin(importer_ssce)
	add_import_plugin(importer_ssee)

func _exit_tree():
	remove_import_plugin(importer_sspj)
	remove_import_plugin(importer_ssae)
	remove_import_plugin(importer_ssce)
	remove_import_plugin(importer_ssee)
	importer_sspj = null
	importer_ssae = null
	importer_ssce = null
	importer_ssee = null
