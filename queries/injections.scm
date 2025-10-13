; Language injection queries for Markdoc
; Enable language-specific highlighting in fenced code blocks

((fenced_code_block
  (code_fence_open
    (info_string
      (language) @injection.language))
  (code) @injection.content))
