#pragma once

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/node2d.hpp>
using namespace godot;
#else
#include "scene/2d/node_2d.h"
#endif

#include "gd_ssplayer_resource.h"

class GdSsPlayerNode : public Node2D {
    GDCLASS( GdSsPlayerNode, Node2D );

protected:
    static void _bind_methods();
};
