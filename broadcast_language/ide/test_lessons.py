"""Checks that every lesson's solution passes its own check, using the real interpreter.

Usage: python ide/test_lessons.py [path/to/broadcast_language]
"""
import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from lessons import LESSONS, check_answer  # noqa: E402
from broadcast_paths import find_interpreter  # noqa: E402


def main():
    exe = sys.argv[1] if len(sys.argv) > 1 else find_interpreter()
    if not exe:
        print("Interpreter not found. Build it first (see DOCUMENTATION.md).")
        return 1
    ids = [lesson["id"] for lesson in LESSONS]
    assert len(ids) == len(set(ids)), "lesson ids must be unique"
    failures = 0
    for lesson in LESSONS:
        with tempfile.NamedTemporaryFile("w", suffix=".bc", delete=False, encoding="utf-8") as f:
            f.write(lesson["solution"])
        result = subprocess.run([exe, f.name], capture_output=True, text=True)
        os.unlink(f.name)
        passed, message = check_answer(lesson, lesson["solution"], result.stdout + result.stderr, result.returncode)
        print(("PASS " if passed else "FAIL ") + lesson["title"] + ("" if passed else "\n     " + message))
        failures += not passed
    print(f"{len(LESSONS) - failures}/{len(LESSONS)} lessons pass")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
