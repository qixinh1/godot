/**************************************************************************/
/*  toqraq_capture.cpp                                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             TOQRAQ MODULE                              */
/**************************************************************************/
/* Copyright (c) 2026 Toqraq contributors.                                */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "toqraq_capture.h"

#ifdef DEBUG_ENABLED

#include "core/config/engine.h"
#include "core/input/input.h"
#include "core/input/input_event.h"
#include "core/io/image.h"
#include "main/performance.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "servers/rendering/rendering_server.h"

#ifdef RD_ENABLED
#include "servers/rendering/rendering_device.h"
#endif

// Modifier bitmask shared with the client (README.md §modifiers).
#define TOQRAQ_MOD_SHIFT (1 << 0)
#define TOQRAQ_MOD_ALT (1 << 1)
#define TOQRAQ_MOD_CTRL (1 << 2)
#define TOQRAQ_MOD_META (1 << 3)

ToqraqCapture *ToqraqCapture::singleton = nullptr;

ToqraqCapture *ToqraqCapture::get_singleton() {
	return singleton;
}

void ToqraqCapture::initialize() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = memnew(ToqraqCapture);
	EngineDebugger::register_message_capture("toqraq", EngineDebugger::Capture(singleton, &ToqraqCapture::parse_message));
}

void ToqraqCapture::deinitialize() {
	if (!singleton) {
		return;
	}
	EngineDebugger::unregister_message_capture("toqraq");
	memdelete(singleton);
	singleton = nullptr;
}

void ToqraqCapture::_bind_methods() {
	// Nothing script-facing; interaction happens over the debug wire only.
}

ToqraqCapture::ToqraqCapture() {
	handlers["inject_key"] = &ToqraqCapture::_msg_inject_key;
	handlers["inject_mouse_button"] = &ToqraqCapture::_msg_inject_mouse_button;
	handlers["inject_mouse_motion"] = &ToqraqCapture::_msg_inject_mouse_motion;
	handlers["inject_action"] = &ToqraqCapture::_msg_inject_action;
	handlers["screenshot"] = &ToqraqCapture::_msg_screenshot;
	handlers["meta"] = &ToqraqCapture::_msg_meta;
	handlers["scene_watch"] = &ToqraqCapture::_msg_scene_watch;
}

ToqraqCapture::~ToqraqCapture() {
	_set_scene_watch(false);
}

Error ToqraqCapture::parse_message(void *p_user, const String &p_msg, const Array &p_args, bool &r_captured) {
	ToqraqCapture *self = static_cast<ToqraqCapture *>(p_user);
	ERR_FAIL_NULL_V(self, ERR_UNCONFIGURED);
	const Handler *handler = self->handlers.getptr(p_msg);
	if (!handler) {
		return OK; // Unknown message: leave r_captured false for other captures.
	}
	r_captured = true;
	return (self->**handler)(p_args);
}

template <typename T>
static void _apply_modifiers(const Ref<T> &p_event, int64_t p_modifiers) {
	p_event->set_shift_pressed((p_modifiers & TOQRAQ_MOD_SHIFT) != 0);
	p_event->set_alt_pressed((p_modifiers & TOQRAQ_MOD_ALT) != 0);
	p_event->set_ctrl_pressed((p_modifiers & TOQRAQ_MOD_CTRL) != 0);
	p_event->set_meta_pressed((p_modifiers & TOQRAQ_MOD_META) != 0);
}

Error ToqraqCapture::_msg_inject_key(const Array &p_args) {
	ERR_FAIL_COND_V(p_args.size() < 5, ERR_INVALID_DATA);
	Ref<InputEventKey> event;
	event.instantiate();
	const bool physical = p_args[2];
	if (physical) {
		event->set_physical_keycode((Key)(int64_t)p_args[0]);
	} else {
		event->set_keycode((Key)(int64_t)p_args[0]);
	}
	event->set_pressed(p_args[1]);
	event->set_echo(p_args[3]);
	_apply_modifiers(event, p_args[4]);
	Input::get_singleton()->parse_input_event(event);
	return OK;
}

Error ToqraqCapture::_msg_inject_mouse_button(const Array &p_args) {
	ERR_FAIL_COND_V(p_args.size() < 6, ERR_INVALID_DATA);
	Ref<InputEventMouseButton> event;
	event.instantiate();
	const Vector2 position((double)p_args[0], (double)p_args[1]);
	event->set_position(position);
	event->set_global_position(position);
	event->set_button_index((MouseButton)(int64_t)p_args[2]);
	event->set_pressed(p_args[3]);
	event->set_double_click(p_args[4]);
	_apply_modifiers(event, p_args[5]);
	Input::get_singleton()->parse_input_event(event);
	return OK;
}

Error ToqraqCapture::_msg_inject_mouse_motion(const Array &p_args) {
	ERR_FAIL_COND_V(p_args.size() < 6, ERR_INVALID_DATA);
	Ref<InputEventMouseMotion> event;
	event.instantiate();
	const Vector2 position((double)p_args[0], (double)p_args[1]);
	event->set_position(position);
	event->set_global_position(position);
	event->set_relative(Vector2((double)p_args[2], (double)p_args[3]));
	event->set_button_mask(BitField<MouseButtonMask>((MouseButtonMask)(int64_t)p_args[4]));
	_apply_modifiers(event, p_args[5]);
	Input::get_singleton()->parse_input_event(event);
	return OK;
}

Error ToqraqCapture::_msg_inject_action(const Array &p_args) {
	ERR_FAIL_COND_V(p_args.size() < 3, ERR_INVALID_DATA);
	const StringName action = p_args[0];
	const bool pressed = p_args[1];
	const float strength = p_args[2];
	if (pressed) {
		Input::get_singleton()->action_press(action, strength);
	} else {
		Input::get_singleton()->action_release(action);
	}
	return OK;
}

Error ToqraqCapture::_msg_screenshot(const Array &p_args) {
	Viewport *root = SceneTree::get_singleton()->get_root();
	ERR_FAIL_NULL_V_MSG(root, ERR_UNCONFIGURED, "No root viewport.");
	Ref<ViewportTexture> texture = root->get_texture();
	ERR_FAIL_COND_V_MSG(texture.is_null(), ERR_UNCONFIGURED, "No viewport texture.");
	Ref<Image> image = texture->get_image();
	ERR_FAIL_COND_V_MSG(image.is_null(), ERR_UNCONFIGURED, "No viewport image.");
	image->clear_mipmaps();
	image->convert(Image::FORMAT_RGBA8);
#ifdef RD_ENABLED
	RenderingDevice *rendering_device = RD::get_singleton();
	if (rendering_device && RenderingServer::get_singleton()->viewport_is_using_hdr_2d(root->get_viewport_rid())) {
		image->linear_to_srgb();
	}
#endif
	Vector<uint8_t> png = image->save_png_to_buffer();
	ERR_FAIL_COND_V_MSG(png.is_empty(), FAILED, "PNG encoding failed.");
	Array answer;
	answer.append(image->get_width());
	answer.append(image->get_height());
	answer.append(png);
	EngineDebugger::get_singleton()->send_message("toqraq:screenshot", answer);
	return OK;
}

Error ToqraqCapture::_msg_meta(const Array &p_args) {
	Performance *performance = Performance::get_singleton();
	ERR_FAIL_NULL_V(performance, ERR_UNCONFIGURED);
	Array monitors;
	const int max = performance->get("MONITOR_MAX");
	monitors.resize(max);
	for (int i = 0; i < max; i++) {
		monitors[i] = performance->call("get_monitor", i);
	}
	Array answer;
	answer.append(monitors);
	answer.append(Engine::get_singleton()->get_frames_per_second());
	EngineDebugger::get_singleton()->send_message("toqraq:meta", answer);
	return OK;
}

Error ToqraqCapture::_msg_scene_watch(const Array &p_args) {
	ERR_FAIL_COND_V(p_args.is_empty(), ERR_INVALID_DATA);
	_set_scene_watch(p_args[0]);
	return OK;
}

void ToqraqCapture::_set_scene_watch(bool p_enabled) {
	if (p_enabled == scene_watch_enabled) {
		return;
	}
	SceneTree *tree = SceneTree::get_singleton();
	if (tree == nullptr) {
		scene_watch_enabled = false;
		return;
	}
	const Callable added = callable_mp(this, &ToqraqCapture::_on_node_event).bind("added");
	const Callable removed = callable_mp(this, &ToqraqCapture::_on_node_event).bind("removed");
	const Callable renamed = callable_mp(this, &ToqraqCapture::_on_node_event).bind("renamed");
	const Callable flush = callable_mp(this, &ToqraqCapture::_flush_scene_events);
	if (p_enabled) {
		tree->connect("node_added", added, CONNECT_REFERENCE_COUNTED);
		tree->connect("node_removed", removed, CONNECT_REFERENCE_COUNTED);
		tree->connect("node_renamed", renamed, CONNECT_REFERENCE_COUNTED);
		tree->connect("process_frame", flush, CONNECT_REFERENCE_COUNTED);
	} else {
		tree->disconnect("node_added", added);
		tree->disconnect("node_removed", removed);
		tree->disconnect("node_renamed", renamed);
		tree->disconnect("process_frame", flush);
		scene_event_queue.clear();
	}
	scene_watch_enabled = p_enabled;
}

void ToqraqCapture::_on_node_event(Node *p_node, const String &p_event) {
	if (!scene_watch_enabled || p_node == nullptr) {
		return;
	}
	Array event;
	event.append(p_event);
	event.append(String(p_node->get_path()));
	event.append(p_node->get_class());
	event.append((int64_t)p_node->get_instance_id());
	scene_event_queue.push_back(event);
}

void ToqraqCapture::_flush_scene_events() {
	if (scene_event_queue.is_empty()) {
		return;
	}
	Array batch;
	batch.resize(scene_event_queue.size());
	for (uint32_t i = 0; i < scene_event_queue.size(); i++) {
		batch[i] = scene_event_queue[i];
	}
	scene_event_queue.clear();
	EngineDebugger::get_singleton()->send_message("toqraq:scene_events", batch);
}

#endif // DEBUG_ENABLED
