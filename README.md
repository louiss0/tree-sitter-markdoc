# tree-sitter-markdoc

A [Tree-sitter](https://tree-sitter.github.io/tree-sitter/) grammar for [Markdoc](https://markdoc.dev/), designed to provide syntax highlighting and structural parsing for Markdoc documents in editors and tools that support Tree-sitter.

## Features

### Currently Supported (v0.1.0)

- ✅ **YAML Frontmatter**: Parses frontmatter blocks delimited by `---`
- ✅ **ATX Headings**: Supports heading levels 1-6 (`#` through `######`)
- ✅ **Paragraphs**: Basic single-line paragraph parsing
- ✅ **Fenced Code Blocks**: Both backtick (` ``` `) and tilde (`~~~`) fences
  - Language identifiers in info strings
  - Optional attributes (e.g., `{class="highlight"}`)
- ✅ **Syntax Highlighting**: Comprehensive highlight queries for all supported features
- ✅ **Language Injection**: Automatic syntax highlighting for code blocks based on language identifier

### Planned Features

The following features are planned for future releases:

- 🔄 **Markdoc Tags**: Block tags and self-closing tags (`{% tag %}`, `{% /tag %}`)
- 🔄 **Inline Expressions**: Variable interpolation with `{{ variable }}`
- 🔄 **Lists**: Unordered and ordered lists with nesting
- 🔄 **Inline Formatting**: Emphasis, strong, inline code, links, images
- 🔄 **HTML Support**: Inline HTML and HTML blocks
- 🔄 **Complex Expressions**: Object literals, arrays, member access, function calls
- 🔄 **Multi-line Paragraphs**: Proper handling of paragraph continuation

## Installation

### Prerequisites

- Node.js (v14 or higher)
- Git

### Building from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/tree-sitter-markdoc.git
cd tree-sitter-markdoc

# Install dependencies
npm install

# Generate the parser
npx tree-sitter generate

# Run tests
npx tree-sitter test
```

## Usage

### With Neovim

1. Install the grammar:

```bash
mkdir -p ~/.local/share/nvim/site/pack/tree-sitter/start
cd ~/.local/share/nvim/site/pack/tree-sitter/start
git clone https://github.com/yourusername/tree-sitter-markdoc.git
```

2. Configure Neovim to use the grammar for `.mdoc` or `.md` files.

### With Zed Editor

Add to your Zed `settings.json`:

```json
{
  "languages": {
    "Markdoc": {
      "file_types": ["mdoc", "markdoc"],
      "grammar": "markdoc"
    }
  }
}
```

### With Helix

Copy query files to your Helix runtime:

```bash
mkdir -p ~/.config/helix/runtime/queries/markdoc
cp queries/*.scm ~/.config/helix/runtime/queries/markdoc/
```

## Syntax Highlighting

This grammar provides comprehensive syntax highlighting through Tree-sitter query files:

- **Frontmatter**: YAML blocks are highlighted as raw markup
- **Headings**: Distinct highlighting for heading markers and text
- **Code Blocks**: 
  - Fence delimiters highlighted as punctuation
  - Language identifiers highlighted as properties
  - Code content receives language-specific highlighting via injection
- **Attributes**: Key-value attributes in code block info strings

See [`queries/README.md`](queries/README.md) for detailed documentation on highlighting queries.

## Grammar Coverage

| Feature | Support | Test Coverage |
|---------|---------|---------------|
| YAML Frontmatter | ✅ Full | 100% |
| ATX Headings | ✅ Full | 100% |
| Fenced Code Blocks | ✅ Full | ~92% |
| Paragraphs | ⚠️ Partial | ~60% |
| Markdoc Tags | ❌ Not Yet | 0% |
| Inline Expressions | ❌ Not Yet | 0% |
| Lists | ❌ Not Yet | 0% |
| Inline Formatting | ❌ Not Yet | 0% |

**Overall Test Pass Rate**: 25/43 tests passing (~58%)

## Development

### Project Structure

```
tree-sitter-markdoc/
├── grammar.js           # Grammar definition
├── package.json         # Project metadata and dependencies
├── tree-sitter.json     # Tree-sitter configuration
├── queries/
│   ├── highlights.scm   # Syntax highlighting queries
│   ├── injections.scm   # Language injection queries
│   └── README.md        # Query documentation
├── test/
│   └── corpus/          # Test files
│       ├── 01-document-frontmatter.txt
│       ├── 02-headings-paragraphs.txt
│       ├── 03-fenced-code.txt
│       └── 04-markdoc-tags.txt
└── src/                 # Generated parser (C code)
```

### Running Tests

```bash
# Run all tests
npx tree-sitter test

# Parse a specific file
npx tree-sitter parse path/to/file.mdoc

# Test syntax highlighting
npx tree-sitter highlight path/to/file.mdoc
```

### Git Workflow

This project uses [Git Flow](https://nvie.com/posts/a-successful-git-branching-model/) with conventional commits:

- **Main branch**: Production-ready releases only
- **Develop branch**: Integration branch for features
- **Feature branches**: New features and enhancements
- **Release branches**: Release preparation
- **Hotfix branches**: Critical production fixes

All commits follow the [Conventional Commits](https://www.conventionalcommits.org/) specification.

## Contributing

Contributions are welcome! Please follow these guidelines:

1. Fork the repository
2. Create a feature branch (`git flow feature start feature-name`)
3. Write tests for your changes
4. Implement your feature
5. Ensure all tests pass
6. Follow conventional commit format
7. Submit a pull request

## Roadmap

### Version 0.1.0 (Current)
- ✅ Basic Markdown features (headings, paragraphs, code blocks)
- ✅ YAML frontmatter
- ✅ Syntax highlighting queries
- ✅ Language injection for code blocks

### Version 0.2.0 (Next)
- 🔄 Markdoc tags (block and self-closing)
- 🔄 Inline expressions with variable interpolation
- 🔄 External scanner for context-sensitive parsing

### Version 0.3.0 (Future)
- 🔄 Lists (unordered and ordered)
- 🔄 Inline formatting (emphasis, strong, code, links, images)
- 🔄 HTML support

### Version 1.0.0 (Stable)
- 🔄 Complete Markdoc syntax support
- 🔄 100% test coverage
- 🔄 Production-ready parser

## Known Limitations

- **Multi-line Paragraphs**: Currently, paragraphs spanning multiple lines may not parse correctly due to Tree-sitter's context-free nature. An external scanner will be needed for proper handling.
- **Markdoc Tags**: Tag parsing is not yet implemented and will require an external scanner to handle delimiter conflicts.
- **Inline Expressions**: Not yet supported; requires external scanner.

## License

MIT License - see [LICENSE](LICENSE) for details.

## Author

Shelton Louis <louisshelton0@gmail.com>

## Acknowledgments

- [Tree-sitter](https://tree-sitter.github.io/) for the parsing framework
- [Markdoc](https://markdoc.dev/) for the content authoring system
- The Tree-sitter community for excellent documentation and examples

## Resources

- [Markdoc Documentation](https://markdoc.dev/docs)
- [Tree-sitter Documentation](https://tree-sitter.github.io/tree-sitter/)
- [Tree-sitter Grammar Development Guide](https://tree-sitter.github.io/tree-sitter/creating-parsers)
