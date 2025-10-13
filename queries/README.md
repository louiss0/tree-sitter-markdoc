# Tree-sitter Query Files

This directory contains query files used by Tree-sitter for syntax highlighting and language injection in Markdoc documents.

## Files

### highlights.scm

Defines syntax highlighting rules for Markdoc syntax elements:

- **Frontmatter**: YAML frontmatter blocks wrapped in `---`
- **Headings**: ATX-style headings (`#` through `######`) with markers and text
- **Code Blocks**: Fenced code blocks with backticks or tildes
  - Opening/closing fence markers
  - Language identifiers in info strings
  - Code content
  - Optional attributes (e.g., `{class="highlight"}`)
- **Markdoc Tags**: Block and self-closing tags (when fully implemented)
  - Tag delimiters (`{%`, `%}`, `/%}`)
  - Tag names
  - Attributes and their values
- **Inline Expressions**: Variable references wrapped in `{{` and `}}` (when fully implemented)
- **Text**: Regular paragraph text content

### injections.scm

Defines language injection rules for embedded content:

- **Code Fence Injection**: Automatically detects the language specified in fenced code blocks and enables language-specific syntax highlighting for that content.
  - For example, code blocks with ` ```javascript ` will have JavaScript syntax highlighting applied to the code content.

## Usage

These query files are automatically used by editors and tools that support Tree-sitter:

- **Neovim**: Place this grammar in `~/.local/share/nvim/site/pack/tree-sitter/start/`
- **Zed**: Configure in `~/.config/zed/settings.json` or as an extension
- **Helix**: Add to `~/.config/helix/runtime/queries/markdoc/`

## Supported Highlights

The following Tree-sitter capture names are used:

| Capture Name | Description | Applied To |
|--------------|-------------|------------|
| `@markup.raw.block` | Raw markup blocks | Frontmatter, code content |
| `@markup.heading.marker` | Heading level markers | `#`, `##`, etc. |
| `@markup.heading` | Heading text content | Text after heading marker |
| `@punctuation.bracket` | Bracket-like delimiters | ` ``` `, `{%`, `}}` |
| `@punctuation.delimiter` | Separator characters | `.`, `/` |
| `@property` | Property or attribute keys | Language identifiers, attribute names |
| `@tag` | Structural tags | Markdoc tag names |
| `@attribute` | Attributes and metadata | Attribute objects |
| `@operator` | Operators | `=` in attributes |
| `@variable` | Variable identifiers | Simple identifiers |
| `@variable.member` | Member access expressions | `obj.prop` |
| `@number` | Numeric literals | Integer and float values |
| `@string` | String literals | Quoted strings |
| `@text` | Plain text | Paragraph content |

## Examples

### Code Fence Injection

````markdoc
```javascript
const greeting = "Hello, World!";
console.log(greeting);
```
````

The `javascript` language identifier triggers JavaScript syntax highlighting for the code block content.

### Heading Highlighting

```markdoc
# Main Title
## Subsection
### Detail
```

Each heading marker and its text receive distinct highlighting based on heading level.

## Current Limitations

As of version 0.1.0, the following features are **not yet fully supported** in the grammar and queries:

- Inline expressions (`{{ variable }}`)
- Markdoc tags (block and self-closing)
- Complex attribute expressions
- Multi-line paragraphs

These features are planned for future releases and will require an external scanner for proper parsing.

## Contributing

When adding new grammar rules, ensure corresponding highlight queries are added to `highlights.scm` with appropriate capture names that follow Tree-sitter conventions.
