# 🎞️ Integration with AnimationPlayer (Cinematics and State Machines)

By combining SSPlayer with Godot's standard `AnimationPlayer`, you can integrate SpriteStudio animation playback with Godot's powerful timeline and state machine features.

This enables you to visually perform tasks such as **synchronizing attack hitboxes, triggering sound effects (SE), shaking the camera, and creating cutscenes** without writing complex code.

---

## 1. Generating an Animation Library

To control SSPlayer with an `AnimationPlayer`, you must first convert all animations (e.g., walk, attack) contained in the target `.ssab` file into an `AnimationLibrary` (`_anims.res`) that Godot can read.

1. In the Godot editor, select the `.ssab` (or `.ssqb`) file from the FileSystem dock.
2. Click the **"Gen AnimLib"** button at the bottom of the Inspector.
3. Upon success, a file named `[original_filename]_anims.res` will be generated in the same directory.

> [!NOTE]
> The generated `_anims.res` automatically contains "Value Tracks" with the exact same names as the original SpriteStudio animations. These tracks control the `current_animation` (animation name) and `frame_no` (playhead) properties of the target node.

---

## 2. Setting Up the AnimationPlayer

Here is how to apply the generated library to your scene.

1. **Prepare the Node**
   Place a `SpriteStudioPlayer2D` node in your scene and assign the target `.ssab` file to its `Ssab` property.
2. **Add an AnimationPlayer**
   Add an `AnimationPlayer` node to the scene.
3. **Specify the Target (Important!)**
   Select the added `AnimationPlayer` node and set its **`Root Node`** property in the Inspector to the **`SpriteStudioPlayer2D` node** from step 1.
   *Note: Since the generated animations are configured to control the `current_animation` and `frame_no` properties of the target node itself, the Root Node must point directly to the SSPlayer node.*
4. **Load the Library**
   Open the "Animation" panel at the bottom of the editor, click the "Animation" menu > **"Manage Animations..."**.
   Click the folder icon (Load Library) in the dialog that appears, and load the generated `_anims.res` file.

You will now see a list of all animations created in SpriteStudio directly in the AnimationPlayer dropdown!

---

## 3. Advanced Use Cases

Once integrated with `AnimationPlayer`, you unlock several advanced control methods:

### A. Synchronizing with Events and Sound Effects (Call Method Track / Audio Track)
Add a "Call Method Track" or "Audio Playback Track" on the AnimationPlayer timeline.
You can visually and perfectly sync the exact frame a character swings a sword with playing a sound effect or calling a GDScript function like `enable_hitbox()`.

### B. Using State Machines (AnimationTree)
Add an `AnimationTree` node to the scene and assign the aforementioned AnimationPlayer to its `Anim Player` property.
By setting the `Tree Root` to `AnimationNodeStateMachine`, you can visually construct state transitions (e.g., Idle -> Walk -> Attack) using a node-based interface instead of writing complex logic in code.

### C. Cutscenes and Cinematics
By adding tracks that move the camera (`Camera2D`), change background colors, or trigger other visual effects, you can create fully synchronized event scenes (cutscenes) all within a single timeline.
