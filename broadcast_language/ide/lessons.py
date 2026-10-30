"""Guided lessons for Broadcast Studio.

Each lesson teaches one idea, then gives the learner a mission. Broadcast Studio runs the
learner's code with the real interpreter and compares what it printed with `expected`.

Fields:
  id        stable key, used to remember which lessons are finished
  title     shown in the lesson list
  intro     the explanation; text inside `backticks` is shown as code
  example   a short program that shows the idea
  mission   what the learner must make the program do
  starter   code put in the editor when the lesson starts
  solution  one correct answer (shown on request; checked by test_lessons.py)
  expected  the exact output a correct program prints
  requires  words or symbols the code must use (outside comments and text), so the
            mission is solved with the idea being taught
  hint      a nudge for learners who are stuck
"""
import re

LESSONS = [
    {
        "id": "hello",
        "title": "1. Go on air",
        "intro": "Every broadcast_language program talks to the world with `broadcast`. "
                 "Put a value after it and end the line with `;`, like a period at the end of a sentence. "
                 "Text goes inside double quotes. Lines starting with `//` are notes for people and are ignored.",
        "example": '// A note for humans\nbroadcast "Good morning!";\nbroadcast 7;',
        "mission": "Broadcast the text Hello, world!",
        "starter": "// Write your first broadcast below, then press Run (F5).\n",
        "solution": 'broadcast "Hello, world!";\n',
        "expected": "Hello, world!",
        "requires": ["broadcast"],
        "hint": 'Type broadcast, a space, then "Hello, world!" in double quotes, and finish with ;',
    },
    {
        "id": "math",
        "title": "2. The calculator",
        "intro": "broadcast_language can do math with `+` `-` `*` `/` and `%` (the remainder). "
                 "It follows school rules: parentheses first, then `*` `/` `%`, then `+` `-`.",
        "example": "broadcast 2 + 3 * 4;     // 14\nbroadcast (2 + 3) * 4;   // 20\nbroadcast 7 / 2;         // 3.5",
        "mission": "A studio has 4 rooms with 6 microphones each, plus 3 spare microphones. "
                   "Broadcast the total number of microphones using one calculation.",
        "starter": "// 4 rooms with 6 mics each, plus 3 spares\n",
        "solution": "broadcast 4 * 6 + 3;\n",
        "expected": "27",
        "requires": ["broadcast", "*", "+"],
        "hint": "Multiply the rooms by the microphones, then add the spares: 4 * 6 + 3",
    },
    {
        "id": "variables",
        "title": "3. Labelled boxes",
        "intro": "A variable is a named box that holds a value. Create one with `assign`, "
                 "and change it later with `=`. You must `assign` a variable before you use it, "
                 "which catches spelling mistakes.",
        "example": "assign score = 10;\nscore = score + 5;\nbroadcast score;   // 15",
        "mission": "Create a variable called listeners with the value 120. "
                   "Then 30 more people tune in: add 30 to listeners. Broadcast listeners.",
        "starter": "",
        "solution": "assign listeners = 120;\nlisteners = listeners + 30;\nbroadcast listeners;\n",
        "expected": "150",
        "requires": ["assign", "listeners"],
        "hint": "Three lines: assign listeners = 120;  then  listeners = listeners + 30;  then broadcast it.",
    },
    {
        "id": "text",
        "title": "4. Joining text",
        "intro": "When one side of `+` is text, broadcast_language glues both sides together. "
                 "Numbers are turned into text automatically.",
        "example": 'assign name = "Ana";\nassign age = 20;\nbroadcast "Name: " + name + ", age: " + age;',
        "mission": 'Make a variable host set to "Ana" and a variable hour set to 8. '
                   "Use them to broadcast exactly: Ana goes live at 8 o'clock.",
        "starter": 'assign host = "Ana";\nassign hour = 8;\n',
        "solution": 'assign host = "Ana";\nassign hour = 8;\nbroadcast host + " goes live at " + hour + " o\'clock.";\n',
        "expected": "Ana goes live at 8 o'clock.",
        "requires": ["host", "hour", "+"],
        "hint": "Watch the spaces: host + \" goes live at \" + hour + \" o'clock.\"",
    },
    {
        "id": "compare",
        "title": "5. Yes or no?",
        "intro": "Comparisons answer yes-or-no questions with `affirm` (yes) or `negative` (no). "
                 "Use `==` for equal, `!=` for not equal, and `<` `<=` `>` `>=`. "
                 "Combine answers with `and`, `or`, and flip one with `!`.",
        "example": "broadcast 5 == 5;            // affirm\nbroadcast 2 > 8;             // negative\nbroadcast affirm and negative; // negative",
        "mission": "The starter makes volume equal to 7. Broadcast whether volume is greater than 5, "
                   "then broadcast whether volume is equal to 10.",
        "starter": "assign volume = 7;\n",
        "solution": "assign volume = 7;\nbroadcast volume > 5;\nbroadcast volume == 10;\n",
        "expected": "affirm\nnegative",
        "requires": ["volume", ">", "=="],
        "hint": "Two broadcasts: volume > 5, then volume == 10 (two equals signs to compare).",
    },
    {
        "id": "when",
        "title": "6. Making decisions",
        "intro": "`when` runs code only if its condition is true. The optional `otherwise` part runs "
                 "when it is false. The condition goes in `( )` and the code in `{ }`.",
        "example": 'assign temperature = 31;\nwhen (temperature > 30) {\n  broadcast "It\'s hot.";\n} otherwise {\n  broadcast "It\'s fine.";\n}',
        "mission": "The starter sets signal to 3. When signal is less than 5, broadcast Weak signal. "
                   "Otherwise, broadcast Strong signal.",
        "starter": "assign signal = 3;\n",
        "solution": 'assign signal = 3;\nwhen (signal < 5) {\n  broadcast "Weak signal";\n} otherwise {\n  broadcast "Strong signal";\n}\n',
        "expected": "Weak signal",
        "requires": ["when", "otherwise", "<"],
        "hint": "when (signal < 5) { ... } otherwise { ... }",
    },
    {
        "id": "repeat",
        "title": "7. Loops",
        "intro": "`repeat` runs its code again and again for as long as its condition is true. "
                 "Remember to change something inside the loop, or it will never stop! "
                 "(If that happens, press Stop.)",
        "example": "assign count = 1;\nrepeat (count <= 3) {\n  broadcast count;\n  count = count + 1;\n}",
        "mission": "Count down to the show: broadcast 5, 4, 3, 2, 1 (one per line) using repeat, "
                   "then broadcast On air!",
        "starter": "assign count = 5;\n",
        "solution": 'assign count = 5;\nrepeat (count >= 1) {\n  broadcast count;\n  count = count - 1;\n}\nbroadcast "On air!";\n',
        "expected": "5\n4\n3\n2\n1\nOn air!",
        "requires": ["repeat"],
        "hint": "repeat (count >= 1) { broadcast count; count = count - 1; } and then broadcast \"On air!\"; after the loop.",
    },
    {
        "id": "remainder",
        "title": "8. Loops that decide",
        "intro": "You can put a `when` inside a `repeat`. The remainder operator `%` is handy here: "
                 "`n % 2 == 0` is `affirm` exactly when n is even.",
        "example": "broadcast 10 % 3;   // 1\nbroadcast 8 % 2;    // 0, so 8 is even",
        "mission": "Broadcast every even number from 1 to 10 (that is 2, 4, 6, 8, 10). "
                   "Loop through all numbers 1 to 10 and use % to pick the even ones.",
        "starter": "assign n = 1;\n",
        "solution": "assign n = 1;\nrepeat (n <= 10) {\n  when (n % 2 == 0) {\n    broadcast n;\n  }\n  n = n + 1;\n}\n",
        "expected": "2\n4\n6\n8\n10",
        "requires": ["repeat", "when", "%"],
        "hint": "Inside repeat (n <= 10) { ... }, use when (n % 2 == 0) { broadcast n; } and then add 1 to n.",
    },
    {
        "id": "functions",
        "title": "9. Your own commands",
        "intro": "A function is a named piece of work you can reuse. Create one with `protocol`, give it "
                 "inputs in `( )`, and send back an answer with `report`.",
        "example": "protocol area(width, height) {\n  report width * height;\n}\nbroadcast area(5, 3);   // 15",
        "mission": "Write a function triple(n) that reports n times 3. "
                   "Then broadcast triple(5) and triple(14).",
        "starter": "",
        "solution": "protocol triple(n) {\n  report n * 3;\n}\nbroadcast triple(5);\nbroadcast triple(14);\n",
        "expected": "15\n42",
        "requires": ["protocol", "report", "triple"],
        "hint": "protocol triple(n) { report n * 3; }  then two broadcast lines.",
    },
    {
        "id": "recursion",
        "title": "10. Functions that call themselves",
        "intro": "A function may call itself. This is called recursion. It always needs a stopping "
                 "case, usually a `when` that reports an answer directly.",
        "example": "protocol factorial(n) {\n  when (n <= 1) { report 1; }\n  report n * factorial(n - 1);\n}\nbroadcast factorial(5);   // 120",
        "mission": "Write a function sumTo(n) that adds up every number from 1 to n, "
                   "using recursion. Broadcast sumTo(10) and then sumTo(100).",
        "starter": "protocol sumTo(n) {\n  // stopping case first, then call sumTo(n - 1)\n}\n",
        "solution": "protocol sumTo(n) {\n  when (n <= 1) { report n; }\n  report n + sumTo(n - 1);\n}\nbroadcast sumTo(10);\nbroadcast sumTo(100);\n",
        "expected": "55\n5050",
        "requires": ["protocol", "report", "sumTo", "when"],
        "hint": "When n <= 1, report n. Otherwise report n + sumTo(n - 1).",
    },
    {
        "id": "final",
        "title": "11. Final challenge: Ping Pong",
        "intro": "Time to put it all together: variables, loops, decisions and the remainder operator. "
                 "You can put a `when` inside an `otherwise { }` to check several things in order.",
        "example": 'when (x > 10) {\n  broadcast "big";\n} otherwise {\n  when (x > 5) { broadcast "medium"; } otherwise { broadcast "small"; }\n}',
        "mission": "Go through the numbers 1 to 15. For multiples of both 3 and 5 broadcast PingPong; "
                   "for other multiples of 3 broadcast Ping; for other multiples of 5 broadcast Pong; "
                   "otherwise broadcast the number.",
        "starter": "assign n = 1;\n",
        "solution": (
            "assign n = 1;\n"
            "repeat (n <= 15) {\n"
            "  when (n % 15 == 0) {\n"
            '    broadcast "PingPong";\n'
            "  } otherwise {\n"
            "    when (n % 3 == 0) {\n"
            '      broadcast "Ping";\n'
            "    } otherwise {\n"
            '      when (n % 5 == 0) { broadcast "Pong"; } otherwise { broadcast n; }\n'
            "    }\n"
            "  }\n"
            "  n = n + 1;\n"
            "}\n"
        ),
        "expected": "1\n2\nPing\n4\nPong\nPing\n7\n8\nPing\nPong\n11\nPing\n13\n14\nPingPong",
        "requires": ["repeat", "when", "otherwise", "%"],
        "hint": "Check the 'both' case first: a number is a multiple of 3 and 5 when n % 15 == 0.",
    },
]


_COMMENTS_AND_TEXT = re.compile(r'//[^\n]*|"(?:[^"\\\n]|\\.)*"?')


def code_without_text(code):
    """The code with comments and quoted text removed, so `requires` only sees real code."""
    return _COMMENTS_AND_TEXT.sub(" ", code)


def _uses(code, word):
    if re.fullmatch(r"\w+", word):
        return re.search(r"\b" + re.escape(word) + r"\b", code) is not None
    return word in code


def _clean_lines(text):
    lines = [line.rstrip() for line in text.replace("\r\n", "\n").split("\n")]
    while lines and not lines[-1]:
        lines.pop()
    return lines


def check_answer(lesson, code, output, exit_code):
    """Returns (passed, message) for a learner's program and what it printed."""
    if exit_code != 0:
        return False, "Your program has an error. Read the message in the Output panel, fix it, and try again."
    got, want = _clean_lines(output), _clean_lines(lesson["expected"])
    if got != want:
        for number, (g, w) in enumerate(zip(got, want), start=1):
            if g != w:
                return False, f"Output line {number} should be:\n    {w}\nbut your program printed:\n    {g}"
        if len(got) < len(want):
            return False, f"Almost! Your program printed {len(got)} line(s), but the mission needs {len(want)}. Next expected line:\n    {want[len(got)]}"
        return False, f"Your program printed {len(got) - len(want)} extra line(s) after the correct output:\n    {got[len(want)]}"
    plain = code_without_text(code)
    missing = [word for word in lesson["requires"] if not _uses(plain, word)]
    if missing:
        return False, "The output is right, but this mission asks you to use: " + ", ".join(missing)
    return True, "Correct!"
