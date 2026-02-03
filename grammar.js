/**
 * @file This is a tree sitter parser for markdoc
 * @author Shelton Louis <louisshelton0@gmail.com>
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

const WS = /[ \t]+/;
const WS1 = /[ \t]+/;
const TAG_WS = /[ \t\r\n]+/;
const TAG_WS1 = /[ \t\r\n]+/;
const HTML_TAG_NAME = /[a-zA-Z][a-zA-Z0-9]*/;
const LIST_MARKER_MINUS_FALLBACK = /[ \t]*-[ \t]+/;
const LIST_MARKER_PLUS_FALLBACK = /[ \t]*\+[ \t]+/;
const LIST_MARKER_STAR_FALLBACK = /[ \t]*\*[ \t]+/;
const LIST_MARKER_DOT_FALLBACK = /[ \t]*[0-9]{1,9}\.[ \t]+/;
const LIST_MARKER_PAREN_FALLBACK = /[ \t]*[0-9]{1,9}\)[ \t]+/;

module.exports = grammar({
  name: "markdoc",

  externals: $ => [
    $._CODE_CONTENT,
    $._FRONTMATTER_DELIM,
    $._LIST_CONTINUATION,
    $._SOFT_LINE_BREAK,
    $._THEMATIC_BREAK,
    $._HTML_COMMENT,
    $._HTML_BLOCK
  ],

  extras: $ => [],

  conflicts: $ => [
    [$.source_file],
    [$.markdoc_tag],
    [$.markdoc_tag, $._inline_line_start],
    [$.tag_self_close, $._primary_expression],
    [$.tag_open, $.tag_self_close, $._primary_expression],
    [$.binary_expression],
    [$.attribute_value, $._primary_expression]
  ],

  inline: $ => [
    $.line_break
  ],

  rules: {
    source_file: $ => choice(
      prec(1, seq(
        $.frontmatter,
        repeat(choice($._BLANK_LINE, $._NEWLINE)),
        optional(seq(
          $._block,
          repeat(seq(
            choice($._BLANK_LINE, $._NEWLINE),
            $._block
          ))
        )),
        repeat(choice($._BLANK_LINE, $._NEWLINE))
      )),
      prec.right(seq(
        repeat($._NEWLINE),
        optional(seq(
          $._block,
          repeat(seq(
            choice($._BLANK_LINE, $._NEWLINE),
            $._block
          ))
        )),
        repeat(choice($._BLANK_LINE, $._NEWLINE))
      ))
    ),

    _block: $ => choice(
      $.comment_block,  // Must come before markdoc_tag to match {% comment %}
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

    frontmatter: $ => seq(
      $._FRONTMATTER_DELIM,
      $._NEWLINE,
      alias($.yaml_content, $.yaml),
      $._FRONTMATTER_DELIM
    ),

    yaml_content: $ => repeat1(
      seq(/[^\n]+/, $._NEWLINE)
    ),


    heading: $ => prec.right(2, seq(
      field('heading_marker', $.heading_marker),
      field('heading_text', optional($.heading_text))
    )),

    heading_marker: $ => token(prec(3, /#{1,6}[ \t]/)),  // Require space/tab after #
    heading_text: $ => token(prec(1, /[^\r\n]+/)),

    // Thematic break (horizontal rule)
    thematic_break: $ => prec.dynamic(1, $._THEMATIC_BREAK),

    // Blockquote (consume consecutive > lines as a single block)
    blockquote: $ => token(prec(2, />[^\r\n]*(\r?\n>[^\r\n]*)*\r?\n?/)),

    fenced_code_block: $ => seq(
      field('open', $.code_fence_open),
      optional(field('code', $.code)),
      optional($._NEWLINE),
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
      optional(WS),
      optional(alias(/\{[^}\n]*\}/, $.attributes))
    ),

    code: $ => $._CODE_CONTENT,

    code_fence_close: $ => choice(
      token(prec(3, '```')),
      token(prec(3, '~~~'))
    ),

    // Markdoc comment block: {% comment %}...{% /comment %}
    comment_block: $ => token(prec(6, /\{%\s*comment\s*%\}(.|\r|\n)*\{%\s*\/comment\s*%\}/)),

    // Block-level Markdoc tag ({% tag %}...{% /tag %} or {% tag /%})
    markdoc_tag: $ => prec.dynamic(4, choice(
      // Self-closing tag on its own line
      seq(
        $.tag_self_close,
        $.line_break
      ),
      // Full tag with content on following lines
      seq(
        $.tag_open,
        choice(
          // Empty tag
          seq(
            repeat(choice($._NEWLINE, $._BLANK_LINE)),
            $.tag_close
          ),
          // Tag with content
          seq(
            repeat(choice($._NEWLINE, $._BLANK_LINE)),
            $._block,
            repeat(choice(
              seq($._BLANK_LINE, $._block),
              seq($._NEWLINE, $._block)
            )),
            repeat1(choice($._NEWLINE, $._BLANK_LINE)),
            $.tag_close
          )
        )
      )
    )),

    tag_open: $ => prec.right(seq(
      token(prec(6, '{%')),
      optional(WS),
      alias($.identifier, $.tag_name),
      repeat(seq(WS1, $.attribute)),
      token(prec(6, /[ \t]*%}\r?\n/))
    )),

    tag_close: $ => prec.right(seq(
      token(prec(6, '{%')),
      optional(WS),
      seq(token('/'), optional(WS)),
      alias($.identifier, $.tag_name),
      token(prec(6, /[ \t]*%}/))
    )),

    // Inline self-closing tag for use in paragraphs/lists
    inline_tag: $ => prec(1, choice(
      $.tag_self_close
    )),

    tag_self_close: $ => prec.right(seq(
      token(prec(6, '{%')),
      optional(WS),
      alias($.identifier, $.tag_name),
      repeat(seq(WS1, $.attribute)),
      token(prec(6, /[ \t]*\/%}/))
    )),

    attribute: $ => seq(
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.attribute_name),
      '=',
      $.attribute_value
    ),

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

    _tag_expression: $ => choice(
      $.variable,
      $.call_expression,
      $.member_expression,
      $.array_access,
      $.parenthesized_expression
    ),

    // Parenthesized expressions for grouping
    parenthesized_expression: $ => seq(
      '(',
      optional(WS),
      $.expression,
      optional(WS),
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
      optional(WS),
      optional(seq(
        $.identifier,
        repeat(seq(optional(WS), ',', optional(WS), $.identifier))
      )),
      optional(WS),
      ')',
      optional(WS),
      '=>',
      optional(WS),
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
      prec.left(1, seq(field('left', $.expression), optional(WS), field('operator', $.binary_or), optional(WS), field('right', $.expression))),
      prec.left(2, seq(field('left', $.expression), optional(WS), field('operator', $.binary_and), optional(WS), field('right', $.expression))),
      prec.left(3, seq(field('left', $.expression), optional(WS), field('operator', $.binary_equal), optional(WS), field('right', $.expression))),
      prec.left(3, seq(field('left', $.expression), optional(WS), field('operator', $.binary_not_equal), optional(WS), field('right', $.expression))),
      prec.left(4, seq(field('left', $.expression), optional(WS), field('operator', $.binary_less_than), optional(WS), field('right', $.expression))),
      prec.left(4, seq(field('left', $.expression), optional(WS), field('operator', $.binary_greater_than), optional(WS), field('right', $.expression))),
      prec.left(4, seq(field('left', $.expression), optional(WS), field('operator', $.binary_less_equal), optional(WS), field('right', $.expression))),
      prec.left(4, seq(field('left', $.expression), optional(WS), field('operator', $.binary_greater_equal), optional(WS), field('right', $.expression))),
      prec.left(5, seq(field('left', $.expression), optional(WS), field('operator', $.binary_add), optional(WS), field('right', $.expression))),
      prec.left(5, seq(field('left', $.expression), optional(WS), field('operator', $.binary_subtract), optional(WS), field('right', $.expression))),
      prec.left(6, seq(field('left', $.expression), optional(WS), field('operator', $.binary_multiply), optional(WS), field('right', $.expression))),
      prec.left(6, seq(field('left', $.expression), optional(WS), field('operator', $.binary_divide), optional(WS), field('right', $.expression))),
      prec.left(6, seq(field('left', $.expression), WS1, field('operator', $.binary_modulo), WS1, field('right', $.expression)))
    ),

    // Unary operators
    unary_expression: $ => choice(
      prec.right(7, seq(field('operator', $.unary_not), optional(WS), field('argument', $.expression))),
      prec.right(7, seq(field('operator', $.unary_minus), optional(WS), field('argument', $.expression))),
      prec.right(7, seq(field('operator', $.unary_plus), optional(WS), field('argument', $.expression)))
    ),

    // Ordered by precedence - call > member > array access 
    call_expression: $ => prec.left(4, seq(
      choice(
        $.member_expression,
        $.identifier
      ),
      '(',
      optional(WS),
      optional(seq(
        $.expression,
        repeat(seq(optional(WS), ',', optional(WS), $.expression))
      )),
      optional(WS),
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
      optional(WS),
      field('index', $.expression),
      optional(WS),
      ']'
    )),

    // Alias for backward compatibility with tests
    subscript_expression: $ => $.array_access,

    array_literal: $ => seq(
      '[',
      optional(WS),
      optional(seq(
        $.expression,
        repeat(seq(optional(WS), ',', optional(WS), $.expression)),
        optional(seq(optional(WS), ','))
      )),
      optional(WS),
      ']'
    ),

    object_literal: $ => seq(
      '{',
      optional(WS),
      optional(seq(
        $.pair,
        repeat(seq(optional(WS), ',', optional(WS), $.pair)),
        optional(seq(optional(WS), ','))
      )),
      optional(WS),
      '}'
    ),

    pair: $ => seq(
      $.identifier,
      optional(WS),
      ':',
      optional(WS),
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
      token(prec(6, '{%')),
      optional(WS),
      field('content', choice(
        $.expression,
        $.emphasis,
        $.strong
      )),
      optional(WS),
      token(prec(10, '%}'))
    ),

    // Lists
    list: $ => prec.dynamic(2, seq(
      $.list_item,
      repeat($.list_item)
    )),
    list_item: $ => seq(
      field('marker', $.list_marker),
      $._list_item_content,
      $._block_close
    ),

    list_marker: $ => choice(
      token(prec(2, LIST_MARKER_MINUS_FALLBACK)),
      token(prec(2, LIST_MARKER_PLUS_FALLBACK)),
      token(prec(2, LIST_MARKER_STAR_FALLBACK)),
      token(prec(2, LIST_MARKER_DOT_FALLBACK)),
      token(prec(2, LIST_MARKER_PAREN_FALLBACK))
    ),

    _block_close: $ => $._NEWLINE,
    _list_item_content: $ => $.list_paragraph,

    line_break: $ => choice(
      $._BLANK_LINE,
      $._NEWLINE
    ),

    html_comment: $ => $._HTML_COMMENT,

    html_block: $ => $._HTML_BLOCK,

    html_inline: $ => choice(
      // Opening and closing tag
      token(prec(1, seq(
        '<',
        HTML_TAG_NAME,
        optional(/[^\n>]*/),
        '>',
        optional(/[^\n<]*/),
        '</',
        HTML_TAG_NAME,
        '>'
      ))),
      // Self-closing tag
      token(prec(1, seq(
        '<',
        HTML_TAG_NAME,
        optional(/[^\n>]*/),
        '/>'
      )))
    ),

    // Paragraph: consecutive lines of content (separated by single newlines, not double)
    paragraph: $ => prec.left(-1, seq(
      $._inline_line_start,
      repeat($._inline_content),
      repeat(seq(
        $._SOFT_LINE_BREAK,
        choice(
          $._inline_expression_line,
          seq($._inline_line_start_no_expression, repeat($._inline_content))
        )
      ))
    )),

    // List paragraph: same as paragraph but semantically distinct for list items
    list_paragraph: $ => prec.right(1, seq(
      $._inline_line_start,
      repeat($._inline_content),
      repeat(seq(
        $._LIST_CONTINUATION,
        choice(
          $._inline_expression_line,
          seq($._inline_line_start_no_expression, repeat($._inline_content))
        )
      ))
    )),

    emphasis: $ => choice(
      token(prec(2, /\*[^\s*]\*/)),
      token(prec(2, /\*[^\s*][^*\n]*[^\s*]\*/)),
      token(prec(2, /_[^\s_]\_/)),
      token(prec(2, /_[^\s_][^_\n]*[^\s_]\_/))
    ),

    strong: $ => choice(
      token(prec(3, /\*\*[^\s*]\*\*/)),
      token(prec(3, /\*\*[^\s*][^*\n]*[^\s*]\*\*/)),
      token(prec(3, /__[^\s_]\__/)),
      token(prec(3, /__[^\s_][^_\n]*[^\s_]\__/))
    ),

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

    _inline_line_start: $ => choice(
      $.inline_expression,
      $.tag_self_close,
      $.text,
      $.html_inline,
      $.link,
      $.emphasis,
      $.strong,
      $.inline_code,
      $.image,
      alias($.standalone_punct, $.text)
    ),

    _inline_line_start_no_expression: $ => choice(
      $.tag_self_close,
      $.text,
      $.html_inline,
      $.link,
      $.emphasis,
      $.strong,
      $.inline_code,
      $.image,
      alias($.standalone_punct, $.text)
    ),

    _inline_expression_line: $ => seq(
      $.inline_expression,
      repeat1($._inline_content)
    ),

    // Inline content after first element
    _inline_content: $ => choice(
      $._inline_first,
      $.image,
      alias($.standalone_punct, $.text)
    ),

    text: $ => token(prec(1, /[^\r\n{\[<!`*_]+/)),
    
    // Fallback for standalone punctuation that doesn't start special syntax
    standalone_punct: $ => token(/[!_*]/),

    _NEWLINE: $ => token(/\r?\n/),
    _BLANK_LINE: $ => token(/\r?\n[ \t]*\r?\n+/)

  }
});
