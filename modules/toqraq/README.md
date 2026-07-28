# Toqraq Godot Module

An engine module adding a `toqraq:` capture to `EngineDebugger`. It is the
only divergence of the Toqraq Godot fork from upstream. The module is
inert unless a debug client sends `toqraq:*` messages.

Source of truth: `engine/godot/modules/toqraq/` in the
`Toqraq-hermes-plugin` repository. The client-side mirror of this wire
contract is `src/plugin/toqraq/engines/godot/debugger/toqraq_messages.py`.

## Wire vocabulary

Transport: the standard remote-debug channel. Message names below are
dispatched under the `toqraq:` prefix (e.g. wire name `toqraq:inject_key`).

### Requests (client → game)

| Message | Arguments | Answer |
| --- | --- | --- |
| `inject_key` | `[keycode:int, pressed:bool, physical:bool, echo:bool, modifiers:int]` | — |
| `inject_mouse_button` | `[x:float, y:float, button:int, pressed:bool, double_click:bool, modifiers:int]` | — |
| `inject_mouse_motion` | `[x:float, y:float, rel_x:float, rel_y:float, buttons_mask:int, modifiers:int]` | — |
| `inject_action` | `[action:StringName, pressed:bool, strength:float]` | — |
| `screenshot` | `[]` | `toqraq:screenshot` `[width:int, height:int, png:PackedByteArray]` |
| `meta` | `[]` | `toqraq:meta` `[monitors:Array, fps:float]` |
| `scene_watch` | `[enabled:bool]` | events below |

### Events (game → client)

| Message | Payload | When |
| --- | --- | --- |
| `toqraq:scene_events` | `[[event, path, type, id], ...]` | batched per frame while scene watch is enabled |

`event` is one of `added`, `removed`, `renamed`; `path` the node path at
event time; `type` the engine class; `id` the instance id.

### Modifiers bitmask

| Bit | Modifier |
| --- | --- |
| 0 | Shift |
| 1 | Alt |
| 2 | Ctrl |
| 3 | Meta |

### Coordinates

Mouse positions are root-viewport pixels, matching the embedded window's
client area when the game runs embedded.

## Build

Drop this directory into a Godot source tree at `modules/toqraq/` and run
the normal SCons build; no other step is required. See
`docs/engine-fork.md` for the fork runbook.
