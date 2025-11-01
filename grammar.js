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
    [$.attribute_value, $._primary_expression],
    [$.tag_open, $.tag_close],
    [$.binary_expression]
  ],

  rules: {
    source_file: $ => prec.right(optional(seq(
      choice(
        $.frontmatter,
        $._block
      ),
      repeat(seq(
        choice(
          $._BLANK_LINE
        ),
        choice(
          $.frontmatter,
          $._block
        )
      ))
    ))),

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
      alias($.yaml_content, $.yaml),
      token(prec(10, '---'))
    ),

    yaml_content: $ => repeat1(
      /[^-][^\n]*/ 
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

    markdoc_tag: $ => prec.dynamic(10, choice(
      seq(
        $.tag_open,
        optional(seq(
          optional($._NEWLINE),  // Optional newline after opening tag
          $._block,
          repeat(seq(
            optional($._BLANK_LINE),
            $._block
          ))
        )),
        optional($._NEWLINE),  // Optional newline before closing tag
        $.tag_close
      ),
      $.tag_self_close
    )),

    tag_open: $ => prec.right(seq(
      token(prec(6, '{%')),
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.tag_name),
      repeat($.attribute),
      token(prec(6, '%}'))
    )),

    tag_close: $ => prec.right(seq(
      token(prec(6, '{%')),
      optional(token('/')),
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.tag_name),
      token(prec(6, '%}'))
    )),

    tag_self_close: $ => prec.right(seq(
      token(prec(6, '{%')),
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.tag_name),
      repeat($.attribute),
      token(prec(6, '/%}'))
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

    // Arrow function: () => expr or (params) => expr
    arrow_function: $ => seq(
      '(',
      optional(seq(
        $.identifier,
        repeat(seq(',', $.identifier))
      )),
      ')',
      '=>',
      $.expression
    ),

    // Binary operators (in order of precedence)
    // Use token(prec()) for operators to give them priority over conflicting markdown tokens
    // Add spaces around operators for proper parsing
    binary_expression: $ => choice(
      prec.left(1, seq($.expression, token(prec(5, '||')), $.expression)),
      prec.left(2, seq($.expression, token(prec(5, '&&')), $.expression)),
      prec.left(3, seq($.expression, token(prec(5, choice('==', '!='))), $.expression)),
      prec.left(4, seq($.expression, token(prec(5, choice('<', '>', '<=', '>='))), $.expression)),
      prec.left(5, seq($.expression, token(prec(5, choice('+', '-'))), $.expression)),
      prec.left(6, seq($.expression, token(prec(5, choice('*', '/', '%'))), $.expression))
    ),

    // Unary operators
    unary_expression: $ => prec.right(7, seq(
      choice('!', '-', '+'),
      $.expression
    )),

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

    // A list item: marker + list_paragraph content
    list_item: $ => seq(
      field('marker', $.list_marker),
      field('content', $.list_paragraph)
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
      prec(5, $.inline_expression),
      prec(4, $.text),
      prec(3, alias($.standalone_punct, $.text)),
      prec(2, $.html_inline),
      prec(2, $.link),
      prec(1, $.emphasis),
      prec(1, $.strong),
      prec(2, $.inline_code)
    ),

    // Inline content after first element
    _inline_content: $ => choice(
      $._inline_first,
      $.image,
      alias($.standalone_punct, $.text)
    ),

    text: $ => token(/[^\n{<`*_\[\]!\->]+/),
    
    // Fallback for standalone punctuation that doesn't start special syntax
    standalone_punct: $ => token(choice('!', '-', '_', '*')),
  }
});
