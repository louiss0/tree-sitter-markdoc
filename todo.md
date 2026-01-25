# TODO

## Tests that are failing
- `tree-sitter test` currently reports failures in `04-markdoc-tags` (self-closing tags, block tags with/without content, nested tags, tags with attributes, embedded blocks, tagging around frontmatter, etc.) and `06-expressions-and-attributes` (operators in tag attributes) plus `07-tag-closing-and-whitespace` (alternative closing syntax, extra whitespace, blank lines). There are 16 failing cases in total; rerun the suite with `tree-sitter test` or `tree-sitter --file-name test/corpus/04-markdoc-tags.txt --debug` (and the other failing corpora) to inspect parse trees.

## What bugs are present
- The tag grammar still produces `paragraph` wrappers and `ERROR` nodes where `markdoc_tag` nodes should be, so block and inline tags with attributes/expressions are not parsed correctly.
- Tag attribute parsing rejects expressions (`{{ ... }}` by the old delimiter) and blank lines inside tags are treated as expression errors.
- Closing/tag whitespace rules and alternative closing syntax are not recognized, so tags produce ambiguous parses.
- Lists may need revisiting once the tags are stable; the current failing tests signal that tag delimiters are being confused with the preceding paragraphs and expressions.
- The spec still references the `{{ }}` expression delimiter; we need to switch all mentions (including tests) to `{% %}` and treat tags with that delimiter per the thematic-break/frontmatter heuristics described earlier.

## What to do next
1. Re-run `tree-sitter test` to confirm failures, then run `tree-sitter --file-name` with `--debug` on each failing corpus (especially `test/corpus/04-markdoc-tags.txt` and `test/corpus/07-tag-closing-and-whitespace.txt`) to inspect how parser states are handling tags and expressions.
2. Update the grammar/corpus so tags with attributes (including string and expression values) emit `markdoc_tag` nodes with the proper `tag_open`, nested content, and `tag_close`; address the `ERROR` dumps noted above and ensure nested tags, headings, code blocks, and blank lines stay inside the tag node.
3. Make the spec (`notes/markdoc.ebnf` and any documentation in `queries/README.md`) reflect `{% %}` expressions instead of `{{ }}` and explain how the thematic break vs. frontmatter heuristics apply to tags.
4. Once tags parse, revisit list grammars for nested/mixed items to make sure the new tag handling doesn't break them; fix any additional failing list tests (especially those covering interior tags or expressions inside lists) after the tag problems are resolved.
