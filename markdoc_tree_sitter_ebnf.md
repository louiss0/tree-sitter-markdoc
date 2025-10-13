# Markdoc — Tree-sitter EBNF

> A proposed Extended Backus–Naur Form (EBNF) grammar for Markdoc, designed to guide a Tree-sitter parser implementation. This file is organized into sections (features) so you can pick and implement them iteratively.

---

## Table of contents

1. Introduction & notes
2. Lexical tokens (regexes)
3. Document structure
4. Frontmatter
5. Headings
6. Paragraphs & inline content
7. Emphasis, strong, code, links, images
8. Lists (ordered / unordered)
9. Fenced code blocks
10. Block-level Markdoc tags (`{% ... %}`)
11. Inline expressions / variables (`{{ ... }}`)
12. Tag attributes & expressions
13. HTML-style tags and self-closing tags
14. Comments
15. Whitespace & indentation
16. Precedence & ambiguity notes
17. Example snippets
18. Implementation notes for Tree-sitter

---

## 1. Introduction & notes

This EBNF is intentionally pragmatic — it captures common Markdoc features (Markdown + Markdoc tags and expressions) to help build a Tree-sitter grammar. It is not a formal specification of every edge case in Markdoc; use it as a starting point and refine while writing tests.

Conventions used below:

- terminals are shown in `code` (regex style for token definitions)
- non-terminals are written as `\<name\>` (angle-bracket style)
- `*` means zero-or-more, `+` means one-or-more, `?` means optional
- alternatives use `|`

---

## 2. Lexical tokens (regexes)

Define lexical tokens first — Tree-sitter supports regex-based tokenizers.

```txt
TOK_NEWLINE       = /\r?\n/
TOK_WHITESPACE    = /[ \t]+/
TOK_HEADING_MARK  = /#{1,6}/           # at start of line
TOK_FENCE         = /(`{3,}|~{3,})/     # fenced code backticks or tildes
TOK_LIST_MARK     = /([*+-])|([0-9]+\.)/
TOK_BLOCKTAG_OPEN = /\{\%/            # {%
TOK_BLOCKTAG_CLOSE= /\%\}/            # %}
TOK_INLINE_OPEN   = /\{\{/            # {{
TOK_INLINE_CLOSE  = /\}\}/            # }}
TOK_ATTR_EQ       = /=/                 # =
TOK_IDENT         = /[A-Za-z_][A-Za-z0-9_\-]*/
TOK_STRING_SINGLE = /'([^'\\]|\\.)*'/
TOK_STRING_DOUBLE = /"([^"\\]|\\.)*"/
TOK_NUMBER        = /(?:0|[1-9][0-9]*)(?:\.[0-9]+)?/
TOK_BOOL          = /true|false/
TOK_LBRACE        = /\{/;
TOK_RBRACE        = /\}/;
TOK_LBRACK        = /\[/;
TOK_RBRACK        = /\]/;
TOK_COLON         = /:/;
TOK_COMMA         = /,/;
TOK_LT            = /</;
TOK_GT            = />/;
TOK_SLASH         = /\//;
TOK_TEXT          = /[^\n\{\<`]+/    # fallback text
```

Notes:

- Adjust `TOK_TEXT` to avoid swallowing special characters you want to handle elsewhere (e.g. `<`, `{`, backticks, etc.).
- For Tree-sitter you may prefer to create more fine-grained lexical tokens (e.g. `TEXT_WORD`, `PUNCT`) depending on parsing approach.

---

## 3. Document structure

```txt
<document> ::= <frontmatter>? <block>* EOF

<block> ::= <heading>
          | <fenced_code_block>
          | <list>
          | <blockquote>
          | <hr>
          | <markdoc_block_tag>   # {% tag %} ... {% /tag %}
          | <html_block>
          | <paragraph>
```

---

## 4. Frontmatter (YAML)

Frontmatter appears at the very beginning of the file and is delimited by `---` lines.

```txt
<frontmatter> ::= '---' TOK_NEWLINE <yaml_content> '---' TOK_NEWLINE
<yaml_content> ::= (TOK_TEXT | TOK_NEWLINE)*  # capture raw YAML; hand off to YAML parser
```

Implementation note: Treat frontmatter as a single node that stores raw text; don't try to parse YAML in Tree-sitter.

---

## 5. Headings

```txt
<heading> ::= TOK_HEADING_MARK TOK_WHITESPACE? <inline>* TOK_NEWLINE

# alternative setext-style headings (optional):
<setext_heading> ::= <paragraph> TOK_NEWLINE ('===' | '---') TOK_NEWLINE
```

---

## 6. Paragraphs & inline content

Paragraphs are groups of inline nodes separated by blank lines.

```txt
<paragraph> ::= <inline>+ (TOK_NEWLINE <inline>+)* TOK_NEWLINE

<inline> ::= <text>
           | <emphasis>
           | <strong>
           | <inline_code>
           | <link>
           | <image>
           | <inline_expression>   # {{ ... }}
           | <inline_markdoc_tag>  # {% ... %} used inline (if supported)
           | <html_inline>
```

`<text>` := sequence of TOK_TEXT and punctuation not matched by other inline tokens.

---

## 7. Emphasis, strong, code, links, images

```txt
<emphasis> ::= '*' <inline>+ '*' | '_' <inline>+ '_'
<strong>   ::= '**' <inline>+ '**' | '__' <inline>+ '__'

<inline_code> ::= '`' /[^`\n]+/ '`'

<link> ::= '[' <link_text> ']' '(' <link_destination> ( '"' <title> '"' )? ')'
<link_text> ::= <inline>*
<link_destination> ::= TOK_TEXT  # or more precise URL token

<image> ::= '![' <alt_text> ']' '(' <link_destination> ( '"' <title> '"' )? ')'
```

Note: Inline parsing needs careful tokenization to avoid greedy matches; Tree-sitter's external scanner or priority ordering helps.

---

## 8. Lists (ordered / unordered)

```txt
<list> ::= <list_item>+

<list_item> ::= TOK_LIST_MARK TOK_WHITESPACE <block>+  # allow nested blocks
```

Handle nested lists by tracking indentation or by using Tree-sitter node structure and explicit `INDENT`/`DEDENT` tokens if desired.

---

## 9. Fenced code blocks

```txt
<fenced_code_block> ::= TOK_FENCE <fence_info>? TOK_NEWLINE <code_raw> TOK_FENCE TOK_NEWLINE
<fence_info> ::= TOK_IDENT (TOK_WHITESPACE TOK_TEXT)?   # language and optional attributes
<code_raw> ::= ( /[^`\n].*/ TOK_NEWLINE )*    # raw code lines, stop when closing fence matches
```

---

## 10. Block-level Markdoc tags (`{% ... %}`)

Markdoc block tags resemble Liquid-style tags. Common forms:

- Opening tag: `{% tagName attr1="value" attr2=expr %}`
- Closing tag: `{% /tagName %}`
- Self-closing: `{% tagName ... /%}` (if supported)

```txt
<markdoc_block_tag> ::= <markdoc_block_tag_open> <block>* <markdoc_block_tag_close>

<markdoc_block_tag_open> ::= TOK_BLOCKTAG_OPEN TOK_WHITESPACE? TOK_IDENT <attr_list>? TOK_WHITESPACE? TOK_BLOCKTAG_CLOSE

<markdoc_block_tag_close> ::= TOK_BLOCKTAG_OPEN TOK_WHITESPACE? '/'? TOK_IDENT? TOK_WHITESPACE? TOK_BLOCKTAG_CLOSE

<attr_list> ::= (<attr> (TOK_WHITESPACE <attr> )* )?
<attr> ::= TOK_IDENT TOK_ATTR_EQ <attr_value>
<attr_value> ::= TOK_STRING_SINGLE | TOK_STRING_DOUBLE | <expression>
```

Notes:

- For tags that can be closed with `/{%` (self-closing), allow `/'` before the closing `%}` or support a boolean attribute `self_closing=true`.
- Decide whether to allow nested `markdoc_block_tag` nodes; many tags will contain blocks.

---

## 11. Inline expressions / variables (`{{ ... }}`)

Inline expressions allow data interpolation.

```txt

<inline_expression> ::= TOK_INLINE_OPEN <expression> TOK_INLINE_CLOSE

<expression> ::= <literal>
               | <identifier>
               | <member_access>
               | <call_expression>
               | <object>
               | <array>
               | <binary_expression>
               | <unary_expression>

<literal> ::= TOK_STRING_SINGLE | TOK_STRING_DOUBLE | TOK_NUMBER | TOK_BOOL | 'null'
<identifier> ::= TOK_IDENT ('.' TOK_IDENT)*
<member_access> ::= <identifier> ('.' <identifier>)+
<call_expression> ::= <identifier> '(' (<expression> (',' <expression>)*)? ')'
<object> ::= '{' ( <pair> (',' <pair>)* )? '}'
<pair> ::= (TOK_STRING_SINGLE | TOK_STRING_DOUBLE | TOK_IDENT) ':' <expression>
<array> ::= '[' (<expression> (',' <expression>)*)? ']'

# Basic binary operators (precedence will be described later)
<binary_expression> ::= <expression> OPERATOR <expression>
<unary_expression> ::= ('-' | '!') <expression>
```

Implementation note: This expression grammar is intentionally broad and can be simplified if Markdoc's expression subset is smaller. For Tree-sitter, you may prefer to hand off complex expression parsing to a separate `expression` subgrammar or external scanner.

---

## 12. Tag attributes & expressions

Attributes in Markdoc may accept expressions (objects/arrays) in addition to strings. Example:

```txt
{% gallery images=[{src: "a.jpg", alt: "A"}, {src: "b.jpg"}] %}
```

EBNF for attribute values was shown earlier in `<attr_value>` and `<expression>`.

---

## 13. HTML-style tags and self-closing tags

Markdoc allows raw HTML blocks and inline HTML.

```txt
<html_block> ::= '<' TOK_IDENT ( <html_attr>* ) '>' <html_content>* '</' TOK_IDENT '>'
<html_inline> ::= '<' TOK_IDENT ( <html_attr>* ) '/>'
<html_attr> ::= TOK_IDENT '=' (TOK_STRING_SINGLE | TOK_STRING_DOUBLE | TOK_IDENT)
<html_content> ::= (TOK_TEXT | <inline>)+
```

Treat HTML nodes as raw nodes if you prefer not to attempt full HTML parsing.

---

## 14. Comments

Markdoc comments often use Liquid-style comments `{% comment %} ... {% /comment %}` or HTML comments `<!-- ... -->`.

```txt
<comment_block> ::= TOK_BLOCKTAG_OPEN 'comment' TOK_BLOCKTAG_CLOSE .*? TOK_BLOCKTAG_OPEN '/comment' TOK_BLOCKTAG_CLOSE
<html_comment> ::= '<!--' /(.|\n)*? '-->'
```

---

## 15. Whitespace & indentation

Markdown list nesting and blockquote nesting commonly rely on indentation. Two approaches:

1. **Indent tokens (explicit)** — produce `INDENT` / `DEDENT` tokens with an external scanner (more precise for nested list parsing).
2. **Line prefix parsing** — in grammar, capture leading whitespace and list markers; treat nested list items by examining the number of spaces.

Example (simple):

```txt
<line_prefix> ::= TOK_WHITESPACE?
<blockquote> ::= '>' TOK_WHITESPACE? <block>*
```

---

## 16. Precedence & ambiguity notes

- Emphasis vs strong: prefer longest-match first (i.e. `**` before `*`).
- Inline code has higher precedence than emphasis/strong.
- Block-level tags (`{% ... %}`) should be matched before parsing paragraphs to avoid consuming their content as plain text.
- For ambiguous constructs (e.g. HTML vs label text), prefer to implement tests and tune token precedence in Tree-sitter.

---

## 17. Example snippets (EBNF usage)

**Block tag with nested blocks:**

```txt
{% callout title="Note" %}
This is a paragraph inside a callout.

- list item
{% /callout %}
```

**Inline expression:**

```txt
Hello {{ user.name }}!
```

**Gallery tag with array attribute:**

```txt
{% gallery images=[{src: "a.jpg", alt: "A"}, {src: "b.jpg", alt: "B"}] %}
{% /gallery %}
```

---

## 18. Implementation notes for Tree-sitter

- Start by implementing tokens (lexical rules) and a small set of block rules: frontmatter, headings, paragraphs, fenced code, and block tags.
- Use an external scanner if you need `INDENT`/`DEDENT` tokens for nested lists and blockquote levels.
- Prefer to treat complex inline expressions (JS-like) as a subtree with its own grammar rules to isolate complexity.
- Build a robust test-suite of representative Markdoc documents (frontmatter, nested tags, inline expressions, mixed Markdown+tags) and iterate.
- When conflicts arise (e.g. emphasis vs link text), change token priorities and/or transform ambiguous terminals into contextualized tokens (e.g. `STAR_OPEN`, `STAR_CLOSE`).

---

### Quick checklist for parser feature rollout

1. Document, frontmatter
2. Headings, paragraphs
3. Fenced code blocks
4. Block tags (`{% ... %}`) and inline expressions (`{{ ... }}`)
5. Lists and blockquote nesting (external scanner if needed)
6. Inline formatting (emphasis, strong, links, images)
7. HTML block/inline handling
8. Attribute expression parsing and tests

---
