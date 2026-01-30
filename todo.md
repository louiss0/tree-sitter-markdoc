# TODO

## Tests that are failing
- Not run yet after the latest grammar/scanner changes; run `npx tree-sitter test` to confirm.

## What bugs are present
- HTML blocks are not recognized: `<div>...` parses as `ERROR` and paragraph text because `HTML_BLOCK` is never returned by the external scanner.
- Frontmatter takes precedence at file start; a file starting with `---` but without a closing delimiter now errors instead of producing a thematic break (decide if this is acceptable per spec).

## What to do next
- Debug `scan_html_block` using `tree-sitter parse --debug` (no custom logs) and fix the external scanner so `HTML_BLOCK` is emitted at column 0.
- Update corpus tests to match the corrected grammar (frontmatter/thematic break behavior, list markers including `-`, and HTML blocks) and re-run `npx tree-sitter test -u` as needed.
- Re-run `npx tree-sitter generate` after any grammar edits, then `npx tree-sitter test` to verify.
