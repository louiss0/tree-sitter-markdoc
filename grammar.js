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
    [$.source_file]
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

    paragraph: $ => prec.left(seq(
      $.text,
      repeat(seq(/\n/, $.text))
    )),

    text: $ => token(prec(-1, /[^\n]+/)),
  }
});
