# Tests that are failing
- Nested fenced code block corpus tests in `test/corpus/03-fenced-code.txt` (run with `tree-sitter test --include "Nested fences"`).

# What bugs are present
- `src/scanner.c` emits `_NESTED_CODE_BLOCK` for any line that starts with >= 4 fence chars and does not scan to a matching close, which breaks outer fence handling and highlighting.
- Nested fences are still being consumed as `_CODE_CONTENT`, so outer `_CODE_FENCE_CLOSE` can be missed.

# What to do next
- Rework `scan_nested_code_block` in `src/scanner.c` to span from an inner fence open to a matching close (length >= 4 and equal), and ensure it only triggers inside outer fences without consuming outer closes.
- Re-run `tree-sitter generate` and update `src/parser.c`, `src/grammar.json`, `src/node-types.json` if needed.
- Run `tree-sitter test --debug --include "Nested fences"` to validate fixes, then `tree-sitter test`.
