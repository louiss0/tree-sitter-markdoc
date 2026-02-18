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

module.exports = grammar({
  name: "markdoc",

  externals: ($) => [
    $._CODE_FENCE_OPEN,
    $._CODE_FENCE_CLOSE,
    $._CODE_CONTENT,
    $._FRONTMATTER_DELIM,
    $._LIST_CONTINUATION,
    $._UNORDERED_LIST_MARKER,
    $._ORDERED_LIST_MARKER,
    $._INDENTED_UNORDERED_LIST_MARKER,
    $._INDENTED_ORDERED_LIST_MARKER,
    $._SOFT_LINE_BREAK,
    $._THEMATIC_BREAK,
    $._HTML_COMMENT,
    $._HTML_BLOCK,
  ],

  extras: ($) => [],

  conflicts: ($) => [[$.source_file], [$.markdoc_tag], [$.markdoc_tag, $._inline_line_start]],

  inline: ($) => [$.line_break],

  rules: {
    source_file: ($) =>
      choice(
        prec(
          1,
          seq(
            $.frontmatter,
            repeat(choice($._BLANK_LINE, $._NEWLINE)),
            optional(seq($._block, repeat(seq(choice($._BLANK_LINE, $._NEWLINE), $._block)))),
            repeat(choice($._BLANK_LINE, $._NEWLINE)),
          ),
        ),
        prec.right(
          seq(
            repeat($._NEWLINE),
            optional(seq($._block, repeat(seq(choice($._BLANK_LINE, $._NEWLINE), $._block)))),
            repeat(choice($._BLANK_LINE, $._NEWLINE)),
          ),
        ),
      ),

    _block: ($) =>
      choice(
        $.comment_block, // Must come before markdoc_tag to match {% comment %}
        $.markdoc_tag,
        $.fenced_code_block,
        $.heading,
        $.thematic_break,
        $.blockquote,
        $.html_block,
        $.unordered_list,
        $.ordered_list,
        $.html_comment,
        $.paragraph,
      ),

    frontmatter: ($) =>
      seq(
        $._FRONTMATTER_DELIM,
        $._NEWLINE,
        alias($.yaml_content, $.yaml),
        $._FRONTMATTER_DELIM,
      ),

    yaml_content: ($) => repeat1(seq(/[^\n]+/, $._NEWLINE)),

    heading: ($) =>
      prec.right(
        2,
        seq(
          field("heading_marker", $.heading_marker),
          field("heading_text", optional($.heading_text)),
        ),
      ),

    heading_marker: ($) => token(prec(3, /#{1,6}[ \t]/)), // Require space/tab after #
    heading_text: ($) => token(prec(1, /[^\r\n]+/)),

    // Thematic break (horizontal rule)
    thematic_break: ($) => prec.dynamic(1, $._THEMATIC_BREAK),

    // Blockquote (consume consecutive > lines as a single block)
    blockquote: ($) => token(prec(2, />[^\r\n]*(\r?\n>[^\r\n]*)*\r?\n?/)),

    fenced_code_block: ($) =>
      seq(
        field("open", $.code_fence_open),
        $._NEWLINE,
        optional(field("code", $.code)),
        field("close", $.code_fence_close),
      ),

    code_fence_open: ($) => seq($._CODE_FENCE_OPEN, optional($.info_string)),

    info_string: ($) =>
      seq(
        alias(/[a-zA-Z0-9_+-]+/, $.language),
        optional(WS),
        optional(alias(/\{[^}\n]*\}/, $.attributes)),
      ),

    code: ($) => repeat1($._code_item),

    _code_item: ($) => choice($.fenced_code_block, $._CODE_CONTENT),

    code_fence_close: ($) => $._CODE_FENCE_CLOSE,

    // Markdoc comment block: {% comment %}...{% /comment %}
    comment_block: ($) => token(prec(6, /\{%\s*comment\s*%\}(.|\r|\n)*\{%\s*\/comment\s*%\}/)),

    // Block-level Markdoc tag ({% tag %}...{% /tag %} or {% tag /%})
    markdoc_tag: ($) =>
      prec.dynamic(
        4,
        choice(
          // Self-closing tag on its own line
          seq($.tag_self_close, $.line_break),
          // Full tag with content on following lines
          seq(
            $.tag_open,
            choice(
              // Empty tag
              seq(repeat(choice($._NEWLINE, $._BLANK_LINE)), $.tag_close),
              // Tag with content
              seq(
                repeat(choice($._NEWLINE, $._BLANK_LINE)),
                $._block,
                repeat(choice(seq($._BLANK_LINE, $._block), seq($._NEWLINE, $._block))),
                repeat1(choice($._NEWLINE, $._BLANK_LINE)),
                $.tag_close,
              ),
            ),
          ),
        ),
      ),

    tag_open: ($) =>
      prec.right(
        seq(
          $.tag_open_delimiter,
          optional(WS),
          alias($.identifier, $.tag_name),
          repeat(seq(WS1, $.attribute)),
          $.tag_block_close,
        ),
      ),

    tag_close: ($) =>
      prec.right(
        seq(
          $.tag_open_delimiter,
          optional(WS),
          seq(token("/"), optional(WS)),
          alias($.identifier, $.tag_name),
          $.inline_expression_close,
        ),
      ),

    // Inline self-closing tag for use in paragraphs/lists
    inline_tag: ($) => prec(1, choice($.tag_self_close)),

    tag_self_close: ($) =>
      prec.right(
        seq(
          $.tag_open_delimiter,
          optional(WS),
          alias($.identifier, $.tag_name),
          repeat(seq(WS1, $.attribute)),
          $.tag_self_close_delimiter,
        ),
      ),

    tag_open_delimiter: ($) => token(prec(6, "{%")),
    tag_block_close: ($) => token(prec(6, /[ \t]*%}\r?\n/)),
    inline_expression_close: ($) => token(prec(5, /[ \t]*%}/)),
    tag_self_close_delimiter: ($) => token(prec(6, /[ \t]*\/%}/)),

    attribute: ($) =>
      seq(alias(/[a-zA-Z_][a-zA-Z0-9_-]*/, $.attribute_name), "=", $.attribute_value),

    attribute_value: ($) => $.value_expression,

    value_expression: ($) => choice($.variable_value, $.call_expression, $.json_value),

    json_value: ($) =>
      choice($.string, $.number, $.boolean, $.null, $.array_literal, $.object_literal),

    // Variables prefixed with $ or @
    variable: ($) => seq("$", $.identifier),
    special_variable: ($) => seq("@", $.identifier),

    variable_reference: ($) => seq($.variable, repeat(seq(".", $.identifier))),

    special_variable_reference: ($) => seq($.special_variable, repeat(seq(".", $.identifier))),

    array_subscript: ($) =>
      seq("[", optional(WS), choice($.number, $.string), optional(WS), "]"),

    subscript_reference: ($) =>
      seq(
        choice($.variable_reference, $.special_variable_reference),
        repeat1($.array_subscript),
      ),

    variable_value: ($) =>
      choice($.variable_reference, $.special_variable_reference, $.subscript_reference),

    call_expression: ($) =>
      prec.left(
        seq(
          field("function", $.identifier),
          "(",
          optional(WS),
          optional(
            seq(
              $.value_expression,
              repeat(seq(optional(WS), ",", optional(WS), $.value_expression)),
            ),
          ),
          optional(WS),
          ")",
        ),
      ),

    // JSON-like literals
    boolean: ($) => choice("true", "false"),

    null: ($) => "null",

    array_literal: ($) =>
      prec.right(
        seq(
          "[",
          optional(WS),
          optional(
            seq(
              $.json_value,
              repeat(seq(optional(WS), ",", optional(WS), $.json_value)),
              optional(seq(optional(WS), ",")),
            ),
          ),
          optional(WS),
          "]",
        ),
      ),

    object_literal: ($) =>
      prec.right(
        seq(
          "{",
          optional(WS),
          optional(
            seq(
              $.pair,
              repeat(seq(optional(WS), ",", optional(WS), $.pair)),
              optional(seq(optional(WS), ",")),
            ),
          ),
          optional(WS),
          "}",
        ),
      ),

    pair: ($) =>
      seq(
        field("key", choice($.identifier, $.string)),
        optional(WS),
        ":",
        optional(WS),
        field("value", $.json_value),
      ),

    identifier: ($) => /[a-zA-Z_][a-zA-Z0-9_]*/,

    string: ($) =>
      choice(
        seq('"', repeat(choice(/[^"\\\n]/, seq("\\", /./))), '"'),
        seq("'", repeat(choice(/[^'\\\n]/, seq("\\", /./))), "'"),
      ),

    number: ($) => /-?[0-9]+(\.[0-9]+)?/,

    inline_expression: ($) =>
      seq(
        $.tag_open_delimiter,
        optional(WS),
        field("content", choice($.variable_value, $.call_expression)),
        $.inline_expression_close,
      ),

    // Lists
    unordered_list: ($) => prec.right(field("items", repeat1($.unordered_list_item))),
    unordered_list_item: ($) =>
      prec.right(
        1,
        seq(
          field("marker", alias($._UNORDERED_LIST_MARKER, $.unordered_list_marker)),
          field("content", $.list_paragraph),
          $._NEWLINE,
          repeat(
            choice(
              $.list_item_continuation,
              alias($._nested_unordered_list, $.unordered_list),
              alias($._nested_ordered_list, $.ordered_list),
            ),
          ),
        ),
      ),

    ordered_list: ($) => prec.right(field("items", repeat1($.ordered_list_item))),
    ordered_list_item: ($) =>
      prec.right(
        1,
        seq(
          field("marker", alias($._ORDERED_LIST_MARKER, $.ordered_list_marker)),
          field("content", $.list_paragraph),
          $._NEWLINE,
          repeat(
            choice(
              $.list_item_continuation,
              alias($._nested_unordered_list, $.unordered_list),
              alias($._nested_ordered_list, $.ordered_list),
            ),
          ),
        ),
      ),

    _nested_unordered_list: ($) =>
      prec.right(field("items", repeat1($._nested_unordered_list_item))),
    _nested_unordered_list_item: ($) =>
      prec.right(
        seq(
          field("marker", alias($._INDENTED_UNORDERED_LIST_MARKER, $.unordered_list_marker)),
          field("content", $.list_paragraph),
          $._NEWLINE,
          repeat(
            choice(
              $.list_item_continuation,
              alias($._nested_unordered_list, $.unordered_list),
              alias($._nested_ordered_list, $.ordered_list),
            ),
          ),
        ),
      ),

    _nested_ordered_list: ($) =>
      prec.right(field("items", repeat1($._nested_ordered_list_item))),
    _nested_ordered_list_item: ($) =>
      prec.right(
        seq(
          field("marker", alias($._INDENTED_ORDERED_LIST_MARKER, $.ordered_list_marker)),
          field("content", $.list_paragraph),
          $._NEWLINE,
          repeat(
            choice(
              $.list_item_continuation,
              alias($._nested_unordered_list, $.unordered_list),
              alias($._nested_ordered_list, $.ordered_list),
            ),
          ),
        ),
      ),

    list_item_continuation: ($) =>
      seq(
        $._LIST_CONTINUATION,
        field(
          "block",
          choice(
            $.paragraph,
            $.markdoc_tag,
            $.fenced_code_block,
            $.heading,
            $.thematic_break,
            $.blockquote,
            $.html_block,
            $.html_comment,
            $.unordered_list,
            $.ordered_list,
          ),
        ),
      ),

    line_break: ($) => choice($._BLANK_LINE, $._NEWLINE),

    html_comment: ($) => $._HTML_COMMENT,

    html_block: ($) => $._HTML_BLOCK,

    html_inline: ($) =>
      choice(
        // Opening and closing tag
        token(
          prec(
            1,
            seq(
              "<",
              HTML_TAG_NAME,
              optional(/[^\n>]*/),
              ">",
              optional(/[^\n<]*/),
              "</",
              HTML_TAG_NAME,
              ">",
            ),
          ),
        ),
        // Self-closing tag
        token(prec(1, seq("<", HTML_TAG_NAME, optional(/[^\n>]*/), "/>"))),
      ),

    // Paragraph: consecutive lines of content (separated by single newlines, not double)
    paragraph: ($) =>
      prec.left(
        -1,
        seq(
          $._inline_line_start,
          repeat($._inline_content),
          repeat(
            seq(
              $._SOFT_LINE_BREAK,
              choice(
                $._inline_expression_line,
                seq($._inline_line_start_no_expression, repeat($._inline_content)),
              ),
            ),
          ),
        ),
      ),

    // List paragraph: same as paragraph but semantically distinct for list items
    list_paragraph: ($) =>
      prec.right(
        1,
        seq(
          $._inline_line_start,
          repeat($._inline_content),
          repeat(
            seq(
              $._LIST_CONTINUATION,
              choice(
                $._inline_expression_line,
                seq($._inline_line_start_no_expression, repeat($._inline_content)),
              ),
            ),
          ),
        ),
      ),

    emphasis: ($) =>
      choice(
        token(prec(2, /\*[^\s*]\*/)),
        token(prec(2, /\*[^\s*][^*\n]*[^\s*]\*/)),
        token(prec(2, /_[^\s_]\_/)),
        token(prec(2, /_[^\s_][^_\n]*[^\s_]\_/)),
      ),

    strong: ($) =>
      choice(
        token(prec(3, /\*\*[^\s*]\*\*/)),
        token(prec(3, /\*\*[^\s*][^*\n]*[^\s*]\*\*/)),
        token(prec(3, /__[^\s_]\__/)),
        token(prec(3, /__[^\s_][^_\n]*[^\s_]\__/)),
      ),

    inline_code: ($) => seq("`", token(prec(1, /[^`\n]+/)), "`"),

    link: ($) =>
      seq(
        "[",
        alias(token(prec(1, /[^\]\n]+/)), $.link_text),
        "]",
        "(",
        alias(token(prec(1, /[^)\n]+/)), $.link_destination),
        ")",
      ),

    image: ($) =>
      seq(
        token("!["),
        alias(token(prec(1, /[^\]\n]+/)), $.image_alt),
        "]",
        "(",
        alias(token(prec(1, /[^)\n]+/)), $.image_destination),
        ")",
      ),

    // First inline element in a paragraph/line
    _inline_first: ($) =>
      choice(
        $.inline_expression,
        $.inline_tag,
        $.text,
        $.html_inline,
        $.link,
        $.emphasis,
        $.strong,
        $.inline_code,
      ),

    _inline_line_start: ($) =>
      choice(
        $.inline_expression,
        $.tag_self_close,
        $.text,
        $.html_inline,
        $.link,
        $.emphasis,
        $.strong,
        $.inline_code,
        $.image,
        alias($.standalone_punct, $.text),
      ),

    _inline_line_start_no_expression: ($) =>
      choice(
        $.tag_self_close,
        $.text,
        $.html_inline,
        $.link,
        $.emphasis,
        $.strong,
        $.inline_code,
        $.image,
        alias($.standalone_punct, $.text),
      ),

    _inline_expression_line: ($) => seq($.inline_expression, repeat1($._inline_content)),

    // Inline content after first element
    _inline_content: ($) => choice($._inline_first, $.image, alias($.standalone_punct, $.text)),

    text: ($) => token(prec(1, /[^\r\n{\[<!`*_]+/)),

    // Fallback for standalone punctuation that doesn't start special syntax
    standalone_punct: ($) => token(/[!_*]/),

    _NEWLINE: ($) => token(/\r?\n/),
    _BLANK_LINE: ($) => token(/\r?\n[ \t]*\r?\n+/),
  },
});
