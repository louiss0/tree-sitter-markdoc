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
    /\s/
  ],

  rules: {
    source_file: $ => seq(
      optional($.frontmatter),
      repeat($._block)
    ),

    frontmatter: $ => seq(
      token(prec(1, '---')),
      token(prec(1, /\r?\n/)),
      alias($.yaml_content, $.yaml),
      token(prec(1, '---')),
      optional(token(prec(1, /\r?\n/)))
    ),

    yaml_content: $ => repeat1(
      /[^\r\n-][^\r\n]*\r?\n|\r?\n/
    ),

    _block: $ => choice(
      $.paragraph
    ),

    paragraph: $ => prec.right(seq(
      $.text,
      repeat(seq(/\r?\n/, $.text)),
      optional(/\r?\n/)
    )),

    text: $ => /[^\r\n]+/
  }
});
