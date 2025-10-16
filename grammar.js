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
    $._code_content
  ],

  extras: $ => [
    /[\t\r]/  // Horizontal whitespace
  ],

  conflicts: $ => [
    [$.source_file],
    [$.list_item],
    [$.list_item, $._list_item_content],
    [$.attribute, $.expression],
    [$.code_fence_close],
    [$.paragraph, $._inline_content],
    [$.paragraph],
    [$.attribute, $._primary_expression],
    [$.markdoc_tag, $.paragraph]
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
          repeat1(/\n/),
          choice(
            $.frontmatter,
            $._block
          )
        ))
      )),
      repeat(/\n/)
    )),

    _block: $ => choice(
      $.markdoc_tag,
      $.fenced_code_block,
      $.heading,
      $.html_block,
      $.list,
      $.html_comment,
      $.paragraph
    ),

    frontmatter: $ => seq(
      token(prec(2, '---')),
      /\n/,
      alias($.yaml_content, $.yaml),
      token(prec(2, '---'))
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

    markdoc_tag: $ => prec.dynamic(10, choice(
      seq(
        $.tag_open,
        repeat($._block),
        $.tag_close
      ),
      $.tag_self_close
    )),

    tag_open: $ => prec.right(seq(
      token(prec(6, seq('{%', /[ \t]*/))),
      field('name', alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, 'tag_name')),
      repeat(seq(
        /[ \t]+/,
        $.attribute
      )),
      token(prec(6, seq(/[ \t]*/, '%}'))),
      optional(/\n/)
    )),

    tag_close: $ => prec.right(seq(
      token(prec(6, seq('{%', /[ \t]*/))),
      token('/'),
      field('name', alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, 'tag_name')),
      token(prec(6, seq(/[ \t]*/, '%}'))),
      optional(/\n/)
    )),

    tag_self_close: $ => prec.right(seq(
      token(prec(6, seq('{%', /[ \t]*/))),
      field('name', alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, 'tag_name')),
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
      field('attribute_value', choice(
        $.string,
        $.expression
      ))
    ),

    // Top-level expressions have highest precedence to contain their
    // own operations and prevent ambiguity with attribute_value
    expression: $ => prec.right(3, choice(
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
      $.object_literal
    ),

    // Variable prefixed with $
    variable: $ => seq('$', $.identifier),

    // Ordered by precedence - call > member > array access 
    call_expression: $ => prec.right(4, seq(
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

    string: $ => seq(
      '"',
      repeat(choice(
        /[^"\\\n]/,
        seq('\\', /./)
      )),
      '"'
    ),

    number: $ => /-?[0-9]+(\.[0-9]+)?/,

    inline_expression: $ => prec.right(4, seq(
      token(prec(5, '{{')),
      optional(/[ \t]*/),
      field('content', prec.right(3, $.expression)),
      optional(/[ \t]*/),
      token(prec(5, '}}'))
    )),

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
      field('content', $._list_item_content) 
    )),

    // Content that can appear in a list item
    _list_item_content: $ => choice(
      $.paragraph,
      $.list,
      $.fenced_code_block,
      $.html_block
    ),

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

    // One or more lines of text ending at block starters or blank lines
    paragraph: $ => prec.left(1, choice(
      seq(
        $._inline_first,
        repeat($._inline_content)
      ),
      seq(
        $._inline_content,
        repeat($._inline_content),
        repeat(seq(
          /\n[ \t]*/,  // Single line break with optional whitespace
          choice(
            seq($._inline_first, repeat($._inline_content)),
            $._inline_content
          )
        ))
      )
    )),

    _paragraph_line: $ => prec.right(seq(
      repeat1(seq(
        optional(/[ \t]*/),
        $._inline_content
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

    text: $ => token(prec(-2, /[^\n{*_`!\[<\r]+/)),
  }
});
