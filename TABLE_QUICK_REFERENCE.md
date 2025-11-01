# Markdoc Table Tag - Quick Reference

## Basic Syntax

```mdoc
{% table %}
* Header 1
* Header 2
---
* Cell 1
* Cell 2
{% /table %}
```

## Structure

| Element | Purpose | Syntax |
|---------|---------|--------|
| Table Opening | Marks start of table | `{% table %}` |
| Header Row | First row of table | Items followed by `---` |
| Body Rows | Data rows | Items separated by `---` |
| Row Separator | Divides rows | `---` on its own line |
| Table Closing | Marks end of table | `{% /table %}` |
| Cell | Individual data cell | `* content` |

## Cell Content Examples

### Text Only
```mdoc
* Plain text here
```

### With Formatting
```mdoc
* **Bold text** and *italic*
* `inline code` in cell
```

### With Links and Images
```mdoc
* [Link text](https://example.com)
* ![Alt text](image.png)
```

### Multi-line Content
```mdoc
*
  First paragraph
  
  Second paragraph
```

### With Code Blocks
```mdoc
*
  ```javascript
  const x = 42;
  console.log(x);
  ```
```

### With Nested Tags
```mdoc
*
  {% callout %}
  Important message
  {% /callout %}
```

### With Lists
```mdoc
*
  {% list type="checkmark" %}
  * Item 1
  * Item 2
  {% /list %}
```

## Cell Annotations

### Colspan (Span Columns)
```mdoc
* Spans two columns {% colspan=2 %}
```

Annotation Syntax: `{% attribute=value %}`

## Complete Examples

### Contact Information
```mdoc
{% table %}
* Name
* Email
---
* Alice Johnson
* alice@example.com
---
* Bob Smith
* bob@example.com
{% /table %}
```

### Features Matrix
```mdoc
{% table %}
* Feature
* Free
* Pro
* Enterprise
---
* Basic Support
* ✓
* ✓
* ✓
---
* Priority Support
*
* ✓
* ✓
---
* Dedicated Manager
*
*
* ✓
{% /table %}
```

### Documentation Table
```mdoc
{% table %}
* Method
* Description
* Return Type
---
*
  ```typescript
  getValue()
  ```
*
  Gets the current value
  from the state
*
  `any`
---
*
  ```typescript
  setValue(val)
  ```
*
  Updates the state with
  a new value
*
  `void`
{% /table %}
```

## Tips & Tricks

1. **Empty cells**: Use just the marker `*` to leave a cell empty
2. **Multi-line**: Indent content after the marker for multiple lines
3. **Consistent columns**: Ensure each row has the same number of cells
4. **Nested tags**: Any Markdoc tag can be used within cells
5. **Expressions**: Use `{{ expression }}` for dynamic content

## Common Patterns

### Aligned Spanning
```mdoc
{% table %}
* A
* B
* C
---
* Span 2 {% colspan=2 %}
* D
---
* E
* Span all {% colspan=3 %}
{% /table %}
```

### Mixed Content Types
```mdoc
{% table %}
* Code
* Output
---
*
  ```
  function() { }
  ```
*
  ```
  Returns undefined
  ```
---
*
  `const x = 5;`
*
  `x` equals 5
{% /table %}
```

### Nested Information
```mdoc
{% table %}
* Topic
* Details
---
*
  Setup Guide
*
  {% callout type="info" %}
  Follow these steps carefully
  {% /callout %}
---
*
  Best Practices
*
  - Use semantic HTML
  - Follow WCAG guidelines
  - Test accessibility
{% /table %}
```

## Limitations

- Rowspan not supported (colspan only)
- All rows should have same column count
- Complex nesting may impact rendering performance
- Table styling done via Markdoc tag attributes

## See Also

- [TABLE_TAG_SPECIFICATION.md](./TABLE_TAG_SPECIFICATION.md) - Full specification
- [test/table_examples.md](./test/table_examples.md) - More examples
- [TABLE_IMPLEMENTATION_SUMMARY.md](./TABLE_IMPLEMENTATION_SUMMARY.md) - Technical details
