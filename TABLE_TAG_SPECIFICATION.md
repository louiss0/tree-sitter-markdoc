# Markdoc Table Tag Specification

## Overview

The table tag is a block-level Markdoc construct that allows rendering tabular data with structured rows and cells. Tables are delimited by `{% table %}...{% /table %}` and use a row-separator syntax to distinguish header rows from body rows.

## Syntax

### Basic Structure

```mdoc
{% table %}
* Header Cell 1
* Header Cell 2
---
* Body Cell 1
* Body Cell 2
---
* Body Cell 1
* Body Cell 2
{% /table %}
```

### Row Composition

Tables consist of:
1. **Header Row**: The first set of list items (`* ...`) followed by a `---` separator
2. **Body Rows**: Subsequent sets of list items, each separated by `---`
3. **Last Row**: The final set of items ends with the closing tag `{% /table %}` instead of `---`

## Cell Content

Each cell in a table row is marked with a list marker (`*`, `-`, or `+`) and can contain:

### Inline Content
- Plain text
- Emphasis (`*text*`, `_text_`)
- Strong (`**text**`, `__text__`)
- Inline code (`` `code` ``)
- Links (`[text](url)`)
- Images (`![alt](src)`)
- Inline expressions (`{{ expression }}`)
- Inline tags (`{% tag ... /%}`)

### Block Content
- Fenced code blocks
- Markdoc tags (nested block-level tags)
- Multi-line content (loose list style)

### Cell Annotations
Cell annotations are inline tag markers that modify cell rendering properties:

```mdoc
* Cell content {% colspan=2 %}
```

Supported annotations:
- `{% colspan=N %}` - Cell spans N columns
- Additional annotations can be added following the same pattern

## Examples

### Simple Table

```mdoc
{% table %}
* Name
* Age
---
* Alice
* 30
---
* Bob
* 25
{% /table %}
```

### Table with Code

```mdoc
{% table %}
* Language
* Example
---
*
  ```javascript
  console.log("Hello");
  ```
*
  Outputs "Hello" to console
{% /table %}
```

### Table with Nested Markdoc Tags

```mdoc
{% table %}
* Feature
* Description
---
*
  {% callout %}
  Important feature
  {% /callout %}
*
  {% list type="checkmark" %}
  * First benefit
  * Second benefit
  {% /list %}
{% /table %}
```

### Table with Cell Spanning

```mdoc
{% table %}
* Col 1
* Col 2
* Col 3
---
* Spans two columns
* {% colspan=2 %}
* Normal cell
---
* Another row
* More data
* Additional info
{% /table %}
```

### Table with Multi-line Content

```mdoc
{% table %}
* Header 1
* Header 2
---
*
  First paragraph in cell

  Second paragraph in cell
* Simple cell
{% /table %}
```

## Grammar Rules

The table tag grammar includes these core rules:

```javascript
table_tag:
  '{%' 'table' attributes '%}'
  [ header_row ('---' body_row)* ]
  '{%' '/' 'table' '%}'

table_header_row:
  table_row_items

table_body_row:
  table_row_items

table_row_items:
  table_cell (newline? table_cell)*

table_cell:
  list_marker table_cell_content

table_cell_content:
  table_cell_item+

table_cell_item:
  inline_expression
  | inline_tag
  | table_cell_annotation
  | fenced_code_block
  | markdoc_tag
  | text
  | html_inline
  | link
  | emphasis
  | strong
  | inline_code
  | image
  | newline
  | standalone_punct

table_cell_annotation:
  '{%' annotation_name '=' annotation_value '%}'
```

## Parsing Behavior

### Row Detection
- Rows are delimited by standalone `---` lines
- Each `---` acts as a row separator
- The first row (before the first `---`) is the header row
- All subsequent rows are body rows

### Cell Parsing
- Cells are identified by list markers (`*`, `-`, `+`)
- Cell content extends from the marker to the next marker or row separator
- Multiple lines of content within a cell are supported with proper indentation
- Inline and block elements can be mixed within cells

### Conflict Resolution
- Table tags are parsed with higher precedence than generic Markdoc tags to ensure `{% table %}` is recognized specifically
- The `table_cell_content` rule allows flexible content while avoiding parsing ambiguities
- Newlines within cell content are preserved as content items

## Design Considerations

1. **Header/Body Distinction**: The first `---` separator explicitly marks the transition from header to body, making table structure clear and unambiguous

2. **Flexibility**: Cells can contain complex nested structures (code blocks, other tags) without requiring special escaping

3. **Consistency**: Uses list markers (`*`) consistent with Markdoc's list syntax, making tables feel natural within the document structure

4. **Annotations**: Cell-level annotations follow Markdoc's inline tag syntax, maintaining visual and syntactic consistency

## Rendering Implications

When rendering tables, implementers should:

1. Identify the header row (first row before first `---`)
2. Render all subsequent rows as body rows
3. Process cell content according to Markdoc's inline and block rendering rules
4. Apply cell annotations (e.g., colspan) to affect table layout
5. Support nested structures within cells by recursively rendering contained elements

## Notes

- Tables currently do not support row-spanning (rowspan) annotations; only colspan is specified
- Empty cells are not explicitly handled; use whitespace or a placeholder if needed
- Table tag attributes are parsed but not specified here; implementation determines semantics
