"""Broadcast Studio: a friendly editor for writing and running broadcast_language programs.

Start it with:   python ide/broadcast_ide.py      (or double-click ide/start_studio.bat on Windows)

It needs only Python 3 (tkinter comes with it) and the built interpreter (see DOCUMENTATION.md).
If the interpreter has not been built yet, Broadcast Studio offers to build it.
Programs are always run by the real interpreter, so what you see here is exactly what
`broadcast_language file.bc` prints.

Parts of the window:
  editor        syntax colours, line numbers, auto-indent, auto-closing brackets
  Output        what the program printed; errors are red, explained, and clickable
  Reference     every keyword with an example you can insert
"""
import bisect
import codecs
import difflib
import glob
import json
import os
import queue
import re
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from tkinter import font as tkfont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from broadcast_paths import DOCUMENTATION, EXAMPLES_DIR, PROJECT_DIR, find_interpreter  # noqa: E402

APP_NAME = "Broadcast Studio"
SETTINGS_FILE = os.path.join(os.path.expanduser("~"), ".broadcast_studio.json")
MAX_OUTPUT_CHARS = 200_000  # stop a program that prints forever (usually an endless repeat loop)
SLOW_RUN_SECONDS = 4        # after this long, remind the learner about the Stop button
NO_WINDOW = 0x08000000 if sys.platform == "win32" else 0  # CREATE_NO_WINDOW: no console window flash

KEYWORDS = {"assign", "broadcast", "when", "otherwise", "repeat", "protocol", "report", "and", "or"}
LITERALS = {"affirm", "negative", "silence"}

# A dark "studio" theme.
C = {
    "bg": "#1b1d28", "panel": "#232635", "panel2": "#2b2f42", "border": "#363a52",
    "fg": "#e8e8f0", "muted": "#8a8fa8", "gutter": "#5d6280", "curline": "#22263a",
    "select": "#3d4466", "keyword": "#ff7ac6", "literal": "#bd93f9", "string": "#f1fa8c",
    "number": "#ffb86c", "comment": "#6c7393", "function": "#50fa7b", "error": "#ff6b6b",
    "error_bg": "#4a1f28", "tip": "#8be9fd", "ok": "#50fa7b", "accent": "#ff5555",
    "button": "#343954", "button_hover": "#41476a", "bracket": "#4b5378",
}

TOKEN_RE = re.compile(
    r'(?P<comment>//[^\n]*)|(?P<string>"(?:[^"\\\n]|\\.)*"?)'
    r'|(?P<number>\b\d+(?:\.\d+)?\b)|(?P<word>[A-Za-z_]\w*)')
CALL_AFTER_RE = re.compile(r"\s*\(")
ERROR_RE = re.compile(r"^\[line (\d+)\] (Syntax|Runtime) error: (.*)$")
SOURCE_LINE_RE = re.compile(r"^\s+\d*\s\|")

EXIT_MESSAGES = {
    64: "The interpreter was started the wrong way.",
    65: "Syntax error: nothing ran. Fix the line shown above and run again.",
    70: "Runtime error: the program stopped at the line shown above.",
    74: "The program file could not be opened.",
}

WELCOME_PROGRAM = """// Welcome to Broadcast Studio!
// Press Run (F5) to run this program. Then change something and run it again.

broadcast "Hello from broadcast_language!";

assign listeners = 3;
repeat (listeners <= 5) {
  broadcast "Listeners tuned in: " + listeners;
  listeners = listeners + 1;
}

protocol shout(message) {
  report message + "!!!";
}
broadcast shout("We are live");
"""

# (name, what it does, example) for the Reference tab.
REFERENCE = [
    ("broadcast", "Show a value on the screen.", 'broadcast "Hello!";'),
    ("assign", "Create a variable (a named box).", "assign score = 10;"),
    ("=", "Change a variable that already exists.", "score = score + 1;"),
    ("when / otherwise", "Run code only if something is true.",
     'when (score > 5) {\n  broadcast "High!";\n} otherwise {\n  broadcast "Low.";\n}'),
    ("repeat", "Run code again and again while something is true.",
     "assign i = 1;\nrepeat (i <= 3) {\n  broadcast i;\n  i = i + 1;\n}"),
    ("protocol / report", "Make a reusable function that gives back an answer.",
     "protocol square(n) {\n  report n * n;\n}\nbroadcast square(4);"),
    ("+  -  *  /  %", "Math. % is the remainder after dividing.", "broadcast (2 + 3) * 4 % 7;"),
    ("==  !=  <  <=  >  >=", "Compare two values; the answer is affirm or negative.", "broadcast 3 <= 4;"),
    ("and  or  !", "Combine yes-or-no values, or flip one.", "broadcast affirm and !negative;"),
    ("affirm  negative  silence", "Yes, no, and \"no value\".", "assign ready = affirm;"),
    ("\"text\"", "Text goes in double quotes. + joins text. \\n is a new line.",
     'broadcast "Score: " + 10;'),
    ("// comment", "A note for people. It is ignored when the program runs.", "// This explains my code"),
    ("{ }  scope", "Variables made inside braces disappear after them.",
     'assign x = "outside";\n{\n  assign x = "inside";\n  broadcast x;\n}\nbroadcast x;'),
]


def declared_names(source):
    """Every variable, protocol and parameter name the source creates."""
    names = set(re.findall(r"\b(?:assign|protocol)\s+([A-Za-z_]\w*)", source))
    for params in re.findall(r"\bprotocol\s+\w+\s*\(([^)]*)\)", source):
        names.update(re.findall(r"[A-Za-z_]\w*", params))
    return sorted(names)


def friendly_tip(message, line, source):
    """A beginner-friendly explanation of an interpreter error message, or None."""
    if message.startswith("Expect ';'"):
        return f"Every statement ends with ;   Check the end of line {line}, or the line just above it."
    if message.startswith("Expect expression"):
        return "Something is missing here, such as a number or a variable after an operator."
    if message.startswith("Expect ')'"):
        return "A ( was opened but never closed with )."
    if message.startswith("Expect '}'"):
        return "A { was opened but never closed with }. Every block needs both braces."
    if message.startswith("Expect '('"):
        return "when, repeat and protocol need parentheses, like:  when (x > 1) { ... }"
    if message.startswith("Expect '{'"):
        return "A protocol's body goes in braces:  protocol name(n) { report n; }"
    if message.startswith("Expect variable name"):
        return "Give the variable a name:  assign score = 10;"
    if message.startswith("Expect function name") or message.startswith("Expect parameter name"):
        return "Names start with a letter or _, like:  protocol double(n) { ... }"
    if message.startswith("Unterminated string"):
        return 'Text needs a closing double quote:  "like this"'
    if message.startswith("Unexpected character"):
        return "That symbol is not part of broadcast_language. The Reference tab lists the symbols you can use."
    if message.startswith("Invalid assignment target"):
        return "Only variables can be changed with =, like:  score = 5;   (use == to compare)"
    if message.startswith("Can't report from top-level"):
        return "report only works inside a protocol. Use broadcast to show a value."
    undefined = re.match(r"Undefined variable '(\w+)'", message)
    if undefined:
        name = undefined.group(1)
        close = difflib.get_close_matches(name, declared_names(source), n=1, cutoff=0.6)
        if close:
            return f"Did you mean '{close[0]}'? Names must be spelled exactly the same (capitals count)."
        return f"Create it first with  assign {name} = ...;  before you use it."
    if message.startswith("Division by zero"):
        return "You can't divide by zero. Check the number after / or %."
    if message.startswith("Operands must be numbers"):
        return "- * / % < > only work on numbers. Text can only be joined with +."
    if message.startswith("Operands of '+'"):
        return "+ adds two numbers or joins text. affirm, negative and silence can't be added."
    if message.startswith("Expected") and "argument" in message:
        return "Count the inputs: a call must pass exactly as many values as the protocol lists."
    if message.startswith("Can only call functions"):
        return "Only protocols can be called with ( ). To multiply, write * between the numbers."
    if message.startswith("Stack overflow"):
        return "A protocol kept calling itself forever. Add a stopping case:  when (n <= 1) { report 1; }"
    return None


def load_settings():
    try:
        with open(SETTINGS_FILE, encoding="utf-8") as f:
            data = json.load(f)
        return data if isinstance(data, dict) else {}
    except (OSError, ValueError):
        return {}


def save_settings(settings):
    try:
        with open(SETTINGS_FILE, "w", encoding="utf-8") as f:
            json.dump(settings, f, indent=2)
    except OSError:
        pass


def read_stream(proc, events):
    """Runs on a worker thread: forwards everything `proc` prints to `events`, then its exit code."""
    decoder = codecs.getincrementaldecoder("utf-8")(errors="replace")
    while True:
        chunk = proc.stdout.read1(4096)
        if not chunk:
            break
        events.put(("out", decoder.decode(chunk)))
    events.put(("done", proc.wait()))


DERIVED_FONTS = []  # (base font, derived font): derived fonts follow the base font's size when zooming


def derived_font(base, **options):
    """A copy of `base` with some options changed (tkfont.Font ignores options when given font=)."""
    font = tkfont.Font(font=base)
    font.configure(**options)
    DERIVED_FONTS.append((base, font))
    return font


def pick_font(candidates, size, **options):
    available = set(tkfont.families())
    family = next((name for name in candidates if name in available), candidates[-1])
    return tkfont.Font(family=family, size=size, **options)


def make_button(parent, text, command, primary=False, color=None):
    base = color or (C["accent"] if primary else C["button"])
    hover = "#ff7070" if primary and not color else C["button_hover"]
    button = tk.Button(parent, text=text, command=command, relief="flat", bd=0, cursor="hand2",
                       bg=base, fg="white" if primary else C["fg"], activebackground=hover,
                       activeforeground="white", disabledforeground="#c9a0a6" if primary else C["gutter"],
                       padx=12, pady=5, highlightthickness=0)
    button.bind("<Enter>", lambda e: button.configure(bg=hover) if button["state"] != "disabled" else None)
    button.bind("<Leave>", lambda e: button.configure(bg=base))
    return button


class CodeEditor(tk.Frame):
    """A text editor that knows broadcast_language: colours, line numbers, indenting and brackets."""

    PAIRS = {"(": ")", "{": "}", '"': '"'}

    def __init__(self, parent, code_font, on_change, on_cursor):
        super().__init__(parent, bg=C["bg"])
        self.font = code_font
        self.on_change = on_change
        self.on_cursor = on_cursor
        self.error_line = None
        self._highlight_job = None

        self.gutter = tk.Canvas(self, width=48, bg=C["panel"], highlightthickness=0, bd=0)
        self.text = tk.Text(self, undo=True, autoseparators=True, maxundo=-1, wrap="none", font=code_font,
                            bg=C["bg"], fg=C["fg"], insertbackground=C["fg"], selectbackground=C["select"],
                            inactiveselectbackground=C["select"], padx=10, pady=8, bd=0, highlightthickness=0,
                            insertwidth=2, tabs=(code_font.measure("  "),))
        yscroll = ttk.Scrollbar(self, orient="vertical", command=self.text.yview)
        xscroll = ttk.Scrollbar(self, orient="horizontal", command=self.text.xview)
        self.text.configure(yscrollcommand=lambda *a: (yscroll.set(*a), self.redraw_gutter()),
                            xscrollcommand=xscroll.set)
        self.gutter.grid(row=0, column=0, sticky="ns")
        self.text.grid(row=0, column=1, sticky="nsew")
        yscroll.grid(row=0, column=2, sticky="ns")
        xscroll.grid(row=1, column=1, sticky="ew")
        self.rowconfigure(0, weight=1)
        self.columnconfigure(1, weight=1)

        for tag in ("keyword", "literal", "string", "number", "comment", "function"):
            self.text.tag_configure(tag, foreground=C[tag])
        self.text.tag_configure("keyword", font=derived_font(code_font, weight="bold"))
        self.text.tag_configure("comment", font=derived_font(code_font, slant="italic"))
        self.text.tag_configure("curline", background=C["curline"])
        self.text.tag_configure("errorline", background=C["error_bg"])
        self.text.tag_configure("bracket", background=C["bracket"])
        self.text.tag_lower("curline")
        self.text.tag_raise("sel")

        t = self.text
        t.bind("<<Modified>>", self._on_modified)
        t.bind("<KeyPress>", self._on_key)
        t.bind("<Return>", self._on_return)
        t.bind("<KP_Enter>", self._on_return)
        t.bind("<Tab>", self._on_tab)
        t.bind("<Shift-Tab>", self._on_shift_tab)
        if t.tk.call("tk", "windowingsystem") == "x11":
            t.bind("<ISO_Left_Tab>", self._on_shift_tab)
        t.bind("<BackSpace>", self._on_backspace)
        t.bind("<Control-slash>", self.toggle_comment)
        t.bind("<Control-a>", self._select_all)
        t.bind("<Control-y>", lambda e: self._redo())
        t.bind("<Control-Shift-Z>", lambda e: self._redo())
        for event in ("<KeyRelease>", "<ButtonRelease-1>", "<FocusIn>"):
            t.bind(event, lambda e: self._cursor_moved(), add="+")
        t.bind("<Configure>", lambda e: self.redraw_gutter())
        self.gutter.bind("<Button-1>", self._gutter_click)

    # ---- content ----
    def get_code(self):
        return self.text.get("1.0", "end-1c")

    def set_code(self, code):
        self.text.delete("1.0", "end")
        self.text.insert("1.0", code)
        self.text.edit_reset()
        self.text.mark_set("insert", "1.0")
        self.text.see("1.0")
        self.set_error_line(None)
        self.highlight()
        self._cursor_moved()

    def insert_snippet(self, snippet):
        """Inserts `snippet` on its own line(s) at the cursor, indented like the current line."""
        t = self.text
        line_before = t.get("insert linestart", "insert")
        indent = re.match(r"[ \t]*", line_before).group()
        body = snippet.replace("\n", "\n" + indent)
        if line_before.strip():
            t.mark_set("insert", "insert lineend")
            body = "\n" + indent + body
        t.edit_separator()
        t.insert("insert", body + ("\n" + indent if t.get("insert") not in ("", "\n") else ""))
        t.edit_separator()
        t.see("insert")
        t.focus_set()

    def goto_line(self, line):
        self.text.mark_set("insert", f"{line}.0")
        self.text.see(f"{line}.0")
        self.text.focus_set()
        self._cursor_moved()

    def set_error_line(self, line):
        self.text.tag_remove("errorline", "1.0", "end")
        self.error_line = line
        if line:
            self.text.tag_add("errorline", f"{line}.0", f"{line}.0 lineend+1c")
            self.text.see(f"{line}.0")
        self.redraw_gutter()

    # ---- colours ----
    def highlight(self):
        self._highlight_job = None
        t = self.text
        code = self.get_code()
        for tag in ("keyword", "literal", "string", "number", "comment", "function"):
            t.tag_remove(tag, "1.0", "end")
        line_starts = [0] + [m.end() for m in re.finditer("\n", code)]

        def index(offset):
            line = bisect.bisect_right(line_starts, offset) - 1
            return f"{line + 1}.{offset - line_starts[line]}"

        previous_word = None
        for match in TOKEN_RE.finditer(code):
            kind = match.lastgroup
            tag = kind
            if kind == "word":
                word = match.group()
                if word in KEYWORDS:
                    tag = "keyword"
                elif word in LITERALS:
                    tag = "literal"
                elif previous_word == "protocol" or CALL_AFTER_RE.match(code, match.end()):
                    tag = "function"
                else:
                    tag = None
                previous_word = word
            else:
                previous_word = None
            if tag:
                t.tag_add(tag, index(match.start()), index(match.end()))

    def _schedule_highlight(self):
        if self._highlight_job:
            self.after_cancel(self._highlight_job)
        self._highlight_job = self.after(60, self.highlight)

    # ---- line numbers ----
    def redraw_gutter(self):
        g, t = self.gutter, self.text
        g.delete("all")
        last_line = int(t.index("end-1c").split(".")[0])
        current = int(t.index("insert").split(".")[0])
        width = max(2, len(str(last_line))) * self.font.measure("9") + 26
        if int(g.cget("width")) != width:
            g.configure(width=width)
        index = t.index("@0,0")
        while True:
            info = t.dlineinfo(index)
            if info is None:
                break
            y, height = info[1], info[3]
            number = int(index.split(".")[0])
            if number == self.error_line:
                middle = y + height / 2
                g.create_oval(5, middle - 4, 13, middle + 4, fill=C["error"], outline="")
            g.create_text(width - 10, y, anchor="ne", text=str(number), font=self.font,
                          fill=C["fg"] if number == current else C["gutter"])
            following = t.index(f"{index}+1line")
            if following == index:
                break
            index = following

    def _gutter_click(self, event):
        index = self.text.index(f"@0,{event.y}")
        line = index.split(".")[0]
        self.text.tag_remove("sel", "1.0", "end")
        self.text.tag_add("sel", f"{line}.0", f"{line}.0+1line")
        self.text.mark_set("insert", f"{line}.0")
        self.text.focus_set()
        self._cursor_moved()

    # ---- events ----
    def _on_modified(self, _event):
        if not self.text.edit_modified():
            return
        self.text.edit_modified(False)
        if self.error_line:
            self.set_error_line(None)
        self._schedule_highlight()
        self.redraw_gutter()
        self.on_change()

    def _cursor_moved(self):
        t = self.text
        t.tag_remove("curline", "1.0", "end")
        t.tag_add("curline", "insert linestart", "insert linestart+1line")
        self._match_brackets()
        self.redraw_gutter()
        line, column = t.index("insert").split(".")
        self.on_cursor(int(line), int(column) + 1)

    def _match_brackets(self):
        t = self.text
        t.tag_remove("bracket", "1.0", "end")
        code = TOKEN_RE.sub(lambda m: m.group() if m.lastgroup in ("word", "number") else " " * len(m.group()),
                            self.get_code())
        cursor = len(t.get("1.0", "insert"))
        openers, closers = "({", ")}"
        for position in (cursor - 1, cursor):
            if 0 <= position < len(code) and code[position] in openers + closers:
                char = code[position]
                if char in openers:
                    partner, step, depth_char = closers[openers.index(char)], 1, char
                else:
                    partner, step, depth_char = openers[closers.index(char)], -1, char
                depth, i = 0, position
                while 0 <= i < len(code):
                    if code[i] == depth_char:
                        depth += 1
                    elif code[i] == partner:
                        depth -= 1
                        if depth == 0:
                            for p in (position, i):
                                t.tag_add("bracket", f"1.0+{p}c", f"1.0+{p + 1}c")
                            return
                    i += step
                return

    def _on_key(self, event):
        t, char = self.text, event.char
        if not char or event.state & 0x4:  # nothing typed, or Ctrl is held
            return None
        selection = t.tag_ranges("sel")
        if char in (")", "}", '"') and not selection and t.get("insert") == char:
            t.mark_set("insert", "insert+1c")  # type over the closing character added automatically
            self._cursor_moved()
            return "break"
        if char == "}":
            line_before = t.get("insert linestart", "insert")
            if line_before.endswith("  ") and not line_before.strip():
                t.delete("insert-2c", "insert")
        if char in self.PAIRS:
            if selection:
                inner = t.get("sel.first", "sel.last")
                t.edit_separator()
                t.delete("sel.first", "sel.last")
                t.insert("insert", char + inner + self.PAIRS[char])
                t.edit_separator()
                return "break"
            following = t.get("insert")
            line_before = t.get("insert linestart", "insert")
            inside_text = char == '"' and (line_before.count('"') % 2 == 1 or line_before[-1:].isalnum())
            if not inside_text and following in ("", "\n", " ", "\t", ")", "}", ";", ","):
                t.insert("insert", char + self.PAIRS[char])
                t.mark_set("insert", "insert-1c")
                self._cursor_moved()
                return "break"
        return None

    def _on_return(self, _event):
        t = self.text
        if t.tag_ranges("sel"):
            t.delete("sel.first", "sel.last")
        line_before = t.get("insert linestart", "insert")
        indent = re.match(r"[ \t]*", line_before).group()
        t.edit_separator()
        if line_before.rstrip().endswith("{"):
            inner = indent + "  "
            if t.get("insert") == "}":
                t.insert("insert", "\n" + inner + "\n" + indent)
                t.mark_set("insert", f"insert-{len(indent) + 1}c")
            else:
                t.insert("insert", "\n" + inner)
        else:
            t.insert("insert", "\n" + indent)
        t.edit_separator()
        t.see("insert")
        self._cursor_moved()
        return "break"

    def _selected_lines(self):
        t = self.text
        if t.tag_ranges("sel"):
            first = int(t.index("sel.first").split(".")[0])
            last_index = t.index("sel.last")
            last = int(last_index.split(".")[0])
            if last_index.endswith(".0") and last > first:
                last -= 1
            return first, last
        line = int(t.index("insert").split(".")[0])
        return line, line

    def _on_tab(self, _event):
        t = self.text
        if t.tag_ranges("sel"):
            first, last = self._selected_lines()
            for line in range(first, last + 1):
                t.insert(f"{line}.0", "  ")
        else:
            t.insert("insert", "  ")
        return "break"

    def _on_shift_tab(self, _event):
        t = self.text
        first, last = self._selected_lines()
        for line in range(first, last + 1):
            leading = len(re.match(r" {0,2}", t.get(f"{line}.0", f"{line}.2")).group())
            if leading:
                t.delete(f"{line}.0", f"{line}.{leading}")
        return "break"

    def _on_backspace(self, _event):
        t = self.text
        if t.tag_ranges("sel"):
            return None
        before, after = t.get("insert-1c"), t.get("insert")
        if before in self.PAIRS and self.PAIRS[before] == after:
            t.delete("insert-1c", "insert+1c")  # delete an empty pair together
            return "break"
        line_before = t.get("insert linestart", "insert")
        if len(line_before) >= 2 and not line_before.strip() and len(line_before) % 2 == 0:
            t.delete("insert-2c", "insert")  # remove one indent level
            return "break"
        return None

    def toggle_comment(self, _event=None):
        t = self.text
        first, last = self._selected_lines()
        lines = [t.get(f"{n}.0", f"{n}.0 lineend") for n in range(first, last + 1)]
        commented = all(line.lstrip().startswith("//") for line in lines if line.strip())
        t.edit_separator()
        for number, line in zip(range(first, last + 1), lines):
            if not line.strip():
                continue
            indent = len(line) - len(line.lstrip())
            if commented:
                remove = 3 if line[indent:indent + 3] == "// " else 2
                t.delete(f"{number}.{indent}", f"{number}.{indent + remove}")
            else:
                t.insert(f"{number}.{indent}", "// ")
        t.edit_separator()
        return "break"

    def _select_all(self, _event=None):
        self.text.tag_add("sel", "1.0", "end-1c")
        return "break"

    def _redo(self):
        try:
            self.text.edit_redo()
        except tk.TclError:
            pass
        return "break"


class OutputView(tk.Frame):
    """Shows what a program printed: errors in red with a beginner tip, and clickable line numbers."""

    def __init__(self, parent, code_font, ui_font, on_error=None, on_error_click=None):
        super().__init__(parent, bg=C["bg"])
        self.on_error = on_error
        self.on_error_click = on_error_click
        self.pending = ""
        self.pending_tip = None
        self.text = tk.Text(self, wrap="word", font=code_font, bg=C["bg"], fg=C["fg"], bd=0,
                            highlightthickness=0, padx=12, pady=8, state="disabled",
                            selectbackground=C["select"], cursor="arrow")
        scroll = ttk.Scrollbar(self, orient="vertical", command=self.text.yview)
        self.text.configure(yscrollcommand=scroll.set)
        self.text.pack(side="left", fill="both", expand=True)
        scroll.pack(side="right", fill="y")
        t = self.text
        t.tag_configure("error", foreground=C["error"], font=derived_font(code_font, weight="bold"))
        t.tag_configure("source", foreground="#d99aa0")
        t.tag_configure("tip", foreground=C["tip"], font=ui_font, lmargin1=8, lmargin2=8,
                        spacing1=2, spacing3=4)
        t.tag_configure("ok", foreground=C["ok"], font=ui_font)
        t.tag_configure("bad", foreground=C["error"], font=ui_font)
        t.tag_configure("info", foreground=C["muted"], font=ui_font)
        t.tag_configure("echo", foreground=C["keyword"])
        if on_error_click:
            t.tag_configure("link", underline=True)
            t.tag_bind("link", "<Button-1>", self._clicked)
            t.tag_bind("link", "<Enter>", lambda e: t.configure(cursor="hand2"))
            t.tag_bind("link", "<Leave>", lambda e: t.configure(cursor="arrow"))

    def clear(self):
        self.pending, self.pending_tip = "", None
        self.text.configure(state="normal")
        self.text.delete("1.0", "end")
        self.text.configure(state="disabled")

    def write(self, text, *tags):
        self.text.configure(state="normal")
        self.text.insert("end", text, tags)
        self.text.configure(state="disabled")
        self.text.see("end")

    def write_program_output(self, chunk):
        self.pending += chunk
        *lines, self.pending = self.pending.split("\n")
        for line in lines:
            self._write_line(line.rstrip("\r"))

    def flush(self):
        if self.pending:
            self._write_line(self.pending.rstrip("\r"))
            self.pending = ""
        self._write_tip()

    def _write_line(self, line):
        error = ERROR_RE.match(line)
        if SOURCE_LINE_RE.match(line):
            self.write(line + "\n", "source")
            return
        self._write_tip()
        if error:
            self.write(line + "\n", *(("error", "link") if self.on_error_click else ("error",)))
            if self.on_error:
                self.pending_tip = self.on_error(int(error.group(1)), error.group(2), error.group(3))
        else:
            self.write(line + "\n")

    def _write_tip(self):
        if self.pending_tip:
            self.write("Tip: " + self.pending_tip + "\n", "tip")
            self.pending_tip = None

    def _clicked(self, event):
        index = self.text.index(f"@{event.x},{event.y}")
        match = ERROR_RE.match(self.text.get(f"{index} linestart", f"{index} lineend"))
        if match:
            self.on_error_click(int(match.group(1)))


class ReferencePanel(tk.Frame):
    """Every keyword and symbol, each with an example that can be inserted into the editor."""

    def __init__(self, parent, app):
        super().__init__(parent, bg=C["panel"])
        text = tk.Text(self, wrap="word", font=app.ui_font, bg=C["panel"], fg=C["fg"], bd=0,
                       highlightthickness=0, padx=14, pady=10, cursor="arrow")
        scroll = ttk.Scrollbar(self, orient="vertical", command=text.yview)
        text.configure(yscrollcommand=scroll.set)
        text.pack(side="left", fill="both", expand=True)
        scroll.pack(side="right", fill="y")
        text.tag_configure("h1", font=app.title_font, spacing3=4)
        text.tag_configure("name", font=derived_font(app.code_font, weight="bold"), foreground=C["keyword"],
                           spacing1=12)
        text.tag_configure("desc", foreground=C["muted"], spacing1=2, spacing3=4)
        text.tag_configure("block", font=app.code_font, background=C["bg"], lmargin1=10, lmargin2=10)
        text.insert("end", "Quick reference\n", "h1")
        text.insert("end", "broadcast_language has just 12 keywords. Press Insert to add an example "
                           "at the cursor.\n", "desc")
        for name, description, example in REFERENCE:
            text.insert("end", name + "   ", "name")
            text.window_create("end", window=make_button(text, "Insert", lambda s=example: app.editor.insert_snippet(s)))
            text.insert("end", "\n" + description + "\n", "desc")
            text.insert("end", example + "\n", "block")
        text.configure(state="disabled")


class BroadcastStudio:
    def __init__(self, root):
        self.root = root
        self.settings = load_settings()
        self.current_file = None
        self.untitled_name = "untitled.bc"
        self.saved_code = ""
        self.run_proc = None
        self.run_events = queue.Queue()
        self.building = False
        self.on_air_job = None
        self.run = {}

        size = int(self.settings.get("font_size", 12))
        self.code_font = pick_font(["Cascadia Code", "Consolas", "Menlo", "DejaVu Sans Mono", "Courier New"], size)
        self.ui_font = pick_font(["Segoe UI", "Helvetica Neue", "Helvetica", "Arial"], 10)
        self.title_font = pick_font(["Segoe UI Semibold", "Segoe UI", "Helvetica", "Arial"], 13, weight="bold")
        root.option_add("*Font", self.ui_font)
        self._style()

        root.title(APP_NAME)
        root.configure(bg=C["panel"])
        root.geometry(self.settings.get("geometry", "1280x800"))
        root.minsize(900, 560)

        self._toolbar()
        self._status_bar()
        outer = tk.PanedWindow(root, orient="horizontal", bg=C["border"], sashwidth=5, bd=0, showhandle=False)
        outer.pack(fill="both", expand=True)
        left = tk.PanedWindow(outer, orient="vertical", bg=C["border"], sashwidth=5, bd=0)
        self.editor = CodeEditor(left, self.code_font, self._code_changed, self._cursor_changed)
        bottom = ttk.Notebook(left, style="Studio.TNotebook")
        self.output = OutputView(bottom, self.code_font, self.ui_font, on_error=self._program_error,
                                 on_error_click=self.editor.goto_line)
        bottom.add(self.output, text="Output")
        left.add(self.editor, minsize=200, stretch="always")
        left.add(bottom, minsize=120, height=250, stretch="never")
        side = ttk.Notebook(outer, style="Studio.TNotebook")
        side.add(ReferencePanel(side, self), text="Reference")
        outer.add(left, minsize=450, stretch="always")
        outer.add(side, minsize=280, width=380, stretch="never")
        self.bottom_tabs = bottom
        # A fixed pixel height is tiny on high-resolution screens, so give the output 40% of the height.
        root.after(150, lambda: left.sash_place(0, 0, int(left.winfo_height() * 0.6)))

        self._menus()
        self._shortcuts()
        root.protocol("WM_DELETE_WINDOW", self.quit)

        last = self.settings.get("last_file")
        if last and os.path.isfile(last):
            self.open_path(last)
        else:
            self.replace_code(WELCOME_PROGRAM, "welcome.bc", ask=False)
        self.output.write("Ready. Press Run (F5) to run the program in the editor.\n", "info")
        self._update_interpreter_status()
        self.editor.text.focus_set()
        if not self.settings.get("welcomed"):
            root.after(300, self.welcome)

    # ---- look ----
    def _style(self):
        style = ttk.Style(self.root)
        style.theme_use("clam")
        style.configure(".", background=C["panel"], foreground=C["fg"], bordercolor=C["border"],
                        lightcolor=C["panel"], darkcolor=C["panel"], troughcolor=C["bg"])
        style.configure("Studio.TNotebook", background=C["panel"], borderwidth=0, tabmargins=(6, 6, 0, 0))
        style.configure("Studio.TNotebook.Tab", background=C["panel"], foreground=C["muted"], padding=(14, 6),
                        borderwidth=0, font=self.ui_font)
        style.map("Studio.TNotebook.Tab", background=[("selected", C["bg"])],
                  foreground=[("selected", C["fg"])])
        style.configure("TScrollbar", background=C["panel2"], troughcolor=C["bg"], arrowcolor=C["muted"],
                        bordercolor=C["bg"], gripcount=0)
        style.map("TScrollbar", background=[("active", C["button_hover"])])

    def _toolbar(self):
        bar = tk.Frame(self.root, bg=C["panel2"])
        bar.pack(side="top", fill="x")
        tk.Label(bar, text="  ((•))  Broadcast Studio", font=self.title_font, bg=C["panel2"],
                 fg=C["fg"]).pack(side="left", padx=(6, 16), pady=8)
        for label, command in (("New", self.new_file), ("Open", self.open_file), ("Save", self.save)):
            make_button(bar, label, command).pack(side="left", padx=2)
        examples = tk.Menubutton(bar, text="Examples ▾", relief="flat", bg=C["button"], fg=C["fg"],
                                 activebackground=C["button_hover"], activeforeground=C["fg"], padx=12, pady=5,
                                 cursor="hand2", bd=0, highlightthickness=0)
        examples.menu = self._examples_menu(examples)
        examples.configure(menu=examples.menu)
        examples.pack(side="left", padx=(12, 2))
        self.run_button = make_button(bar, "▶  Run   F5", self.run_program, primary=True)
        self.run_button.pack(side="left", padx=(16, 2))
        self.stop_button = make_button(bar, "■  Stop", self.stop_program)
        self.stop_button.pack(side="left", padx=2)
        self.stop_button.configure(state="disabled")
        self.on_air = tk.Label(bar, text="  ●  ON AIR  ", font=self.ui_font, bg=C["panel"], fg=C["gutter"],
                               padx=6, pady=3)
        self.on_air.pack(side="right", padx=12)

    def _status_bar(self):
        bar = tk.Frame(self.root, bg=C["panel"])
        bar.pack(side="bottom", fill="x")
        self.file_label = tk.Label(bar, bg=C["panel"], fg=C["muted"], font=self.ui_font)
        self.file_label.pack(side="left", padx=10, pady=3)
        self.interpreter_label = tk.Label(bar, bg=C["panel"], fg=C["muted"], font=self.ui_font)
        self.interpreter_label.pack(side="right", padx=10)
        self.runs_label = tk.Label(bar, bg=C["panel"], fg=C["muted"], font=self.ui_font)
        self.runs_label.pack(side="right", padx=10)
        self.cursor_label = tk.Label(bar, bg=C["panel"], fg=C["muted"], font=self.ui_font)
        self.cursor_label.pack(side="right", padx=10)
        self._update_runs()

    def _examples_menu(self, parent):
        menu = tk.Menu(parent, tearoff=False)
        paths = sorted(glob.glob(os.path.join(EXAMPLES_DIR, "*.bc")))
        for path in paths:
            menu.add_command(label=os.path.basename(path), command=lambda p=path: self.open_example(p))
        if not paths:
            menu.add_command(label="(no examples found)", state="disabled")
        return menu

    def _menus(self):
        bar = tk.Menu(self.root)
        file_menu = tk.Menu(bar, tearoff=False)
        file_menu.add_command(label="New", accelerator="Ctrl+N", command=self.new_file)
        file_menu.add_command(label="Open…", accelerator="Ctrl+O", command=self.open_file)
        file_menu.add_command(label="Save", accelerator="Ctrl+S", command=self.save)
        file_menu.add_command(label="Save As…", accelerator="Ctrl+Shift+S", command=self.save_as)
        file_menu.add_cascade(label="Examples", menu=self._examples_menu(file_menu))
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.quit)
        bar.add_cascade(label="File", menu=file_menu)

        edit_menu = tk.Menu(bar, tearoff=False)
        edit_menu.add_command(label="Undo", accelerator="Ctrl+Z", command=lambda: self.editor.text.event_generate("<<Undo>>"))
        edit_menu.add_command(label="Redo", accelerator="Ctrl+Y", command=self.editor._redo)
        edit_menu.add_separator()
        edit_menu.add_command(label="Comment / uncomment lines", accelerator="Ctrl+/", command=self.editor.toggle_comment)
        edit_menu.add_separator()
        edit_menu.add_command(label="Bigger text", accelerator="Ctrl+=", command=lambda: self.zoom(1))
        edit_menu.add_command(label="Smaller text", accelerator="Ctrl+-", command=lambda: self.zoom(-1))
        bar.add_cascade(label="Edit", menu=edit_menu)

        run_menu = tk.Menu(bar, tearoff=False)
        run_menu.add_command(label="Run", accelerator="F5", command=self.run_program)
        run_menu.add_command(label="Stop", accelerator="Shift+F5", command=self.stop_program)
        run_menu.add_separator()
        run_menu.add_command(label="Build interpreter", command=self.build_interpreter)
        bar.add_cascade(label="Run", menu=run_menu)

        help_menu = tk.Menu(bar, tearoff=False)
        help_menu.add_command(label="Welcome tour", command=self.welcome)
        help_menu.add_command(label="Language documentation", command=self.open_documentation)
        help_menu.add_command(label="Keyboard shortcuts", command=self.shortcuts_help)
        bar.add_cascade(label="Help", menu=help_menu)
        self.root.configure(menu=bar)

    def _shortcuts(self):
        shortcuts = {
            "<F5>": self.run_program, "<Control-Return>": self.run_program, "<Shift-F5>": self.stop_program,
            "<Control-n>": self.new_file, "<Control-o>": self.open_file, "<Control-s>": self.save,
            "<Control-S>": self.save_as, "<Control-equal>": lambda: self.zoom(1),
            "<Control-plus>": lambda: self.zoom(1), "<Control-KP_Add>": lambda: self.zoom(1),
            "<Control-minus>": lambda: self.zoom(-1), "<Control-KP_Subtract>": lambda: self.zoom(-1),
            "<Control-0>": lambda: self.zoom(12 - self.code_font.cget("size")),
        }
        for sequence, action in shortcuts.items():
            handler = lambda _event, action=action: (action(), "break")[1]
            self.root.bind(sequence, handler)
            self.editor.text.bind(sequence, handler)

    # ---- files ----
    def display_name(self):
        return os.path.basename(self.current_file) if self.current_file else self.untitled_name

    def is_dirty(self):
        return self.editor.get_code() != self.saved_code

    def _update_title(self):
        dirty = self.is_dirty()
        self.root.title(("● " if dirty else "") + f"{self.display_name()} - {APP_NAME}")
        where = self.current_file or "not saved yet"
        self.file_label.configure(text=f"{self.display_name()}{'  •  unsaved changes' if dirty else ''}   ({where})")

    def _code_changed(self):
        self._update_title()

    def _cursor_changed(self, line, column):
        self.cursor_label.configure(text=f"Ln {line}, Col {column}")

    def confirm_discard(self):
        """True if it is fine to replace the editor's contents (after saving them if asked)."""
        if not self.is_dirty() or not self.editor.get_code().strip():
            return True
        answer = messagebox.askyesnocancel(APP_NAME, f"Save your changes to {self.display_name()} first?")
        if answer is None:
            return False
        return self.save() if answer else True

    def replace_code(self, code, name, ask=True):
        if ask and not self.confirm_discard():
            return False
        self.current_file = None
        self.untitled_name = name
        self.editor.set_code(code)
        self.saved_code = code
        self._update_title()
        return True

    def new_file(self):
        if self.replace_code("", "untitled.bc"):
            self.editor.text.focus_set()

    def open_example(self, path):
        if not self.confirm_discard():
            return
        with open(path, encoding="utf-8", errors="replace") as f:
            code = f.read().replace("\r\n", "\n")
        self.replace_code(code, os.path.basename(path).replace(".bc", " (example).bc"), ask=False)
        self.output.write(f"Loaded the example {os.path.basename(path)}. Press Run to see what it does.\n", "info")

    def open_file(self):
        if not self.confirm_discard():
            return
        start = os.path.dirname(self.current_file) if self.current_file else PROJECT_DIR
        path = filedialog.askopenfilename(title="Open a broadcast_language program", initialdir=start,
                                          filetypes=[("broadcast_language programs", "*.bc"), ("All files", "*.*")])
        if path:
            self.open_path(path)

    def open_path(self, path):
        try:
            with open(path, encoding="utf-8", errors="replace") as f:
                code = f.read().replace("\r\n", "\n")
        except OSError as error:
            messagebox.showerror(APP_NAME, f"Could not open {path}:\n{error}")
            return
        self.current_file = os.path.abspath(path)
        self.editor.set_code(code)
        self.saved_code = code
        self.settings["last_file"] = self.current_file
        self._update_title()

    def save(self):
        if not self.current_file:
            return self.save_as()
        try:
            with open(self.current_file, "w", encoding="utf-8", newline="") as f:
                f.write(self.editor.get_code())
        except OSError as error:
            messagebox.showerror(APP_NAME, f"Could not save {self.current_file}:\n{error}")
            return False
        self.saved_code = self.editor.get_code()
        self.settings["last_file"] = self.current_file
        self._update_title()
        return True

    def save_as(self):
        name = self.untitled_name.replace(" (example)", "").replace(" ", "_")
        path = filedialog.asksaveasfilename(title="Save program", defaultextension=".bc", initialfile=name,
                                            initialdir=os.path.dirname(self.current_file or "") or PROJECT_DIR,
                                            filetypes=[("broadcast_language programs", "*.bc"), ("All files", "*.*")])
        if not path:
            return False
        self.current_file = os.path.abspath(path)
        return self.save()

    # ---- running programs ----
    def interpreter(self):
        """The interpreter's path; if it is missing, offers to build it and returns None."""
        exe = find_interpreter()
        if exe:
            return exe
        if self.building:
            messagebox.showinfo(APP_NAME, "The interpreter is still being built. Try again in a moment.")
        elif messagebox.askyesno(APP_NAME, "The broadcast_language interpreter has not been built yet.\n\n"
                                 "Build it now? It takes about a minute."):
            self.build_interpreter()
        return None

    def run_program(self, on_finish=None):
        """Runs the editor's code; returns False if it could not start."""
        if self.run_proc:
            return False
        exe = self.interpreter()
        if not exe:
            return False
        code = self.editor.get_code()
        handle, path = tempfile.mkstemp(suffix=".bc", prefix="broadcast_studio_")
        with os.fdopen(handle, "w", encoding="utf-8", newline="") as f:
            f.write(code)
        self.run = {"code": code, "path": path, "started": time.time(), "chars": 0, "output": [],
                    "stopped": None, "first_error": None, "on_finish": on_finish, "warned": False}
        self.bottom_tabs.select(0)
        self.output.clear()
        self.editor.set_error_line(None)
        self.output.write(f"▶ Running {self.display_name()}\n", "info")
        self.run_proc = subprocess.Popen([exe, path], stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT, cwd=PROJECT_DIR, creationflags=NO_WINDOW)
        threading.Thread(target=read_stream, args=(self.run_proc, self.run_events), daemon=True).start()
        self.run_button.configure(state="disabled")
        self.stop_button.configure(state="normal")
        self._set_on_air(True)
        self._poll_run()
        return True

    def _poll_run(self):
        run = self.run
        while True:
            try:
                kind, value = self.run_events.get_nowait()
            except queue.Empty:
                break
            if kind == "done":
                self._finish_run(value)
                return
            if run["stopped"]:
                continue
            run["output"].append(value)
            run["chars"] += len(value)
            self.output.write_program_output(value)
            if run["chars"] > MAX_OUTPUT_CHARS:
                run["stopped"] = "limit"
                self.run_proc.kill()
        if not run["warned"] and time.time() - run["started"] > SLOW_RUN_SECONDS:
            run["warned"] = True
            self.output.write("\n(Still running… If a repeat loop never ends, press Stop.)\n", "info")
        self.root.after(25, self._poll_run)

    def _finish_run(self, exit_code):
        run = self.run
        self.run_proc = None
        self.output.flush()
        try:
            os.remove(run["path"])
        except OSError:
            pass
        elapsed = time.time() - run["started"]
        if run["stopped"] == "user":
            self.output.write("\n■ Stopped.\n", "info")
        elif run["stopped"] == "limit":
            self.output.write("\n■ Stopped: the program printed far too much. Does a repeat loop forget to "
                              "change its counter?\n", "bad")
        elif exit_code == 0:
            self.output.write(f"\n✓ Finished in {elapsed:.2f} s\n", "ok")
            self.output.text.yview_moveto(0)  # read from the first line; errors stay scrolled to the end
        else:
            self.output.write("\n✗ " + EXIT_MESSAGES.get(exit_code, f"Exited with code {exit_code}.") + "\n", "bad")
        self.run_button.configure(state="normal")
        self.stop_button.configure(state="disabled")
        self._set_on_air(False)
        self.settings["runs"] = int(self.settings.get("runs", 0)) + 1
        self._update_runs()
        if self.settings["runs"] in (10, 50, 100, 250, 500, 1000):
            self.output.write(f"That was your {self.settings['runs']}th program run. Keep broadcasting!\n", "tip")
        if run["on_finish"]:
            run["on_finish"](run["code"], "".join(run["output"]), None if run["stopped"] else exit_code)

    def _program_error(self, line, _kind, message):
        if self.run.get("first_error") is None:
            self.run["first_error"] = line
            self.editor.set_error_line(line)
        return friendly_tip(message, line, self.run["code"])

    def stop_program(self):
        if self.run_proc:
            self.run["stopped"] = "user"
            self.run_proc.kill()

    def build_interpreter(self):
        if self.building:
            return
        cmake, gcc = shutil.which("cmake"), shutil.which("gcc") or shutil.which("clang")
        if cmake:
            commands = [[cmake, "-B", "build"], [cmake, "--build", "build", "--config", "Release"]]
        elif gcc:
            sources = sorted(glob.glob(os.path.join(PROJECT_DIR, "src", "*.c")))
            commands = [[gcc, "-std=c11", "-O2", *sources, "-lm", "-o", "broadcast_language"]]
        else:
            messagebox.showerror(APP_NAME, "No C compiler was found (CMake with Visual Studio, or gcc).\n\n"
                                 "Install one, then build the interpreter as shown in DOCUMENTATION.md, section 3.")
            return
        self.building = True
        self.bottom_tabs.select(0)
        self.output.clear()
        self.output.write("Building the broadcast_language interpreter…\n\n", "info")
        events = queue.Queue()

        def work():
            code = 0
            for command in commands:
                events.put(("out", "> " + " ".join(os.path.basename(command[0]) if i == 0 else part
                                                   for i, part in enumerate(command)) + "\n"))
                try:
                    proc = subprocess.Popen(command, cwd=PROJECT_DIR, stdout=subprocess.PIPE,
                                            stderr=subprocess.STDOUT, creationflags=NO_WINDOW)
                except OSError as error:
                    events.put(("out", f"{error}\n"))
                    code = 1
                    break
                for raw in proc.stdout:
                    events.put(("out", raw.decode("utf-8", "replace")))
                code = proc.wait()
                if code:
                    break
            events.put(("done", code))

        def poll():
            while True:
                try:
                    kind, value = events.get_nowait()
                except queue.Empty:
                    break
                if kind == "out":
                    self.output.write(value, "info")
                else:
                    self.building = False
                    self._update_interpreter_status()
                    if value == 0 and find_interpreter():
                        self.output.write("\n✓ The interpreter is ready. Press Run (F5)!\n", "ok")
                    else:
                        self.output.write("\n✗ The build failed. See the messages above, and "
                                          "DOCUMENTATION.md section 3.\n", "bad")
                    return
            self.root.after(50, poll)

        threading.Thread(target=work, daemon=True).start()
        poll()

    # ---- encouragement ----
    def _set_on_air(self, live):
        if self.on_air_job:
            self.root.after_cancel(self.on_air_job)
            self.on_air_job = None
        if live:
            self.on_air.configure(bg=C["accent"], fg="white", text="  ●  ON AIR  ")
        else:
            self.on_air.configure(bg=C["panel"], fg=C["gutter"], text="  ●  ON AIR  ")

    def _update_runs(self):
        runs = int(self.settings.get("runs", 0))
        self.runs_label.configure(text=f"Programs run: {runs}")

    def _update_interpreter_status(self):
        exe = find_interpreter()
        if exe:
            self.interpreter_label.configure(text="Interpreter: ready", fg=C["ok"])
        else:
            self.interpreter_label.configure(text="Interpreter: not built (Run ▸ Build interpreter)",
                                             fg=C["error"])

    def welcome(self):
        self.settings["welcomed"] = True
        self.remember()
        dialog = tk.Toplevel(self.root, bg=C["panel"])
        dialog.title("Welcome")
        dialog.transient(self.root)
        dialog.resizable(False, False)
        tk.Label(dialog, text="((•))", font=derived_font(self.title_font, size=28), bg=C["panel"],
                 fg=C["accent"]).pack(pady=(22, 0))
        tk.Label(dialog, text="Welcome to Broadcast Studio", font=self.title_font, bg=C["panel"],
                 fg=C["fg"]).pack(pady=(4, 10))
        tk.Label(dialog, justify="left", wraplength=440, bg=C["panel"], fg=C["fg"], font=self.ui_font, text=(
            "broadcast_language is a small, friendly programming language with just 12 keywords.\n\n"
            "•  Write code in the editor and press Run (F5).\n"
            "•  Mistakes are shown in red with a tip. Click one to jump to that line.\n"
            "•  The Reference tab lists every keyword with an example you can insert.")).pack(padx=28)
        buttons = tk.Frame(dialog, bg=C["panel"])
        buttons.pack(pady=20)
        make_button(buttons, "Get started", dialog.destroy, primary=True).pack(side="left", padx=6)
        dialog.update_idletasks()
        x = self.root.winfo_rootx() + (self.root.winfo_width() - dialog.winfo_width()) // 2
        y = self.root.winfo_rooty() + (self.root.winfo_height() - dialog.winfo_height()) // 3
        dialog.geometry(f"+{max(x, 0)}+{max(y, 0)}")
        dialog.grab_set()

    def shortcuts_help(self):
        messagebox.showinfo(APP_NAME, "F5 or Ctrl+Enter   Run\nShift+F5   Stop\nCtrl+S   Save\nCtrl+O   Open\n"
                            "Ctrl+N   New\nCtrl+/   Comment or uncomment lines\nTab / Shift+Tab   Indent / unindent\n"
                            "Ctrl+Z / Ctrl+Y   Undo / redo\nCtrl+= / Ctrl+-   Bigger / smaller text")

    def open_documentation(self):
        try:
            if sys.platform == "win32":
                os.startfile(DOCUMENTATION)
            else:
                subprocess.Popen(["open" if sys.platform == "darwin" else "xdg-open", DOCUMENTATION])
        except OSError:
            messagebox.showinfo(APP_NAME, f"The documentation is in:\n{DOCUMENTATION}")

    def zoom(self, step):
        size = max(8, min(28, self.code_font.cget("size") + step))
        self.code_font.configure(size=size)
        for base, font in DERIVED_FONTS:
            if base is self.code_font:
                font.configure(size=size)
        self.editor.text.configure(tabs=(self.code_font.measure("  "),))
        self.editor.redraw_gutter()
        self.settings["font_size"] = size
        return "break"

    # ---- closing ----
    def remember(self):
        save_settings(self.settings)

    def quit(self):
        if not self.confirm_discard():
            return
        self.stop_program()
        self.settings["geometry"] = self.root.geometry()
        save_settings(self.settings)
        self.root.destroy()


def main():
    if sys.platform == "win32":
        try:
            import ctypes
            ctypes.windll.shcore.SetProcessDpiAwareness(1)  # sharp text on high-resolution screens
        except (AttributeError, OSError):
            pass
    root = tk.Tk()
    BroadcastStudio(root)
    root.mainloop()


if __name__ == "__main__":
    main()
