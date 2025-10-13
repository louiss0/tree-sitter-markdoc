; Syntax highlighting queries for Markdoc

; Frontmatter (YAML)
(frontmatter) @markup.raw.block

; Headings
(heading_marker) @markup.heading.marker
(heading_text) @markup.heading

; Code blocks
(code_fence_open) @punctuation.bracket
(code_fence_close) @punctuation.bracket
(language) @property
(code) @markup.raw.block

; Markdoc tags
("{%" @punctuation.bracket)
("%}" @punctuation.bracket)
("/%}" @punctuation.bracket)
(tag_name) @tag
("/" @punctuation.delimiter) ; For closing tags

; Tag attributes
(attribute_name) @attribute
("=" @operator)

; Inline expressions
(inline_expression
  "{{" @punctuation.bracket
  "}}" @punctuation.bracket)

; Expressions and values
(identifier) @variable
(member_expression) @variable.member
(number) @number
(string) @string
("." @punctuation.delimiter) ; Member access

; Attributes
(attributes) @attribute

; Paragraph text
(text) @text
