/**
 * @file This is a tree sitter parser for markdoc
 * @author Shelton Louis <louisshelton0@gmail.com>
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

module.exports = grammar({
  name: "markdoc",

  extras: $ => [
    /[ \t\r]/
  ],

  conflicts: $ => [
    [$.source_file],
    [$.attribute, $.expression]
  ],

  rules: {
    source_file: $ => prec.right(seq(
      repeat(/\n/),
      optional(seq(
        $._block,
        repeat(seq(
          repeat1(/\n/),
          $._block
        ))
      )),
      repeat(/\n/)
    )),

    _block: $ => choice(
      $.frontmatter,
      $.heading,
      $.fenced_code_block,
      prec(1, $.markdoc_tag),
      $.list,
      $.html_comment,
      $.html_block,
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

    code: $ => repeat1(/[^\n]+\n/),

    code_fence_close: $ => seq(
      choice(
        token(prec(3, '```')),
        token(prec(3, '~~~'))
      )
    ),

    markdoc_tag: $ => choice(
      seq(
        $.tag_open,
        repeat($._block),
        $.tag_close
      ),
      $.tag_self_close
    ),

    tag_open: $ => prec.right(seq(
      '{%',
      /[ \t]*/,
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.tag_name),
      repeat(seq(
        /[ \t]+/,
        $.attribute
      )),
      /[ \t]*/,
      '%}',
      optional(/\n/)
    )),

    tag_close: $ => prec.right(seq(
      '{%',
      /[ \t]*/,
      '/',
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.tag_name),
      /[ \t]*/,
      '%}',
      optional(/\n/)
    )),

    tag_self_close: $ => prec.right(seq(
      '{%',
      /[ \t]*/,
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.tag_name),
      repeat(seq(
        /[ \t]+/,
        $.attribute
      )),
      /[ \t]*/,
      '/%}',
      optional(/\n/)
    )),

    attribute: $ => seq(
      alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.attribute_name),
      '=',
      field('attribute_value', choice(
        $.string,
        $.expression
      ))
    ),

    expression: $ => choice(
      $.call_expression,
      $.member_expression,
      $.array_access,
      $.array_literal,
      $.object_literal,
      $.identifier,
      $.number,
      $.string
    ),

    call_expression: $ => prec.left(3, seq(
      choice(
        $.member_expression,
        $.array_access,
        $.identifier
      ),
      $.arguments
    )),

    arguments: $ => seq(
      '(',
      optional(seq(
        $.expression,
        repeat(seq(',', /[ \t]*/, $.expression))
      )),
      ')'
    ),

    member_expression: $ => prec.left(2, seq(
      choice(
        $.array_access,
        $.identifier
      ),
      repeat1(seq('.', $.identifier))
    )),

    array_access: $ => prec.left(2, seq(
      $.identifier,
      '[',
      $.expression,
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

    inline_expression: $ => seq(
      '{{',
      /[ \t]*/,
      $.expression,
      /[ \t]*/,
      '}}'
    ),

    list: $ => prec.right(repeat1($.list_item)),

    list_item: $ => prec.right(seq(
      field('marker', $.list_marker),
      field('content', seq(
        $.paragraph,
        optional(seq(
          /\n/,
          repeat(seq(
            /[ \t]{2,}/,
            choice($.list, $.paragraph),
            optional(/\n/)
          ))
        ))
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

    paragraph: $ => prec.left(repeat1(choice(
      $.inline_expression,
      $.strong,
      $.emphasis,
      $.inline_code,
      $.link,
      $.image,
      $.html_inline,
      $.text
    ))),

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

    text: $ => token(prec(-1, /[^\n{*_`!\[<]+/)),
  }
});
