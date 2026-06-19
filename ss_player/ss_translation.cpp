#include "ss_translation.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/translation_server.hpp>
#include <godot_cpp/classes/translation.hpp>
using namespace godot;
#else
#include "core/string/translation.h"
#include "core/string/translation_server.h"
#endif

static Ref<Translation> ja_translation;

void register_ss_translations() {
    ja_translation.instantiate();
    ja_translation->set_locale("ja");
    
    // Warnings in node 2d
    ja_translation->add_message("Assign an SSABResource to the \"ssab\" property to play an animation.", "アニメーションを再生するには、\"ssab\" プロパティに SSABResource を割り当ててください。");
    ja_translation->add_message("Select an animation in the \"animation\" property.", "\"animation\" プロパティでアニメーションを選択してください。");

    // Dock strings
    ja_translation->add_message("Add SpriteStudioPlayer2D", "SpriteStudioPlayer2Dを追加");
    ja_translation->add_message("Open SSPJ", "SSPJを開く");
    ja_translation->add_message("Reconvert", "再コンバート");
    ja_translation->add_message("Output:", "出力先:");
    ja_translation->add_message("Choose output directory", "出力先ディレクトリを選択");
    ja_translation->add_message("Reset to default directory", "デフォルトにリセット");
    ja_translation->add_message("Open in File Manager", "ファイルマネージャーで開く");
    ja_translation->add_message("Drop SSPJ or a folder here\n(drag from your file manager)", "ここにSSPJまたはフォルダをドロップ\n(ファイルマネージャーからドラッグ)");
    ja_translation->add_message("Recent SSPJs", "最近のSSPJ");
    ja_translation->add_message("Clear recent SSPJ files", "履歴をクリア");
    ja_translation->add_message("player:", "player:");
    ja_translation->add_message("converter:", "converter:");
    ja_translation->add_message("Import Error", "インポートエラー");
    ja_translation->add_message("No .sspj files or folders found.\nPlease drop SpriteStudio project (.sspj) files or a folder containing them.", ".sspjファイルが見つかりません。\nSpriteStudioプロジェクト(.sspj)ファイル、またはそれを含むフォルダをドロップしてください。");
    ja_translation->add_message("Reveal", "ファイルの場所を開く");
    ja_translation->add_message("Remove from Recent", "履歴から削除");
    ja_translation->add_message("No recent files. Drop a sspj above to start.", "履歴がありません。上の枠にSSPJをドロップして開始してください。");
    
    // Importer
    ja_translation->add_message("Import", "インポート");
    ja_translation->add_message("No .sspj files were found in the dropped folder(s).", "ドロップされたフォルダ内に.sspjファイルが見つかりませんでした。");
    ja_translation->add_message("Scan stopped", "スキャン停止");
    ja_translation->add_message("The folder is very large, so scanning stopped early.\nFound %d .sspj file(s) so far.\n\nImport what was found, keep scanning, or stop?", "フォルダが大きすぎるため、スキャンを中断しました。\nこれまでに %d 個の.sspjファイルが見つかりました。\n\n見つかったものをインポートしますか？スキャンを続行しますか？");
    ja_translation->add_message("Import found", "インポート");
    ja_translation->add_message("Stop", "停止");
    ja_translation->add_message("Keep scanning", "スキャン続行");
    ja_translation->add_message("Some outputs already belong to a different SpriteStudio project and would be overwritten:\n\n", "一部の出力ファイルは別のSpriteStudioプロジェクトに属しており、上書きされます:\n\n");
    ja_translation->add_message("\nOverwrite them anyway?", "\n上書きして続行しますか？");
    ja_translation->add_message("Output name collision", "出力名の衝突");
    ja_translation->add_message("Overwrite", "上書き");
    ja_translation->add_message("Cancel", "キャンセル");
    ja_translation->add_message("Some files failed to import.\n\n", "一部のファイルのインポートに失敗しました。\n\n");
    ja_translation->add_message("\nPlease check the Output tab for details.", "\n詳細は出力タブを確認してください。");

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    TranslationServer::get_singleton()->add_translation(ja_translation);
#else
    TranslationServer::get_singleton()->add_translation(ja_translation);
#endif
}

void unregister_ss_translations() {
    if (ja_translation.is_valid()) {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
        TranslationServer::get_singleton()->remove_translation(ja_translation);
#else
        TranslationServer::get_singleton()->remove_translation(ja_translation);
#endif
        ja_translation.unref();
    }
}
