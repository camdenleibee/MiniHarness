# ECE 309 - Project 2: The Conversation Loop

My implementation of `Conversation` and `SentinelScanner` for the miniharness
project. The harness, model clients, and `main.cpp` are the provided starter
code and are unchanged.

## What I wrote

- `include/core/message.h` - `Message` and the `Role` enum
- `include/core/conversation.h`, `src/conversation.cpp` - a growable array of
  `Message` with its own `new`/`delete` (no `std::vector`). It doubles its
  capacity when full and implements the Rule of Five.
- `include/core/sentinel_scanner.h`, `src/sentinel_scanner.cpp` - detects
  `<|end_conversation|>` in streamed chunks, holding back at most
  `sentinel.size() - 1` characters between calls
- `tests/p2/test_p2.cpp` - assert-based tests for `Conversation`,
  `SentinelScanner`, and the provided harness running on top of them
- `docs/design-log-p2.md` - growth factor proof, Rule of Five notes, and the
  bounded-buffer argument

## Build and run

```bash
cmake -S . -B build
cmake --build build
```

This builds:

- `./build/miniharness` - the interactive CLI
- `./build/test_p2` - the test suite

Run both from the project root. Some tests open `scripts/greeting.script` by
relative path.

```bash
./build/test_p2
./build/miniharness --script scripts/greeting.script --save transcript.txt
```

Ctrl-D on an empty line ends the conversation early, and the transcript is
still saved.