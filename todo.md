# Tests that are failing
- `tree-sitter test --debug --include "Nested fences with longer outer fence"`
- `tree-sitter test --debug --include "Tag with code block inside"`
- `tree-sitter test` (fails due to the two cases above)

# What bugs are present
- Nested fenced code blocks do not terminate the outer fence; CODE_CONTENT consumes the outer close line.
- Code fence inside a Markdoc tag requires an extra `_NEWLINE` before the tag close, causing the "Tag with code block inside" test to fail.

# What to do next
- Debug `src/scanner.c` CODE_CONTENT close detection: verify `s->fence_length` and line-start detection, and ensure the outer close line is detected before consuming it.
- Re-run `tree-sitter test --debug --include "Nested fences with longer outer fence"` after scanner changes.
- Fix the tag + fenced code block case, then re-run `tree-sitter test --debug --include "Tag with code block inside"` and `tree-sitter test`.
