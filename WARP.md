# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Core Development Commands

- Generate the parser from grammar.js
  - npx tree-sitter generate
  - Generated C sources are written into src/.

- Run all tests
  - npx tree-sitter test
  - Optional: add -u to update expected outputs after intentional grammar changes.

- Run the playground / web UI
  - One-shot (builds as needed): npx tree-sitter web-ui
  - If you prefer to build WASM explicitly first:
    - npx tree-sitter build-wasm
    - npx tree-sitter web-ui
  - Open the URL printed by the CLI to interactively inspect parse trees and queries.

- Parse specific files
  - npx tree-sitter parse path/to/file.md
  - Useful flags:
    - -q (quiet), -t (timings), --stat (statistics)

- Test syntax highlighting
  - Build the native parser (for CLI-based highlighting):
    - npx tree-sitter build
  - Highlight a file using local queries:
    - npx tree-sitter highlight --language build/*/tree-sitter-markdoc.* --queries queries path/to/file.md
    - The queries directory should contain highlights.scm and injections.scm.

- Build for WASM
  - npx tree-sitter build-wasm
  - Produces a WebAssembly parser suitable for browser-based tooling and the web UI.

## Architecture Overview

- Tree-sitter grammar for Markdoc (a Markdown extension with tags and expressions).
- grammar.js defines the parsing rules.
- Generated parser code (C) is emitted into src/.
- Test corpus resides in test/corpus/ with 8 files covering different features.
- Query files (highlights.scm, injections.scm) provide syntax highlighting and language injection rules.
- The generated parser supports multiple language bindings (C, Go, Node, Python, Rust, Swift).

## Grammar Architecture

- Core block types:
  - Frontmatter (YAML)
  - Headings (ATX style)
  - Fenced code blocks
  - Markdoc tags
  - Lists
  - HTML
  - Paragraphs
- Inline features:
  - Expressions: {{ ... }}
  - Formatting: emphasis, strong, code, links, images
- Expression system supports:
  - Identifiers, member access, function calls, array access
  - Literals: objects, arrays, strings, numbers
- Precedence and conflict resolution:
  - Attribute parsing vs. embedded expressions are disambiguated via precedence rules to ensure tags with attributes and inline expressions parse consistently.

## Test Structure

- Tests are organized by feature in test/corpus/.
- Current test pass rate: 55/80 (68.75%).
- Known limitations:
  - Multi-line paragraphs
  - Complex nested lists

## Git Workflow

- Uses Git Flow with master and develop branches.
- Follows the Conventional Commits specification for commit messages.
- Currently on the develop branch for active work.
