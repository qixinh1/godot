/**************************************************************************/
/*  toqraq_capture.h                                                      */
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

#pragma once

#ifdef DEBUG_ENABLED

#include "core/debugger/engine_debugger.h"
#include "core/object/object.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"

class Node;

/// EngineDebugger capture implementing the `toqraq:` message vocabulary.
///
/// Wire contract (arguments and answers) is documented in
/// `modules/toqraq/README.md` and mirrored by the plugin's
/// `engines/godot/debugger/toqraq_messages.py`.
class ToqraqCapture : public Object {
	GDCLASS(ToqraqCapture, Object);

	using Handler = Error (ToqraqCapture::*)(const Array &);

	static ToqraqCapture *singleton;

	HashMap<String, Handler> handlers;
	bool scene_watch_enabled = false;
	LocalVector<Array> scene_event_queue;

	Error _msg_inject_key(const Array &p_args);
	Error _msg_inject_mouse_button(const Array &p_args);
	Error _msg_inject_mouse_motion(const Array &p_args);
	Error _msg_inject_action(const Array &p_args);
	Error _msg_screenshot(const Array &p_args);
	Error _msg_meta(const Array &p_args);
	Error _msg_scene_watch(const Array &p_args);

	void _on_node_event(Node *p_node, const String &p_event);
	void _flush_scene_events();
	void _set_scene_watch(bool p_enabled);

	static Error parse_message(void *p_user, const String &p_msg, const Array &p_args, bool &r_captured);

protected:
	static void _bind_methods();

public:
	static ToqraqCapture *get_singleton();
	static void initialize();
	static void deinitialize();

	ToqraqCapture();
	~ToqraqCapture();
};

#endif // DEBUG_ENABLED
