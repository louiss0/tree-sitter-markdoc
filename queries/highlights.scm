; ============================================================================
; Tree-sitter Syntax Highlighting Queries for Markdoc
; ============================================================================
; This file defines syntax highlighting patterns for the Markdoc language,
; which extends Markdown with custom tags and expressions.
;
; Capture naming follows Tree-sitter conventions for cross-editor compatibility:
; - Neovim (nvim-treesitter)
; - Helix
; - Zed
; ============================================================================

; ============================================================================
; FRONTMATTER (YAML)
; ============================================================================

(frontmatter) @markup.raw.block
(yaml) @markup.raw.block

; ============================================================================
; HEADINGS
; ============================================================================

; Heading markers (#, ##, ###, etc.)
(heading_marker) @markup.heading.marker

; Heading text content
(heading_text) @markup.heading

; ============================================================================
; THEMATIC BREAKS (HORIZONTAL RULES)
; ============================================================================

(thematic_break) @punctuation.special

; ============================================================================
; BLOCKQUOTES
; ============================================================================

(blockquote) @markup.quote

; ============================================================================
; CODE BLOCKS (FENCED)
; ============================================================================

; Code fence delimiters (``` or ~~~)
(code_fence_open) @punctuation.bracket
(code_fence_close) @punctuation.bracket

; Language identifier (e.g., javascript, python, go)
(language) @label

; Attributes in info string (e.g., {1-5})
(info_string (attributes) @attribute)

; Code content
(code) @markup.raw.block

; ============================================================================
; LISTS
; ============================================================================

; List markers (-, *, +, 1., 2., etc.)
(unordered_list_marker) @markup.list
(ordered_list_marker) @markup.list

; ============================================================================
; INLINE FORMATTING
; ============================================================================

; Emphasis (italic) - *text* or _text_
(emphasis) @markup.italic

; Strong (bold) - **text** or __text__
(strong) @markup.bold

; Inline code - `code`
(inline_code) @markup.raw.inline

; ============================================================================
; LINKS AND IMAGES
; ============================================================================

; Link structure: [text](url)
(link
  "[" @punctuation.bracket
  "]" @punctuation.bracket
  "(" @punctuation.bracket
  ")" @punctuation.bracket)

(link_text) @markup.link.label
(link_destination) @markup.link.url

; Image structure: ![alt](url)
(image
  "![" @punctuation.bracket
  "]" @punctuation.bracket
  "(" @punctuation.bracket
  ")" @punctuation.bracket)

(image_alt) @markup.link.label
(image_destination) @markup.link.url

; ============================================================================
; HTML
; ============================================================================

; HTML blocks (block-level tags)
(html_block) @markup.raw.block

; HTML inline (inline tags)
(html_inline) @markup.raw.inline

; HTML comments <!-- comment -->
(html_comment) @comment

; ============================================================================
; MARKDOC TAGS
; ============================================================================

; Tag delimiters: {% and %} and /%}
(tag_open_delimiter) @punctuation.bracket
(inline_expression_close) @punctuation.bracket
(tag_block_close) @punctuation.bracket
(tag_self_close_delimiter) @punctuation.bracket

; Tag names (e.g., callout, table, partial)
(tag_name) @tag

; Closing tag slash
("/" @punctuation.delimiter)

; Comment blocks: {% comment %}...{% /comment %}
(comment_block) @comment

; ============================================================================
; TAG ATTRIBUTES
; ============================================================================

; Attribute name (e.g., type, id, class)
(attribute_name) @attribute

; Assignment operator
(attribute ("=" @operator))

; ============================================================================
; INLINE EXPRESSIONS {% ... %}
; ============================================================================

; Expression blocks
(inline_expression) @markup.raw.inline

; ============================================================================
; EXPRESSIONS AND OPERATORS
; ============================================================================

; Variables with $ prefix
(variable "$" @punctuation.special)
(variable (identifier) @variable)

; Variables with @ prefix
(special_variable "@" @punctuation.special)
(special_variable (identifier) @variable)

; Variable references: $var and $var.path
(variable_reference
  (variable (identifier) @variable)
  (identifier) @variable.member)

; Special variable references: @var and @var.path
(special_variable_reference
  (special_variable (identifier) @variable)
  (identifier) @variable.member)

; Subscript references: $items[0]
(subscript_reference
  (variable_reference (variable (identifier) @variable))
  (array_subscript (number) @number))

(subscript_reference
  (variable_reference (variable (identifier) @variable))
  (array_subscript (string) @string))

(subscript_reference
  (special_variable_reference (special_variable (identifier) @variable))
  (array_subscript (number) @number))

(subscript_reference
  (special_variable_reference (special_variable (identifier) @variable))
  (array_subscript (string) @string))

; Identifiers (function names, object keys, etc.)
(identifier) @variable

; Function calls: func(...)
(call_expression
  function: (identifier) @function)

; Subscript references
(array_subscript
  "[" @punctuation.bracket
  "]" @punctuation.bracket)

; ============================================================================
; LITERALS
; ============================================================================

; Strings: "string" or 'string'
(string) @string

; Numbers: 42, 3.14, -10
(number) @number

; Booleans: true, false
(boolean) @boolean

; Null
(null) @constant.builtin

; ============================================================================
; DATA STRUCTURES
; ============================================================================

; Array literals: [1, 2, 3]
(array_literal
  "[" @punctuation.bracket
  "]" @punctuation.bracket)

; Object literals: { key: value }
(object_literal
  "{" @punctuation.bracket
  "}" @punctuation.bracket)

; ============================================================================
; TEXT CONTENT
; ============================================================================

; Plain text in paragraphs and lists
(text) @none
(list_paragraph (text) @none)

; ============================================================================
; PARENTHESES AND BRACKETS (GENERIC)
; ============================================================================

; Function call parentheses
(call_expression
  "(" @punctuation.bracket
  ")" @punctuation.bracket)
