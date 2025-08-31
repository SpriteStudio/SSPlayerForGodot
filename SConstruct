#!/usr/bin/env python
import os
import sys
import subprocess

def normalize_path(val, env):
    return val if os.path.isabs(val) else os.path.join(env.Dir("#").abspath, val)


def validate_parent_dir(key, val, env):
    if not os.path.isdir(normalize_path(os.path.dirname(val), env)):
        raise UserError("'%s' is not a directory: %s" % (key, os.path.dirname(val)))


libname = "SSGodot"
projectdir = os.path.join("examples", "feature_test_gdextension")

localEnv = Environment(tools=["default"], PLATFORM="")

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)
opts.Add(
    BoolVariable(
        key="compiledb",
        help="Generate compilation DB (`compile_commands.json`) for external tools",
        default=localEnv.get("compiledb", False),
    )
)
opts.Add(
    PathVariable(
        key="compiledb_file",
        help="Path to a custom `compile_commands.json` file",
        default=localEnv.get("compiledb_file", "compile_commands.json"),
        validator=validate_parent_dir,
    )
)
opts.Update(localEnv)

Help(opts.GenerateHelpText(localEnv))

env = localEnv.Clone()
env["compiledb"] = False

env.Tool("compilation_db")
compilation_db = env.CompilationDatabase(
    normalize_path(localEnv["compiledb_file"], localEnv)
)
env.Alias("compiledb", compilation_db)

submodule_initialized = False
dir_name = 'godot-cpp'
if os.path.isdir(dir_name):
    if os.listdir(dir_name):
        submodule_initialized = True

if not submodule_initialized:
    print("""godot-cpp is not available within this folder, as Git submodules haven't been initialized.
Run the following command to download godot-cpp:

    git submodule update --init --recursive""")
    sys.exit(1)

env = SConscript("godot-cpp/SConstruct", {"env": env, "customs": customs})

env.Append(CPPDEFINES = "SPRITESTUDIO_GODOT_EXTENSION")
env.Append(CPPDEFINES = "_NOTUSE_STBI")
env.Append(CPPDEFINES = "_NOTUSE_TEXTURE_FULLPATH")
env.Append(CPPDEFINES = "_NOTUSE_EXCEPTION")
env.Append(CPPPATH=["src/"])
env.Append(
	CPPPATH=[
		"gd_spritestudio/SpriteStudio6-SDK/Common/Loader",
		"gd_spritestudio/SpriteStudio6-SDK/Common/Animator",
		"gd_spritestudio/SpriteStudio6-SDK/Common/Helper",
	]
)

if env["platform"] == 'macos':
    env.Append(LINKFLAGS=["-framework", "CoreFoundation"])

# Set iOS minimum deployment target
if env["platform"] == "ios":
    env.Append(CCFLAGS=["-miphoneos-version-min=12.0"])
    env.Append(LINKFLAGS=["-miphoneos-version-min=12.0"])

sources = Glob("gd_spritestudio/*.cpp")
sources.extend(Glob("gd_spritestudio/SpriteStudio6-SDK/Common/Loader/tinyxml2/*.cpp"))
sources.extend(Glob("gd_spritestudio/SpriteStudio6-SDK/Common/Loader/*.cpp"))
sources.extend(Glob("gd_spritestudio/SpriteStudio6-SDK/Common/Animator/*.cpp"))
sources.extend(Glob("gd_spritestudio/SpriteStudio6-SDK/Common/Helper/DebugPrint.cpp"))
sources.extend(Glob("gd_spritestudio/SpriteStudio6-SDK/Common/Helper/IsshTexture.cpp"))
sources.extend(Glob("gd_spritestudio/SpriteStudio6-SDK/Common/Helper/stb_image.c"))

if env["target"] in ["editor", "template_debug"]:
    try:
        doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
        sources.append(doc_data)
    except AttributeError:
        print("Not including class reference as we're targeting a pre-4.3 baseline.")

file = "lib{}{}{}".format(libname, env["suffix"], env["SHLIBSUFFIX"])
filepath = ""

if env["platform"] == "macos" or env["platform"] == "ios":
    filepath = "lib{}.{}.{}.framework/".format(libname, env["platform"], env["target"])
    file = "lib{}.{}.{}".format(libname, env["platform"], env["target"])
    env.Append(LINKFLAGS=["-Wl,-install_name,@rpath/{}{}".format(filepath, file)])

libraryfile = "bin/{}/{}{}".format(env["platform"], filepath, file)
library = env.SharedLibrary(
    libraryfile,
    source=sources,
)

if env["platform"] == "macos" or env["platform"] == "ios":
    plist_subst = {
        "${BUNDLE_LIBRARY}": file,
        "${BUNDLE_NAME}": "ssplayer-godot",
        "${BUNDLE_IDENTIFIER}": "jp.co.cri-mw.spritestudio.ssplayer-godot.{}".format(env["target"]),
        "${BUNDLE_VERSION}": subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', 'HEAD']).decode('utf-8').strip().split('-')[0] + '.0',
        "${MIN_MACOS_VERSION}": "10.12",
        "${MIN_IOS_VERSION}": "12.0"
    }

    if env["platform"] == "macos":
        plist_file = "bin/macos/{}Resources/Info.plist".format(filepath)
        plist = env.Substfile(
            target=plist_file,
            source="misc/Info.macos.plist",
            SUBST_DICT=plist_subst
        )
    elif env["platform"] == "ios":
        plist_file = "bin/ios/{}Info.plist".format(filepath)
        plist = env.Substfile(
            target=plist_file,
            source="misc/Info.ios.plist",
            SUBST_DICT=plist_subst
        )

    env.Depends(library, plist)

copy = env.InstallAs("{}/{}".format(projectdir, libraryfile), library)
default_args = [library, copy]

if env["platform"] == "macos" or env["platform"] == "ios":
    copy_plist = env.InstallAs("{}/{}".format(projectdir, plist_file), plist_file)
    default_args.append(copy_plist)

if localEnv.get("compiledb", False):
    default_args += [compilation_db]
Default(*default_args)