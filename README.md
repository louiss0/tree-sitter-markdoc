# tree-sitter-markdoc

Tree-sitter grammar for Markdoc with multi-language bindings and editor
highlighting support.

## Features

- Frontmatter (YAML) blocks
- Markdoc tags and inline expressions
- Fenced code blocks with info strings and attributes
- Headings, lists, blockquotes, and thematic breaks
- Inline formatting (emphasis, strong, links, images, code)
- HTML blocks and comments

## Prerequisites

- Node.js (for the Tree-sitter CLI via npm) or Rust (for cargo installs)
- Tree-sitter CLI

```sh
npm install -g tree-sitter-cli
```

## Development

Generate the parser from `grammar.js`:

```sh
tree-sitter generate
```

Run the corpus tests in `test/corpus`:

```sh
tree-sitter test
```

Open the playground with the local parser:

```sh
tree-sitter playground
```

## Bindings

This project ships bindings for:

- C
- Go
- Node.js
- Python
- Rust
- Swift

See `bindings/` for language-specific packages.

## Queries

Syntax highlighting queries live in `queries/highlights.scm`.

## Notes

The `notes/` folder collects design references and development logs:

- `notes/markdoc.ebnf` is a hand-maintained grammar sketch used to reason
  about rules and intended structure. It is not generated and is not the
  source of truth. When updating the grammar, update the EBNF sketch first
  to validate the shape of the change, then apply the equivalent updates in
  `grammar.js`.
- `notes/asciidoc_grammar_reference.js` and
  `notes/asciidoc_scanner_reference.c` are reference materials used to compare
  grammar and scanner patterns.
- `notes/command-output.md` records diagnostic command output during parser
  verification.

## License

MIT
