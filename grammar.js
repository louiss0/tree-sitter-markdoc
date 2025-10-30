/**
 * @file This is a tree sitter parser for markdoc
 * @author Shelton Louis <louisshelton0@gmail.com>
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

module.exports = grammar({
  name: "markdoc",

  externals: $ => [
    $._code_content,
    $._NEWLINE,
    $._BLANK_LINE,
    $._INDENT,
    $._DEDENT
  ],

  extras: $ => [
    /[ \t\n\r]/
  ],

    conflicts: $ => [
      [$.source_file],
      [$.list_item],
      [$.attribute, $.expression],
      [$.paragraph, $._inline_content],
      [$.paragraph],
      [$.list_paragraph],
      [$.attribute, $._primary_expression],
      [$.markdoc_tag, $.paragraph],
      [$.markdoc_tag],
      [$.attribute_value, $._primary_expression],
      [$.tag_open, $.tag_close],
      [$.binary_expression],
      [$.markdown_table],
      [$.markdoc_table],
      [$.markdoc_table_cell_content],
      [$.markdoc_table_cell_annotation],
      [$.markdoc_table_open, $.markdoc_table_close],
      [$.if_tag],
      [$._if_block]
    ],
  rules: {
    source_file: $ => prec.right(optional(seq(
      // Frontmatter ONLY at document start, before any content
      optional($.frontmatter),
      repeat(choice($._NEWLINE, $._BLANK_LINE)),
      // Body content: no frontmatter allowed after this point
      optional(seq(
        $._block,
        repeat(seq(
          repeat1(choice(
            $._BLANK_LINE,
            $._NEWLINE
          )),
          $._block
        ))
      ))
    ))),

    _block: $ => choice(
      $.comment_block,  // Must come before markdoc_tag to match {% comment %}
      $.markdown_table, // Must come before markdoc_tag to match pipe-delimited tables
      $.markdoc_table,  // Must come before markdoc_tag to match list-based tables
      $.if_tag,        // Must come before markdoc_tag to handle if/else structures
      $.markdoc_tag,
      $.fenced_code_block,
      $.heading,
      $.thematic_break,
      $.blockquote,
      $.html_block,
      $.list,
      $.html_comment,
      $.paragraph
    ),

    // Frontmatter: metadata block at document start ONLY
    // Since it only appears before body content, no ambiguity with internal --- separators
    frontmatter: $ => seq(
      token(prec(10, '---')),
      repeat(
        /[^\n]+/  // any non-newline content
      ),
      token(prec(10, '---'))
    ),


    heading: $ => prec.right(2, seq(
      field('heading_marker', $.heading_marker),
      field('heading_text', optional($.heading_text))
    )),

    heading_marker: $ => token(prec(3, /#{1,6}[ \t]/)),  // Require space/tab after #

    heading_text: $ => /[^\n]+/,

    // Thematic break (horizontal rule) - must be at least 3 chars
    thematic_break: $ => token(prec(1, choice(
      /\*\s*\*\s*\*[\s*]*/,
      /-\s*-\s*-[\s-]*/,
      /_\s*_\s*_[\s_]*/
    ))),

    // Blockquote
    blockquote: $ => prec.right(seq(
      token(prec(2, '>')),
      optional($._block)
    )),

    fenced_code_block: $ => seq(
      field('open', $.code_fence_open),
      optional(field('code', $.code)),
      field('close', $.code_fence_close)
    ),

    code_fence_open: $ => seq(
      choice(
        token(prec(3, '```')),
        token(prec(3, '~~~'))
      ),
      optional($.info_string)
    ),

    info_string: $ => seq(
      alias(/[a-zA-Z0-9_+-]+/, $.language),
      optional(alias(/\{[^}\n]*\}/, $.attributes))
    ),

    code: $ => $._code_content,

    code_fence_close: $ => choice(
      token(prec(3, '```')),
      token(prec(3, '~~~'))
    ),

    // Markdoc comment block: {% comment %}...{% /comment %}
    comment_block: $ => seq(
      token(prec(10, seq('{%', /[ \t]*/, 'comment', /[ \t]*/, '%}'))),
      repeat(choice(
        /[^{\n]+/,
        /\{/
      )),
      token(prec(10, seq('{%', /[ \t]*/, '/', /[ \t]*/, 'comment', /[ \t]*/, '%}')))
    ),

    // Markdown table: pipe-delimited table with separator row
    // | Header 1 | Header 2 |
    // |----------|----------|
    // | Cell 1   | Cell 2   |
    markdown_table: $ => prec.dynamic(11, seq(
      $.markdown_table_header,
      $.markdown_table_separator,
      repeat1($.markdown_table_row)
    )),

    markdown_table_header: $ => seq(
      token('|'),
      $.markdown_table_cell,
      repeat(seq(token('|'), $.markdown_table_cell)),
      token('|')
    ),

    markdown_table_separator: $ => seq(
      token('|'),
      $.markdown_table_sep_cell,
      repeat(seq(token('|'), $.markdown_table_sep_cell)),
      token('|')
    ),

    markdown_table_row: $ => seq(
      token('|'),
      $.markdown_table_cell,
      repeat(seq(token('|'), $.markdown_table_cell)),
      token('|')
    ),

    markdown_table_cell: $ => /[^|\n]+/,

    markdown_table_sep_cell: $ => /-+/,

    // Markdoc table tag: {% table %}...{% /table %} with row structure using list markers
    // Structure: header cells, separator (---), row cells, separator, row cells, ...
    markdoc_table: $ => prec.dynamic(12, seq(
      $.markdoc_table_open,
      $.markdoc_table_header,
      repeat1(seq(
        $.markdoc_table_separator,
        $.markdoc_table_row
      )),
      $.markdoc_table_close
    )),

    markdoc_table_open: $ => seq(
      token(prec(6, '{%')),
      alias(token('table'), $.tag_name),
      repeat($.attribute),
      token(prec(6, '%}'))
    ),

    markdoc_table_close: $ => seq(
      token(prec(6, '{%')),
      optional(token('/')),
      alias(token('table'), $.tag_name),
      token(prec(6, '%}'))
    ),

    // Table header: first set of cells before first ---
    markdoc_table_header: $ => repeat1($.markdoc_table_cell),

    // Table row separator: ---
    markdoc_table_separator: $ => token(prec(10, '---')),

    // Table row: set of cells after a separator, continues until next --- or close tag
    markdoc_table_row: $ => repeat1($.markdoc_table_cell),

    // A markdoc table cell: list marker + cell content
    markdoc_table_cell: $ => seq(
      field('marker', $.list_marker),
      field('content', $.markdoc_table_cell_content)
    ),

    // Markdoc table cell content - simplified to handle complex content
    markdoc_table_cell_content: $ => prec.left(1, seq(
      $.markdoc_table_cell_item,
      repeat($.markdoc_table_cell_item)
    )),

    // A markdoc table cell item can be text, tags, code blocks, or other content
    markdoc_table_cell_item: $ => choice(
      $.inline_expression,
      $.inline_tag,
      $.markdoc_table_cell_annotation,
      $.fenced_code_block,
      $.text,
      $.html_inline,
      $.link,
      $.emphasis,
      $.strong,
      $.inline_code,
      $.image,
      alias($.standalone_punct, $.text)
    ),

    // Cell annotation: {% colspan=2 %}
    markdoc_table_cell_annotation: $ => seq(
      token('{%'),
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.annotation_name),
      '=',
      alias(/[0-9]+/, $.annotation_value),
      token('%}')
    ),

    // Block-level Markdoc tag ({% tag %}...{% /tag %} or {% tag /%})
    markdoc_tag: $ => prec.dynamic(10, choice(
      // Self-closing tag
      $.tag_self_close,
      // Full tag with content
      seq(
        $.tag_open,
        choice(
          // Empty tag
          seq(optional($._NEWLINE), $.tag_close),
          // Tag with content
          seq(
            repeat(choice($._NEWLINE, $._BLANK_LINE)),
            $._block,
            repeat(choice(
              seq($._BLANK_LINE, $._block),
              seq($._NEWLINE, $._block)
            )),
            repeat(choice($._NEWLINE, $._BLANK_LINE)),
            $.tag_close
          )
        )
      )
    )),

    tag_open: $ => prec.right(seq(
      token(prec(6, '{%')),
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.tag_name),
      optional($.id_shorthand),
      repeat($.class_shorthand),
      repeat($.attribute),
      token(prec(6, '%}'))
    )),

    tag_close: $ => prec.right(seq(
      token(prec(6, '{%')),
      optional(token('/')),
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.tag_name),
      token(prec(6, '%}'))
    )),

    // Self-closing tag used at block level ({% tag /%})
    tag_self_close: $ => prec.right(seq(
      token(prec(6, '{%')),
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.tag_name),
      optional($.id_shorthand),
      repeat($.class_shorthand),
      repeat($.attribute),
      token(prec(6, '/%}'))
    )),

    // Inline self-closing tag for use in paragraphs/lists
    inline_tag: $ => prec(1, seq(
      token('{%'),
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.tag_name),
      optional($.id_shorthand),
      repeat($.class_shorthand),
      repeat($.attribute),
      token('/%}')
    )),

    // If conditional tag: {% if condition %}...{% else %}...{% /if %}
    if_tag: $ => prec.dynamic(11, seq(
      $.if_tag_open,
      repeat(choice($._NEWLINE, $._BLANK_LINE)),
      $._if_block,
      repeat(choice(
        seq($._BLANK_LINE, $._if_block),
        seq($._NEWLINE, $._if_block)
      )),
      repeat(seq(
        repeat(choice($._NEWLINE, $._BLANK_LINE)),
        $.else_tag,
        repeat(choice($._NEWLINE, $._BLANK_LINE)),
        $._if_block,
        repeat(choice(
          seq($._BLANK_LINE, $._if_block),
          seq($._NEWLINE, $._if_block)
        ))
      )),
      repeat(choice($._NEWLINE, $._BLANK_LINE)),
      $.if_tag_close
    )),

    // Blocks inside if_tags - includes most block content
    // We use a negative lookahead approach by not matching 'else' tags
    _if_block: $ => choice(
      $.comment_block,
      $.markdown_table,
      $.markdoc_table,
      // Self-closing tags at block level (but not 'else' which is handled by else_tag)
      $.tag_self_close,
      // Full tags with content (but not 'else' which is handled by else_tag)
      seq(
        $.tag_open,
        choice(
          seq(optional($._NEWLINE), $.tag_close),
          seq(
            repeat(choice($._NEWLINE, $._BLANK_LINE)),
            $._block,
            repeat(choice(
              seq($._BLANK_LINE, $._block),
              seq($._NEWLINE, $._block)
            )),
            repeat(choice($._NEWLINE, $._BLANK_LINE)),
            $.tag_close
          )
        )
      ),
      $.fenced_code_block,
      $.heading,
      $.thematic_break,
      $.blockquote,
      $.html_block,
      $.list,
      $.html_comment,
      $.paragraph
    ),

    // Opening if tag: {% if condition %}
    if_tag_open: $ => seq(
      token(prec(6, '{%')),
      token('if'),
      field('condition', $.expression),
      token(prec(6, '%}'))
    ),

    // Else tag (with optional condition): {% else %} or {% else condition /%}
    else_tag: $ => seq(
      token(prec(6, '{%')),
      token('else'),
      optional(field('condition', $.expression)),
      token(prec(6, '/%}'))
    ),

    // Closing if tag: {% /if %}
    if_tag_close: $ => seq(
      token(prec(6, '{%')),
      optional(token('/')),
      token('if'),
      token(prec(6, '%}'))
    ),

    attribute: $ => seq(
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.attribute_name),
      '=',
      $.attribute_value
    ),

    id_shorthand: $ => prec.right(seq(
      token('#'),
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.shorthand_id)
    )),

    class_shorthand: $ => prec.right(seq(
      token('.'),
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.shorthand_class)
    )),

    attribute_value: $ => choice(
      $.string,
      $.expression
    ),

    // Top-level expressions have highest precedence to contain their
    // own operations and prevent ambiguity with attribute_value
    expression: $ => prec.left(choice(
      $.arrow_function,
      $.binary_expression,
      $.unary_expression,
      $.call_expression,
      $.member_expression,
      $.array_access,
      $._primary_expression
    )),

    // Atomic expressions
    _primary_expression: $ => choice(
      $.variable,
      $.identifier,
      $.string,
      $.number,
      $.array_literal,
      $.object_literal,
      $.boolean,
      $.null,
      $.parenthesized_expression
    ),

    // Parenthesized expressions for grouping
    parenthesized_expression: $ => seq(
      '(',
      $.expression,
      ')'
    ),

    // Boolean literals
    boolean: $ => choice('true', 'false'),

    // Null literal
    null: $ => 'null',

    // Alias: object is same as object_literal for test compatibility
    object: $ => $.object_literal,

    // Alias: array is same as array_literal for test compatibility  
    array: $ => $.array_literal,

    // Variable prefixed with $
    variable: $ => seq('$', $.identifier),

    // Arrow function: () => expr or (params) => expr
    arrow_function: $ => prec(10, seq(
      '(',
      optional(seq(
        $.identifier,
        repeat(seq(',', $.identifier))
      )),
      ')',
      '=>',
      $.expression
    )),

    // Operator tokens (named for highlighting)
    // Arithmetic operators
    binary_add: $ => token(prec(5, '+')),
    binary_subtract: $ => token(prec(5, '-')),
    binary_multiply: $ => token(prec(5, '*')),
    binary_divide: $ => token(prec(5, '/')),
    binary_modulo: $ => token(prec(5, '%')),
    
    // Comparison operators
    binary_equal: $ => token(prec(5, '==')),
    binary_not_equal: $ => token(prec(5, '!=')),
    binary_less_than: $ => token(prec(5, '<')),
    binary_greater_than: $ => token(prec(5, '>')),
    binary_less_equal: $ => token(prec(5, '<=')),
    binary_greater_equal: $ => token(prec(5, '>=')),
    
    // Logical operators
    binary_and: $ => token(prec(5, '&&')),
    binary_or: $ => token(prec(5, '||')),
    
    // Unary operators
    unary_not: $ => '!',
    unary_minus: $ => '-',
    unary_plus: $ => '+',

    // Binary operators (in order of precedence)
    // Use token(prec()) for operators to give them priority over conflicting markdown tokens
    // Add spaces around operators for proper parsing
    binary_expression: $ => choice(
      prec.left(1, seq(field('left', $.expression), field('operator', $.binary_or), field('right', $.expression))),
      prec.left(2, seq(field('left', $.expression), field('operator', $.binary_and), field('right', $.expression))),
      prec.left(3, seq(field('left', $.expression), field('operator', $.binary_equal), field('right', $.expression))),
      prec.left(3, seq(field('left', $.expression), field('operator', $.binary_not_equal), field('right', $.expression))),
      prec.left(4, seq(field('left', $.expression), field('operator', $.binary_less_than), field('right', $.expression))),
      prec.left(4, seq(field('left', $.expression), field('operator', $.binary_greater_than), field('right', $.expression))),
      prec.left(4, seq(field('left', $.expression), field('operator', $.binary_less_equal), field('right', $.expression))),
      prec.left(4, seq(field('left', $.expression), field('operator', $.binary_greater_equal), field('right', $.expression))),
      prec.left(5, seq(field('left', $.expression), field('operator', $.binary_add), field('right', $.expression))),
      prec.left(5, seq(field('left', $.expression), field('operator', $.binary_subtract), field('right', $.expression))),
      prec.left(6, seq(field('left', $.expression), field('operator', $.binary_multiply), field('right', $.expression))),
      prec.left(6, seq(field('left', $.expression), field('operator', $.binary_divide), field('right', $.expression))),
      prec.left(6, seq(field('left', $.expression), field('operator', $.binary_modulo), field('right', $.expression)))
    ),

    // Unary operators
    unary_expression: $ => choice(
      prec.right(7, seq(field('operator', $.unary_not), field('argument', $.expression))),
      prec.right(7, seq(field('operator', $.unary_minus), field('argument', $.expression))),
      prec.right(7, seq(field('operator', $.unary_plus), field('argument', $.expression)))
    ),

    // Ordered by precedence - call > member > array access 
    call_expression: $ => prec.left(4, seq(
      choice(
        $.member_expression,
        $.identifier
      ),
      '(',
      optional(seq(
        $.expression,
        repeat(seq(',', $.expression))
      )),
      ')'
    )),

    member_expression: $ => prec.right(3, seq(
      field('object', choice(
        $.member_expression,
        $.array_access,
        $._primary_expression
      )),
      '.',
      field('property', $.identifier)
    )),

    array_access: $ => prec.right(2, seq(
      field('array', choice(
        $.member_expression,
        $.array_access,
        $._primary_expression
      )),
      '[',
      field('index', $.expression),
      ']'
    )),

    // Alias for backward compatibility with tests
    subscript_expression: $ => $.array_access,

    array_literal: $ => seq(
      '[',
      optional(seq(
        $.expression,
        repeat(seq(',', $.expression)),
        optional(',')
      )),
      ']'
    ),

    object_literal: $ => seq(
      '{',
      optional(seq(
        $.pair,
        repeat(seq(',', $.pair)),
        optional(',')
      )),
      '}'
    ),

    pair: $ => seq(
      $.identifier,
      ':',
      $.expression
    ),

    identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,

    string: $ => choice(
      seq(
        '"',
        repeat(choice(
          /[^"\\\n]/,
          seq('\\', /./)
        )),
        '"'
      ),
      seq(
        "'",
        repeat(choice(
          /[^'\\\n]/,
          seq('\\', /./)
        )),
        "'"
      )
    ),

    number: $ => /-?[0-9]+(\.[0-9]+)?/,

    inline_expression: $ => seq(
      '{{',
      field('content', $.expression),
      '}}'
    ),

    // Lists: one or more list items
    // Items can be separated by optional newlines (handled by extras)
    // Nested lists use INDENT/DEDENT tokens from scanner
    list: $ => prec.right(seq(
      $.list_item,
      repeat(choice(
        // Nested list with indentation
        prec(2, seq($._INDENT, $.list, $._DEDENT)),
        // Another item at same level (newlines handled by extras)
        prec(1, seq(
          optional($._NEWLINE),
          $.list_item
        ))
      ))
    )),

    // A list item: marker + list_paragraph content + optional annotation
    list_item: $ => seq(
      field('marker', $.list_marker),
      field('content', $.list_paragraph),
      optional($.list_item_annotation)
    ),

    list_marker: $ => token(prec(2, choice(
      /[-*+][ \t]+/,
      /[0-9]+\.[ \t]+/
    ))),

    // List item annotation: {% type %} or {% type attr="value" %}
    list_item_annotation: $ => seq(
      token(prec(6, '{%')),
      field('type', alias(token(/[a-zA-Z_][a-zA-Z0-9_-]*/), $.annotation_type)),
      repeat($.attribute),
      token(prec(6, '%}'))
    ),

    html_comment: $ => token(seq(
      '<!--',
      repeat(choice(
        /[^-]+/,
        /-[^-]/
      )),
      '-->'
    )),

    html_block: $ => choice(
      // Multi-line HTML block (opening tag at start of line)
      token(prec(2, seq(
        '<',
        /[a-zA-Z][a-zA-Z0-9]*/,
        /[^>]*/,
        '>',
        /[\s\S]*?/,
        '</',
        /[a-zA-Z][a-zA-Z0-9]*/,
        '>'
      ))),
      // Self-closing HTML tag
      token(prec(2, seq(
        '<',
        /[a-zA-Z][a-zA-Z0-9]*/,
        /[^/]*?/,
        '/>'
      )))
    ),

    html_inline: $ => choice(
      // Opening and closing tag
      token(prec(1, seq(
        '<',
        /[a-zA-Z][a-zA-Z0-9]*/,
        optional(/[^>]*/),
        '>',
        optional(/[^<]*/),
        '</',
        /[a-zA-Z][a-zA-Z0-9]*/,
        '>'
      ))),
      // Self-closing tag
      token(prec(1, seq(
        '<',
        /[a-zA-Z][a-zA-Z0-9]*/,
        optional(/[^>]*/),
        '/>'
      )))
    ),

    // Paragraph: consecutive lines of content (separated by single newlines, not double)
    paragraph: $ => prec.left(1, seq(
      $._inline_first,
      repeat($._inline_content),
      repeat(seq(
        $._NEWLINE,
        seq($._inline_first, repeat($._inline_content))
      ))
    )),

    // List paragraph: same as paragraph but semantically distinct for list items
    list_paragraph: $ => prec.left(1, seq(
      $._inline_first,
      repeat($._inline_content),
      repeat(seq(
        $._NEWLINE,
        seq($._inline_first, repeat($._inline_content))
      ))
    )),

    emphasis: $ => prec.left(1, choice(
      seq('*', token(prec(1, /[^*\n]+/)), '*'),
      seq('_', token(prec(1, /[^_\n]+/)), '_')
    )),

    strong: $ => prec.left(2, choice(
      seq('**', token(prec(2, /[^*\n]+/)), '**'),
      seq('__', token(prec(2, /[^_\n]+/)), '__')
    )),

    inline_code: $ => seq(
      '`',
      token(prec(1, /[^`\n]+/)),
      '`'
    ),

    link: $ => seq(
      '[',
      alias(token(prec(1, /[^\]\n]+/)), $.link_text),
      ']',
      '(',
      alias(token(prec(1, /[^)\n]+/)), $.link_destination),
      ')'
    ),

    image: $ => seq(
      token('!['),
      alias(token(prec(1, /[^\]\n]+/)), $.image_alt),
      ']',
      '(',
      alias(token(prec(1, /[^)\n]+/)), $.image_destination),
      ')'
    ),

    // First inline element in a paragraph/line
    _inline_first: $ => choice(
      $.inline_expression,
      $.inline_tag,
      $.text,
      $.html_inline,
      $.link,
      $.emphasis,
      $.strong,
      $.inline_code
    ),

    // Inline content after first element
    _inline_content: $ => choice(
      $._inline_first,
      $.image,
      alias($.standalone_punct, $.text)
    ),

    text: $ => token(/[^\n{<`*_\[\]!\->]+/),
    
    // Fallback for standalone punctuation that doesn't start special syntax
    standalone_punct: $ => '!',
  }
});
