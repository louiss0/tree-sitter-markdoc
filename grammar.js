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
  ],

  extras: $ => [
    /[\t\r]/  // Horizontal whitespace
  ],

  conflicts: $ => [
    [$.source_file],
    [$.list_item],
    [$.attribute, $.expression],
    [$.code_fence_close],
    [$.paragraph, $._inline_content],
    [$.paragraph],
    [$.attribute, $._primary_expression],
    [$.markdoc_tag, $.paragraph],
    [$.attribute_value, $._primary_expression],
    [$.tag_open, $.tag_close]
  ],

  rules: {
    source_file: $ => prec.right(seq(
      repeat(/\n/),
      optional(seq(
        choice(
          $.frontmatter,
          $._block
        ),
        repeat(seq(
          choice(
            $._BLANK_LINE,  // Scanner emits for blank lines or block boundaries
            /\n/             // Fallback for single newlines between non-paragraph blocks
          ),
          choice(
            $.frontmatter,
            $._block
          )
        ))
      )),
      repeat(/\n/)
    )),

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
      token(prec(10, '---')),
      /\n/,
      alias($.yaml_content, $.yaml),
      token(prec(10, '---'))
    ),

    yaml_content: $ => repeat1(
      choice(
        /[^\n-][^\n]*\n/,
        /\n/
      )
    ),


    heading: $ => prec.right(2, seq(
      field('heading_marker', $.heading_marker),
      field('heading_text', optional($.heading_text))
    )),

    heading_marker: $ => token(prec(1, /#{1,6}[ \t]+/)),

    heading_text: $ => /[^\n]+/,

    // Thematic break (horizontal rule) - must be at least 3 chars
    thematic_break: $ => token(prec(1, choice(
      /\*[ \t]*\*[ \t]*\*[ \t*]*/,
      /-[ \t]*-[ \t]*-[ \t-]*/,
      /_[ \t]*_[ \t]*_[ \t_]*/
    ))),

    // Blockquote
    blockquote: $ => prec.right(seq(
      token(prec(2, seq('>', optional(/[ \t]+/)))),
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
      optional($.info_string),
      /\n/
    ),

    info_string: $ => seq(
      alias(/[a-zA-Z0-9_+-]+/, $.language),
      optional(seq(
        /[ \t]+/,
        alias(/\{[^}\n]*\}/, $.attributes)
      ))
    ),

    code: $ => $._code_content,

    code_fence_close: $ => seq(
      optional(/[ \t]*/),
      choice(
        token(prec(3, '```')),
        token(prec(3, '~~~'))
      ),
      optional(/\n/)
    ),

    // Markdoc comment block: {% comment %}...{% /comment %}
    comment_block: $ => seq(
      token(prec(10, seq('{%', /[ \t]*/, 'comment', /[ \t]*/, '%}'))),
      repeat(choice(
        /[^{\n]+/,
        /\n/,
        /\{/
      )),
      token(prec(10, seq('{%', /[ \t]*/, '\/', /[ \t]*/, 'comment', /[ \t]*/, '%}')))
    ),

    markdoc_tag: $ => prec.dynamic(10, choice(
      seq(
        $.tag_open,
        repeat(seq(
          repeat(/\n/),
          $._block
        )),
        $.tag_close
      ),
      $.tag_self_close
    )),

    tag_open: $ => prec.right(seq(
      token(prec(6, seq('{%', /[ \t]*/))),
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.tag_name),
      repeat(seq(
        /[ \t]+/,
        $.attribute
      )),
      token(prec(6, seq(/[ \t]*/, '%}'))),
      optional(/\n/)
    )),

    tag_close: $ => prec.right(seq(
      repeat(/\n/),
      token(prec(6, seq('{%', /[ \t]*/))),
      optional(token('/')),  // Allow optional slash for alternative syntax
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.tag_name),
      token(prec(6, seq(/[ \t]*/, '%}'))),
      optional(/\n/)
    )),

    tag_self_close: $ => prec.right(seq(
      token(prec(6, seq('{%', /[ \t]*/))),
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.tag_name),
      repeat(seq(
        /[ \t]+/,
        $.attribute
      )),
      token(prec(6, seq(/[ \t]*/, '/%}'))),
      optional(/\n/)
    )),

    attribute: $ => seq(
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.attribute_name),
      /[ \t]*/,
      '=',
      /[ \t]*/,
      $.attribute_value
    ),

    attribute_value: $ => choice(
      $.string,
      $.expression
    ),

    // Top-level expressions have highest precedence to contain their
    // own operations and prevent ambiguity with attribute_value
    expression: $ => choice(
      $.binary_expression,
      $.unary_expression,
      $.call_expression,
      $.member_expression,
      $.array_access,
      $._primary_expression
    ),

    // Atomic expressions
    _primary_expression: $ => choice(
      $.variable,
      $.identifier,
      $.string,
      $.number,
      $.array_literal,
      $.object_literal,
      $.boolean,
      $.null
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

    // Binary operators (in order of precedence)
    binary_expression: $ => choice(
      prec.left(1, seq($.expression, '||', $.expression)),
      prec.left(2, seq($.expression, '&&', $.expression)),
      prec.left(3, seq($.expression, choice('==', '!='), $.expression)),
      prec.left(4, seq($.expression, choice('<', '>', '<=', '>='), $.expression)),
      prec.left(5, seq($.expression, choice('+', '-'), $.expression)),
      prec.left(6, seq($.expression, choice('*', '/', '%'), $.expression))
    ),

    // Unary operators
    unary_expression: $ => prec.right(7, seq(
      choice('!', '-', '+'),
      $.expression
    )),

    // Ordered by precedence - call > member > array access 
    call_expression: $ => prec.left(4, seq(
      field('function', choice(
        $.member_expression,
        $.identifier
      )),
      field('arguments', $.arguments)
    )),

    arguments: $ => seq(
      '(',
      optional(seq(
        $.expression,
        repeat(seq(',', /[ \t]*/, $.expression))
      )),
      ')'
    ),

    member_expression: $ => prec.right(3, seq(
      field('object', choice(
        $.member_expression,
        $._primary_expression
      )),
      '.',
      field('property', $.identifier)
    )),

    array_access: $ => prec.right(2, seq(
      field('array', choice(
        $.member_expression,
        $._primary_expression
      )),
      '[',
      field('index', $.expression),
      ']'
    )),

    array_literal: $ => seq(
      '[',
      optional(seq(
        $.expression,
        repeat(seq(',', /[ \t]*/, $.expression)),
        optional(',')  // trailing comma
      )),
      ']'
    ),

    object_literal: $ => seq(
      '{',
      optional(seq(
        $.pair,
        repeat(seq(',', /[ \t]*/, $.pair)),
        optional(',')  // trailing comma
      )),
      '}'
    ),

    pair: $ => seq(
      $.identifier,
      ':',
      /[ \t]*/,
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
      repeat(/[ \t]/),
      field('content', $.expression),
      repeat(/[ \t]/),
      '}}'
    ),

    // Lists: one or more list items separated by a single newline
    // A blank line between items terminates the list (separate list)
    list: $ => prec.right(seq(
      $.list_item,
      repeat(seq(/\n/, $.list_item)),
      optional(/\n/)  // Optional trailing newline after last item
    )),

    // A list item with content and optional continuation lines
    list_item: $ => prec.right(seq(
      field('marker', $.list_marker),
      field('content', $.paragraph),
      optional(seq(
        /\n/,
        $.list
      ))
    )),

    list_marker: $ => token(prec(2, choice(
      /[-*+][ \t]+/,
      /[0-9]+\.[ \t]+/
    ))),

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
        $._NEWLINE,  // Use ONLY scanner token for context-aware line continuation
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
      '!',
      '[',
      alias(token(prec(1, /[^\]\n]+/)), $.image_alt),
      ']',
      '(',
      alias(token(prec(1, /[^)\n]+/)), $.image_destination),
      ')'
    ),

    // First inline element in a paragraph/line
    _inline_first: $ => choice(
      $.inline_expression,
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
      $.image
    ),

    text: $ => token(/[^\n{<`*_\[\]!]+/),
  }
});
