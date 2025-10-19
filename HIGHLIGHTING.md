# Syntax Highlighting for Tree-sitter Markdoc

This document describes the syntax highlighting capabilities of the Tree-sitter Markdoc parser.

## Overview

The `queries/highlights.scm` file defines Tree-sitter query patterns that map parsed syntax nodes to semantic highlight groups. These highlight groups follow Tree-sitter conventions and work across multiple editors including:

- **Neovim** (with nvim-treesitter)
- **Helix**
- **Zed**
- Any editor with Tree-sitter highlighting support

## Supported Features

### Markdown Elements

#### Document Structure
- **Frontmatter (YAML)**: Highlighted as `@markup.raw.block`
- **Headings**: Markers highlighted as `@markup.heading.marker`, text as `@markup.heading`
- **Thematic Breaks** (---): Highlighted as `@punctuation.special`
- **Blockquotes** (>): Highlighted as `@markup.quote`

#### Inline Formatting
- **Bold/Strong** (`**text**` or `__text__`): Highlighted as `@markup.bold`
- **Italic/Emphasis** (`*text*` or `_text_`): Highlighted as `@markup.italic`
- **Inline Code** (`` `code` ``): Highlighted as `@markup.raw.inline`

#### Links and Images
- **Links**: `[text](url)`
  - Link text: `@markup.link.label`
  - Link URL: `@markup.link.url`
  - Brackets/parens: `@punctuation.bracket`

- **Images**: `![alt](src)`
  - Alt text: `@markup.link.label`
  - Image URL: `@markup.link.url`
  - Brackets/parens: `@punctuation.bracket`

#### Code Blocks
- **Fence Delimiters** (``` or ~~~): `@punctuation.bracket`
- **Language Identifier** (e.g., `javascript`): `@label`
- **Attributes** (e.g., `{1-5}`): `@attribute`
- **Code Content**: `@markup.raw.block`

#### Lists
- **List Markers** (-, *, +, 1., 2., etc.): `@markup.list`
- List content inherits inline formatting

#### HTML
- **HTML Blocks**: `@markup.raw.block`
- **HTML Inline**: `@markup.raw.inline`
- **HTML Comments** (`<!-- -->`): `@comment`

### Markdoc Extensions

#### Tags
- **Tag Delimiters** (`{%`, `%}`, `/%}`): `@punctuation.bracket`
- **Tag Names** (e.g., `callout`, `table`): `@tag`
- **Closing Slash**: `@punctuation.delimiter`
- **Comment Blocks** (`{% comment %}`): `@comment`

#### Tag Attributes
- **Attribute Names** (e.g., `type`, `id`): `@attribute`
- **Assignment Operator** (`=`): `@operator`
- **Attribute Values**:
  - Strings: `@string`
  - Numbers: `@number`
  - Booleans: `@boolean`
  - Null: `@constant.builtin`
  - Expressions: See "Expressions" below

#### Inline Expressions (`{{ ... }}`)
- **Expression Delimiters** (`{{`, `}}`): `@punctuation.bracket`
- **Variables** (`$variable`):
  - Dollar sign: `@punctuation.special`
  - Identifier: `@variable`

#### Expressions
- **Identifiers**: `@variable`
- **Member Access**:
  - Dot: `@punctuation.delimiter`
  - Property: `@property`
- **Array Access**:
  - Brackets: `@punctuation.bracket`
- **Function Calls**:
  - Function name: `@function`
  - Parentheses: `@punctuation.bracket`
  - Commas: `@punctuation.delimiter`
- **Arrow Functions**:
  - Arrow (`=>`): `@keyword.operator`
  - Parameters: `@variable.parameter`
  - Parentheses: `@punctuation.bracket`

#### Data Structures
- **Arrays** (`[...]`):
  - Brackets: `@punctuation.bracket`
  - Commas: `@punctuation.delimiter`
- **Objects** (`{...}`):
  - Braces: `@punctuation.bracket`
  - Keys: `@property`
  - Colons: `@punctuation.delimiter`
  - Commas: `@punctuation.delimiter`

#### Literals
- **Strings** (`"string"` or `'string'`): `@string`
- **Numbers** (42, 3.14, -10): `@number`
- **Booleans** (true, false): `@boolean`
- **Null**: `@constant.builtin`

## Known Limitations

### Operators Cannot Be Highlighted

Binary operators (e.g., `==`, `!=`, `+`, `-`, `*`, `/`, `&&`, `||`) and unary operators (e.g., `!`, `-`, `+`) are defined as **anonymous tokens** in the grammar using `token(prec(...))`. Tree-sitter query language can only match **named nodes**, not anonymous tokens.

To enable operator highlighting, the grammar would need to be modified to make operators named nodes, which would require:
1. Changing operator definitions in `grammar.js`
2. Regenerating the parser
3. Potentially adjusting precedence rules

### Text Content

Plain text nodes (`(text)`) are highlighted with `@none`, making them inherit the default text color rather than applying special formatting. This is intentional to avoid over-highlighting.

## Editor Setup

### Neovim (nvim-treesitter)

1. Install the parser:
```lua
require('nvim-treesitter.configs').setup {
  ensure_installed = { 'markdoc' },
  -- Or use 'all'
}
```

2. The parser will automatically use `queries/highlights.scm` for syntax highlighting.

3. (Optional) Add filetype detection to `~/.config/nvim/ftdetect/markdoc.vim`:
```vim
au BufRead,BufNewFile *.mdoc setfiletype markdoc
```

### Helix

1. Add to `~/.config/helix/languages.toml`:
```toml
[[language]]
name = "markdoc"
scope = "text.markdoc"
injection-regex = "markdoc"
file-types = ["mdoc", "markdoc"]
roots = []
comment-token = "<!--"

[[grammar]]
name = "markdoc"
source = { git = "https://github.com/yourusername/tree-sitter-markdoc", rev = "main" }
```

2. Run `hx --grammar fetch` and `hx --grammar build`

### Zed

Follow Zed's Tree-sitter language integration documentation to add the parser.

## Testing Highlighting

To test the highlighting queries:

```bash
# Run parser tests (includes query validation)
npx tree-sitter test

# Parse a file to see the syntax tree
npx tree-sitter parse path/to/file.md

# Test files for verification
npx tree-sitter parse test_highlight_simple.md
```

## Contributing

If you find highlighting issues or want to add support for additional syntax elements:

1. Identify the node type in the parse tree using `npx tree-sitter parse`
2. Add appropriate query patterns to `queries/highlights.scm`
3. Test with `npx tree-sitter test`
4. Submit a pull request

## References

- [Tree-sitter Syntax Highlighting](https://tree-sitter.github.io/tree-sitter/syntax-highlighting)
- [Tree-sitter Query Syntax](https://tree-sitter.github.io/tree-sitter/using-parsers#pattern-matching-with-queries)
- [Standard Capture Names](https://github.com/nvim-treesitter/nvim-treesitter/blob/master/CONTRIBUTING.md#parser-configurations)
