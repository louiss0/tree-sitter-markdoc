# TODO

## Tests that are failing
- `01-document-frontmatter` (frontmatter-only and frontmatter+body)
- `02-headings-paragraphs` (multi-line paragraphs and frontmatter heading case)
- `03-fenced-code` (frontmatter case)
- `04-markdoc-tags` (frontmatter case)
- `06-inline-formatting` (emphasis/strong and mixed inline cases)
- `08-emphasis-flanking-rules` (most emphasis edge cases)
- `08-paragraphs-and-lists` (basic paragraphs)
- `09-newlines-and-blank-lines` (multi-line paragraph continuation)

## What bugs are present
- Frontmatter is not parsed: `---` lines are currently lexed as `text`, so `frontmatter` never matches and related tests fail.
- Paragraphs no longer span single newlines after removing `_PARAGRAPH_CONTINUATION`, so multi-line paragraph expectations fail.
- Emphasis/strong are now regex tokens without flanking logic, so many inline formatting expectations are wrong.

## What to do next
- Fix frontmatter vs thematic break: ensure `_FRONTMATTER_DELIM` is emitted by the external scanner at column 0 and update grammar/tests so frontmatter is parsed before regular blocks.
- Redesign paragraph handling without external `_PARAGRAPH_CONTINUATION` (e.g., allow `_NEWLINE` joins in `paragraph` and `list_paragraph`, and update tests accordingly).
- Update inline emphasis/strong rules or tests to reflect the new non-scanner implementation; re-run `npx tree-sitter test -u` once behavior is settled.
