"""Where Broadcast Studio finds the project, its examples, and the interpreter."""
import os
import shutil

IDE_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(IDE_DIR)
EXAMPLES_DIR = os.path.join(PROJECT_DIR, "examples")
DOCUMENTATION = os.path.join(PROJECT_DIR, "DOCUMENTATION.md")

# The places the build steps in DOCUMENTATION.md put the program, in order of preference.
_CANDIDATES = [
    os.path.join("build", "Release", "broadcast_language.exe"),
    os.path.join("build", "Debug", "broadcast_language.exe"),
    os.path.join("build", "broadcast_language.exe"),
    os.path.join("build", "broadcast_language"),
    "broadcast_language.exe",
    "broadcast_language",
]


def find_interpreter():
    """The path of the built interpreter, or None if it has not been built yet."""
    for relative in _CANDIDATES:
        path = os.path.join(PROJECT_DIR, relative)
        if os.path.isfile(path):
            return path
    return shutil.which("broadcast_language")
