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
; FRONTMATTER
; ============================================================================

(frontmatter) @markup.raw.block

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

(blockquote ">" @markup.quote)

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
(list_marker) @markup.list

; List item annotations: {% type %} after list content
(list_item_annotation
  "{%" @punctuation.bracket
  "%}" @punctuation.bracket)

; Annotation type name (e.g., important, warning, note)
(annotation_type) @tag.attribute

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
("{%" @punctuation.bracket)
("%}" @punctuation.bracket)
("/%}" @punctuation.bracket)

; Tag names (e.g., callout, table, partial)
(tag_name) @tag

; Closing tag slash
("/" @punctuation.delimiter)

; Comment blocks: {% comment %}...{% /comment %}
(comment_block) @comment

; ============================================================================
; CONDITIONAL TAGS (if/else)
; ============================================================================

; If tag opening: {% if condition %}
(if_tag_open
  "{%" @keyword
  "if" @keyword)

; If tag closing: {% /if %}
(if_tag_close
  "{%" @keyword
  "/" @keyword
  "if" @keyword)

; Else tag: {% else %} or {% else condition /%}
(else_tag
  "{%" @keyword
  "else" @keyword
  "/%}" @keyword)

; ============================================================================
; TAG ATTRIBUTES
; ============================================================================

; Attribute name (e.g., type, id, class)
(attribute_name) @attribute

; Assignment operator
(attribute ("=" @operator))

; ============================================================================
; INLINE EXPRESSIONS {{ ... }}
; ============================================================================

; Expression delimiters
(inline_expression
  "{{" @punctuation.bracket
  "}}" @punctuation.bracket)

; ============================================================================
; EXPRESSIONS AND OPERATORS
; ============================================================================

; Variables with $ prefix
(variable "$" @punctuation.special)
(variable (identifier) @variable)

; Identifiers (function names, object keys, etc.)
(identifier) @variable

; Member expressions: object.property
(member_expression
  "." @punctuation.delimiter
  property: (identifier) @property)

; Array access: array[index]
(array_access
  "[" @punctuation.bracket
  "]" @punctuation.bracket)

; Function calls: func() or obj.method()
; Highlight identifier as function in direct calls
(call_expression
  (identifier) @function)

; Highlight property as function in member expression calls
(call_expression
  (member_expression
    property: (identifier) @function))

; Arrow functions: () => expr
(arrow_function
  "(" @punctuation.bracket
  ")" @punctuation.bracket
  "=>" @keyword.operator)

; Arrow function parameters
(arrow_function (identifier) @variable.parameter)

; ============================================================================
; OPERATORS
; ============================================================================

; Binary arithmetic operators
(binary_add) @operator
(binary_subtract) @operator
(binary_multiply) @operator
(binary_divide) @operator
(binary_modulo) @operator

; Binary comparison operators
(binary_equal) @operator
(binary_not_equal) @operator
(binary_less_than) @operator
(binary_greater_than) @operator
(binary_less_equal) @operator
(binary_greater_equal) @operator

; Binary logical operators
(binary_and) @operator
(binary_or) @operator

; Unary operators
(unary_not) @operator
(unary_minus) @operator
(unary_plus) @operator

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

; Object pairs - key: value
; First child is the key (identifier)
(pair
  (identifier) @property
  ":" @punctuation.delimiter)

; Commas in arrays and objects
(array_literal "," @punctuation.delimiter)
(object_literal "," @punctuation.delimiter)
(call_expression "," @punctuation.delimiter)

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
