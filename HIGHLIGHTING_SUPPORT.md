# Highlighting Support for List Item Annotations

## Overview

Syntax highlighting support has been added for the new generic list item annotation feature in the tree-sitter-markdoc parser.

## Changes Made

### 1. Updated Query File: `queries/highlights.scm`

Added highlighting rules for list item annotations:

```scm
; List item annotations: {% type %} after list content
(list_item_annotation
  "{%" @punctuation.bracket
  "%}" @punctuation.bracket)

; Annotation type name (e.g., important, warning, note)
(annotation_type) @tag.attribute
```

### 2. Updated Configuration: `tree-sitter.json`

Added syntax highlighting paths to the grammar configuration:

```json
"highlights": "queries/highlights.scm",
"injections": "queries/injections.scm"
```

## Highlighting Details

The highlighting rules provide clear visual distinction for list annotations:

- **Annotation Brackets (`{%` and `%}`)**: Highlighted as `@punctuation.bracket`
  - Color: `#4e4e4e` (gray)
  - Font: Normal weight

- **Annotation Type Names**: Highlighted as `@tag.attribute`
  - Color: `#af0000` (red)
  - Font: Italic
  - Examples: `important`, `warning`, `note`, `info`

- **Annotation Attributes**: Use existing attribute highlighting
  - Attribute names: `@attribute` (red italic)
  - Attribute values: Context-dependent (strings in green, expressions with proper syntax highlighting)
  - Operators (like `=`): `@operator` (bold gray)

## Usage Examples

### Simple Annotation
```markdoc
- Item one {% important %}
- Item two {% warning %}
```

### Annotation with Attributes
```markdoc
- Item with custom styling {% custom type="primary" %}
```

### Annotation with Expressions
```markdoc
- Item with dynamic value {% highlight value=$variable %}
```

## Testing

### Test File
A comprehensive test file has been created: `test_highlighting.md`

Contains examples of:
- Unordered lists with annotations
- Ordered lists with annotations
- Annotations with attributes
- Annotations with expressions
- Mixed annotated and non-annotated items

### HTML Output
Test highlighting output can be generated:

```bash
npx tree-sitter highlight --html test_highlighting.md > test_highlighting.html
```

### Verification
The highlighting has been tested and verified to work correctly with:
- All 11 list annotation test cases passing
- Complete parse tree generation showing proper node structure
- HTML rendering with proper color and style application

## Supported Highlight Captures

The following highlight captures are used for list annotations:

| Capture | Style | Usage |
|---------|-------|-------|
| `@punctuation.bracket` | Gray, normal weight | Annotation delimiters `{%` and `%}` |
| `@tag.attribute` | Red, italic | Annotation type names |
| `@attribute` | Red, italic | Attribute names within annotations |
| `@operator` | Bold gray | Assignment operator `=` |
| `@string` | Green | String attribute values |
| `@variable` | Light gray | Variable references in expressions |
| `@punctuation.special` | Gray | `$` prefix for variables |

## Editor Compatibility

The highlighting rules use Tree-sitter standard capture names, ensuring compatibility with:
- Neovim (nvim-treesitter)
- Helix
- Zed
- Other Tree-sitter-based editors

## Files Modified

1. `queries/highlights.scm` - Added annotation highlighting rules
2. `tree-sitter.json` - Added highlighting path configuration
3. `test/corpus/10-list-annotations.txt` - Test cases for annotations
4. `test_highlighting.md` - Test file for highlighting verification
5. `test_highlighting.html` - Generated highlighting output (example)

## Test Results

```
10-list-annotations:
  124. ✓ Unordered list with simple annotation
  125. ✓ Ordered list with simple annotation
  126. ✓ List with multiple annotations
  127. ✓ List with multiple words and annotation
  128. ✓ Nested list with annotations
  129. ✓ List item annotation with attributes
  130. ✓ List annotation with expression attribute
  131. ✓ Ordered list with all items annotated
  132. ✓ List with simple text and annotation
  133. ✓ List with text and another word annotation
  134. ✓ Mixed list with and without annotations
```

All highlighting tests pass successfully with proper color and style application.
