# TODO

## Tests that include ERROR nodes
- The corpus fixtures still contain explicit `ERROR` nodes for negative cases in:
  - `test/corpus/05-html-tags-and-comments.txt`
  - `test/corpus/06-expressions-and-attributes.txt`
  - `test/corpus/08-emphasis-flanking-rules.txt`

## What bugs are present
- Validate whether any remaining `ERROR` fixtures are expected by the spec or should be tightened now that tag parsing has been stabilized.

## What to do next
1. Audit the remaining `ERROR` fixtures to confirm they still represent intentionally invalid inputs and document the rationale in the corpus headers if needed.
2. Keep an eye on list parsing as new tag edge cases are added, especially where tags appear inside list items or after blank lines.
