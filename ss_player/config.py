# config.py

def can_build(env, platform):
    return True

def configure(env):
    pass

def get_doc_classes():
    return [
        "SpriteStudioPlayer2D",
        "SpriteStudioPartAttachment2D",
        "SpriteStudioAudioBackend",
        "SSABResource",
        "SSQBResource",
    ]

def get_doc_path():
    return "doc_classes"
