#!/usr/bin/env python
"""Module declaration for the Toqraq debugger module."""


def can_build(env, platform):
    """The module builds on every platform Godot supports."""
    return True


def configure(env):
    """No extra build configuration required."""


def get_doc_classes():
    """The module exposes no script-facing classes."""
    return []


def get_doc_path():
    """No documentation XML is shipped."""
    return ""
