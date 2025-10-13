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
      $.member_expression,
      $.identifier,
      $.number,
      $.string
    ),

    member_expression: $ => prec.left(2, seq(
      $.identifier,
      repeat1(seq('.', $.identifier))
    )),

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

    paragraph: $ => prec.left(seq(
      choice($.inline_expression, $.text),
      repeat(seq(/\n/, choice($.inline_expression, $.text)))
    )),

    text: $ => token(prec(-1, /[^\n{]+/)),
  }
});
