# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2025-01-30

### Added

- Initial release of tree-sitter-markdoc parser
- Support for YAML frontmatter blocks delimited by `---`
- Full support for ATX headings (levels 1-6)
- Fenced code block parsing with both backtick and tilde syntax
- Language identifiers in code block info strings
- Optional attributes in code block info strings (e.g., `{class="highlight"}`)
- Comprehensive syntax highlighting queries (`queries/highlights.scm`)
- Language injection queries for code blocks (`queries/injections.scm`)
- Test corpus with 43 test cases covering all implemented features
- Project documentation (README.md, queries/README.md)
- Git Flow workflow with conventional commits
- Basic paragraph parsing (single-line)

### Known Limitations

- Multi-line paragraphs not fully supported
- Markdoc tags (`{% tag %}`) not yet implemented
- Inline expressions (`{{ variable }}`) not yet implemented
- Lists not yet implemented
- Inline formatting (emphasis, strong, links, images) not yet implemented
- HTML support not yet implemented

### Test Coverage

- 25 out of 43 tests passing (~58%)
- 100% coverage for frontmatter, headings, and code blocks
- Partial coverage for paragraphs (~60%)

[0.1.0]: https://github.com/louiss0/tree-sitter-markdoc/releases/tag/v0.1.0
