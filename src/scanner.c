#include "tree_sitter/parser.h"
#include <stdlib.h>
#include <wctype.h>
#include <stdbool.h>
#include <string.h>

enum TokenType {
  CODE_CONTENT,
  FRONTMATTER_DELIM,
  LIST_CONTINUATION,
  THEMATIC_BREAK,
  LIST_MARKER_MINUS,
  LIST_MARKER_PLUS,
  LIST_MARKER_STAR,
  LIST_MARKER_DOT,
  LIST_MARKER_PARENTHESIS,
  LIST_MARKER_MINUS_DONT_INTERRUPT,
  LIST_MARKER_PLUS_DONT_INTERRUPT,
  LIST_MARKER_STAR_DONT_INTERRUPT,
  LIST_MARKER_DOT_DONT_INTERRUPT,
  LIST_MARKER_PARENTHESIS_DONT_INTERRUPT,
  HTML_COMMENT,
  HTML_BLOCK
};

typedef struct {
  bool in_list;
  bool at_start;
} Scanner;

void *tree_sitter_markdoc_external_scanner_create() {
  Scanner *scanner = (Scanner *)malloc(sizeof(Scanner));
  scanner->in_list = false;
  scanner->at_start = true;
  return scanner;
}

void tree_sitter_markdoc_external_scanner_destroy(void *payload) {
  free(payload);
}

unsigned tree_sitter_markdoc_external_scanner_serialize(void *payload, char *buffer) {
  Scanner *s = (Scanner *)payload;
  unsigned i = 0;
  if (i < 255) {
    buffer[i++] = (char)(s->in_list ? 1 : 0);
  }
  if (i < 255) {
    buffer[i++] = (char)(s->at_start ? 1 : 0);
  }
  return i;
}

void tree_sitter_markdoc_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  Scanner *s = (Scanner *)payload;
  s->in_list = false;
  s->at_start = true;

  unsigned i = 0;
  if (i < length) {
    s->in_list = buffer[i++] != 0;
  }
  if (i < length) {
    s->at_start = buffer[i++] != 0;
  }
}

static inline bool is_newline(int32_t ch) {
  return ch == '\n' || ch == '\r';
}

static inline bool is_space_ch(int32_t c) {
  return c == ' ' || c == '\t';
}

static inline bool is_digit_ch(int32_t c) {
  return c >= '0' && c <= '9';
}

 

static bool scan_literal(TSLexer *lexer, const char *text);
static bool is_thematic_break_line(TSLexer *lexer);
static bool scan_thematic_break_line(TSLexer *lexer);
static bool scan_frontmatter_delimiter(TSLexer *lexer);

static bool is_frontmatter_delimiter_line_no_column(TSLexer *lexer) {
  if (lexer->lookahead != '-') {
    return false;
  }

  TSLexer saved_state = *lexer;
  int count = 0;

  while (lexer->lookahead == '-' && count < 3) {
    lexer->advance(lexer, false);
    count++;
  }

  if (count != 3) {
    *lexer = saved_state;
    return false;
  }

  while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
    lexer->advance(lexer, false);
  }

  bool ok = is_newline(lexer->lookahead) || lexer->lookahead == 0;
  *lexer = saved_state;
  return ok;
}

static bool is_frontmatter_delimiter_line(TSLexer *lexer) {
  if (lexer->get_column(lexer) != 0) {
    return false;
  }

  return is_frontmatter_delimiter_line_no_column(lexer);
}

static bool scan_frontmatter_delimiter(TSLexer *lexer) {
  if (!is_frontmatter_delimiter_line_no_column(lexer)) {
    return false;
  }

  int count = 0;
  while (lexer->lookahead == '-' && count < 3) {
    lexer->advance(lexer, false);
    count++;
  }

  while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
    lexer->advance(lexer, false);
  }

  lexer->mark_end(lexer);
  return count == 3;
}

static bool scan_literal(TSLexer *lexer, const char *text) {
  for (const char *p = text; *p != '\0'; p++) {
    if (lexer->lookahead != *p) {
      return false;
    }
    lexer->advance(lexer, false);
  }
  return true;
}

static bool scan_thematic_break_line(TSLexer *lexer) {
  if (!is_thematic_break_line(lexer)) {
    return false;
  }

  while (lexer->lookahead != 0 && !is_newline(lexer->lookahead)) {
    lexer->advance(lexer, false);
  }
  lexer->mark_end(lexer);
  return true;
}

static bool scan_brace_close(TSLexer *lexer) {
  if (lexer->lookahead == '%') {
    TSLexer close_state = *lexer;
    lexer->advance(lexer, false);
    if (lexer->lookahead == '}') {
      lexer->advance(lexer, false);
      return true;
    }
    *lexer = close_state;
  }

  if (lexer->lookahead == '/') {
    TSLexer close_state = *lexer;
    lexer->advance(lexer, false);
    if (lexer->lookahead == '%') {
      lexer->advance(lexer, false);
      if (lexer->lookahead == '}') {
        lexer->advance(lexer, false);
        return true;
      }
    }
    *lexer = close_state;
  }

  return false;
}

static bool scan_html_comment(TSLexer *lexer) {
  TSLexer saved_state = *lexer;

  if (!scan_literal(lexer, "<!--")) {
    *lexer = saved_state;
    return false;
  }

  while (lexer->lookahead != 0) {
    if (lexer->lookahead == '-') {
      TSLexer close_state = *lexer;
      if (scan_literal(lexer, "-->")) {
        lexer->mark_end(lexer);
        return true;
      }
      *lexer = close_state;
    }
    lexer->advance(lexer, false);
  }

  *lexer = saved_state;
  return false;
}

static inline bool is_html_tag_start(int32_t c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static inline bool is_html_tag_char(int32_t c) {
  return is_html_tag_start(c) || is_digit_ch(c) || c == '-' || c == ':';
}

static bool scan_html_tag_name(TSLexer *lexer, char *buffer, size_t buffer_len, size_t *name_len) {
  if (!is_html_tag_start(lexer->lookahead)) {
    return false;
  }

  size_t i = 0;
  while (is_html_tag_char(lexer->lookahead)) {
    if (i + 1 >= buffer_len) {
      return false;
    }
    buffer[i++] = (char)lexer->lookahead;
    lexer->advance(lexer, false);
  }
  buffer[i] = '\0';
  *name_len = i;
  return i > 0;
}

static bool scan_html_block(TSLexer *lexer) {
  if (lexer->lookahead != '<') {
    return false;
  }

  TSLexer saved_state = *lexer;

  if (!scan_literal(lexer, "<")) {
    *lexer = saved_state;
    return false;
  }

  char tag_name[64];
  size_t tag_len = 0;
  if (!scan_html_tag_name(lexer, tag_name, sizeof(tag_name), &tag_len)) {
    *lexer = saved_state;
    return false;
  }

  bool self_closing = false;
  bool saw_close = false;
  while (lexer->lookahead != 0) {
    if (lexer->lookahead == '"' || lexer->lookahead == '\'') {
      int32_t quote = lexer->lookahead;
      lexer->advance(lexer, false);
      while (lexer->lookahead != 0 && lexer->lookahead != quote) {
        lexer->advance(lexer, false);
      }
      if (lexer->lookahead == quote) {
        lexer->advance(lexer, false);
      }
      continue;
    }

    if (lexer->lookahead == '/') {
      TSLexer slash_state = *lexer;
      lexer->advance(lexer, false);
      if (lexer->lookahead == '>') {
        lexer->advance(lexer, false);
        lexer->mark_end(lexer);
        self_closing = true;
        saw_close = true;
        break;
      }
      *lexer = slash_state;
    }

    if (lexer->lookahead == '>') {
      lexer->advance(lexer, false);
      lexer->mark_end(lexer);
      saw_close = true;
      break;
    }

    lexer->advance(lexer, false);
  }

  if (!saw_close) {
    *lexer = saved_state;
    return false;
  }

  if (self_closing) {
    return true;
  }

  while (lexer->lookahead != 0) {
    if (lexer->lookahead == '<') {
      TSLexer close_state = *lexer;
      lexer->advance(lexer, false);
      if (lexer->lookahead == '/') {
        lexer->advance(lexer, false);
        size_t match_len = 0;
        char close_name[64];
        if (scan_html_tag_name(lexer, close_name, sizeof(close_name), &match_len) &&
            match_len == tag_len &&
            strncmp(close_name, tag_name, tag_len) == 0) {
          while (is_space_ch(lexer->lookahead)) {
            lexer->advance(lexer, false);
          }
          if (lexer->lookahead == '>') {
            lexer->advance(lexer, false);
            lexer->mark_end(lexer);
            return true;
          }
        }
      }
      *lexer = close_state;
    }

    if (is_newline(lexer->lookahead)) {
      TSLexer newline_state = *lexer;
      if (lexer->lookahead == '\r') {
        lexer->advance(lexer, false);
        if (lexer->lookahead == '\n') {
          lexer->advance(lexer, false);
        }
      } else {
        lexer->advance(lexer, false);
      }
      if (lexer->lookahead == 0 || is_newline(lexer->lookahead)) {
        lexer->mark_end(lexer);
        return true;
      }
      *lexer = newline_state;
    }

    lexer->advance(lexer, false);
  }

  lexer->mark_end(lexer);
  return true;
}

static bool is_thematic_break_line(TSLexer *lexer) {
  if (lexer->get_column(lexer) != 0) {
    return false;
  }

  TSLexer saved_state = *lexer;

  unsigned indent = 0;
  while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
    lexer->advance(lexer, false);
    indent++;
  }

  if (indent >= 4) {
    *lexer = saved_state;
    return false;
  }

  int32_t marker = lexer->lookahead;
  if (marker != '*' && marker != '-' && marker != '_') {
    *lexer = saved_state;
    return false;
  }

  unsigned marker_count = 0;
  while (lexer->lookahead == marker || lexer->lookahead == ' ' || lexer->lookahead == '\t') {
    if (lexer->lookahead == marker) {
      marker_count++;
    }
    lexer->advance(lexer, false);
  }

  while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
    lexer->advance(lexer, false);
  }

  bool ok = marker_count >= 3 && (lexer->lookahead == 0 || is_newline(lexer->lookahead));
  *lexer = saved_state;
  return ok;
}

static bool scan_indentation(TSLexer *lexer, uint8_t *indentation) {
  while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
    lexer->advance(lexer, false);
    (*indentation)++;
  }
  return *indentation <= 3;
}

static bool is_simple_list_marker_start(TSLexer *lexer) {
  TSLexer saved_state = *lexer;
  uint8_t indentation = 0;
  if (!scan_indentation(lexer, &indentation)) {
    *lexer = saved_state;
    return false;
  }

  int32_t first = lexer->lookahead;
  if (first == '*' || first == '+' || first == '-') {
    if (indentation != 0) {
      *lexer = saved_state;
      return false;
    }
    lexer->advance(lexer, false);
    bool ok = lexer->lookahead == ' ' || lexer->lookahead == '\t';
    *lexer = saved_state;
    return ok;
  }

  if (iswdigit(first)) {
    size_t digits = 0;
    while (iswdigit(lexer->lookahead) && digits < 9) {
      digits++;
      lexer->advance(lexer, false);
    }
    if (digits == 0) {
      *lexer = saved_state;
      return false;
    }
    if (lexer->lookahead == '.' || lexer->lookahead == ')') {
      lexer->advance(lexer, false);
      bool ok = lexer->lookahead == ' ' || lexer->lookahead == '\t';
      *lexer = saved_state;
      return ok;
    }
  }

  *lexer = saved_state;
  return false;
}

static bool scan_simple_list_marker(TSLexer *lexer, const bool *valid_symbols) {
  TSLexer saved_state = *lexer;
  uint8_t indentation = 0;

  if (!scan_indentation(lexer, &indentation)) {
    *lexer = saved_state;
    return false;
  }

  int32_t first = lexer->lookahead;
  if (first == '*' || first == '+' || first == '-') {
    if (indentation != 0) {
      *lexer = saved_state;
      return false;
    }
    enum TokenType symbol =
        first == '*'
            ? LIST_MARKER_STAR
            : (first == '+' ? LIST_MARKER_PLUS : LIST_MARKER_MINUS);
    enum TokenType dont_interrupt_symbol =
        first == '*'
            ? LIST_MARKER_STAR_DONT_INTERRUPT
            : (first == '+'
                   ? LIST_MARKER_PLUS_DONT_INTERRUPT
                   : LIST_MARKER_MINUS_DONT_INTERRUPT);
    bool symbol_valid = valid_symbols[symbol];
    bool dont_interrupt_valid = valid_symbols[dont_interrupt_symbol];
    if (!symbol_valid && !dont_interrupt_valid && lexer->get_column(lexer) != 0) {
      *lexer = saved_state;
      return false;
    }
    lexer->advance(lexer, false);
    if (lexer->lookahead != ' ' && lexer->lookahead != '\t') {
      *lexer = saved_state;
      return false;
    }
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
      lexer->advance(lexer, false);
    }
    lexer->mark_end(lexer);
    lexer->result_symbol = symbol_valid ? symbol
                                        : (dont_interrupt_valid ? dont_interrupt_symbol : symbol);
    return true;
  }

  if (iswdigit(first)) {
    size_t digits = 0;
    while (iswdigit(lexer->lookahead) && digits < 9) {
      digits++;
      lexer->advance(lexer, false);
    }
    if (digits == 0) {
      *lexer = saved_state;
      return false;
    }
    bool dot = false;
    if (lexer->lookahead == '.') {
      dot = true;
      lexer->advance(lexer, false);
    } else if (lexer->lookahead == ')') {
      lexer->advance(lexer, false);
    } else {
      *lexer = saved_state;
      return false;
    }
    enum TokenType symbol = dot ? LIST_MARKER_DOT : LIST_MARKER_PARENTHESIS;
    enum TokenType dont_interrupt_symbol =
        dot ? LIST_MARKER_DOT_DONT_INTERRUPT : LIST_MARKER_PARENTHESIS_DONT_INTERRUPT;
    if (!valid_symbols[symbol] && !valid_symbols[dont_interrupt_symbol]) {
      *lexer = saved_state;
      return false;
    }
    if (lexer->lookahead != ' ' && lexer->lookahead != '\t') {
      *lexer = saved_state;
      return false;
    }
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
      lexer->advance(lexer, false);
    }
    lexer->mark_end(lexer);
    lexer->result_symbol = valid_symbols[symbol] ? symbol : dont_interrupt_symbol;
    return true;
  }

  *lexer = saved_state;
  return false;
}

bool tree_sitter_markdoc_external_scanner_scan(void *payload, TSLexer *lexer,
                                              const bool *valid_symbols) {
  Scanner *s = (Scanner *)payload;

  if (lexer->get_column(lexer) == 0) {
    int32_t marker = lexer->lookahead;
    bool wants_unordered = valid_symbols[LIST_MARKER_MINUS] ||
                           valid_symbols[LIST_MARKER_PLUS] ||
                           valid_symbols[LIST_MARKER_STAR] ||
                           valid_symbols[LIST_MARKER_MINUS_DONT_INTERRUPT] ||
                           valid_symbols[LIST_MARKER_PLUS_DONT_INTERRUPT] ||
                           valid_symbols[LIST_MARKER_STAR_DONT_INTERRUPT];

    if (wants_unordered && (marker == '-' || marker == '+' || marker == '*')) {
      TSLexer saved_state = *lexer;
      lexer->advance(lexer, false);
      if (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
        while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
          lexer->advance(lexer, false);
        }
        lexer->mark_end(lexer);
        if (marker == '-') {
          lexer->result_symbol =
              valid_symbols[LIST_MARKER_MINUS] ? LIST_MARKER_MINUS : LIST_MARKER_MINUS_DONT_INTERRUPT;
        } else if (marker == '+') {
          lexer->result_symbol =
              valid_symbols[LIST_MARKER_PLUS] ? LIST_MARKER_PLUS : LIST_MARKER_PLUS_DONT_INTERRUPT;
        } else {
          lexer->result_symbol =
              valid_symbols[LIST_MARKER_STAR] ? LIST_MARKER_STAR : LIST_MARKER_STAR_DONT_INTERRUPT;
        }
        s->in_list = true;
        s->at_start = false;
        return true;
      }
      *lexer = saved_state;
    }
  }

  if (lexer->get_column(lexer) == 0) {
    if (valid_symbols[FRONTMATTER_DELIM] && scan_frontmatter_delimiter(lexer)) {
      lexer->result_symbol = FRONTMATTER_DELIM;
      s->in_list = false;
      s->at_start = false;
      return true;
    }

    if (valid_symbols[THEMATIC_BREAK] && scan_thematic_break_line(lexer)) {
      lexer->result_symbol = THEMATIC_BREAK;
      s->in_list = false;
      s->at_start = false;
      return true;
    }
  }

  if (lexer->get_column(lexer) == 0 && scan_simple_list_marker(lexer, valid_symbols)) {
    if (lexer->result_symbol == LIST_MARKER_STAR || lexer->result_symbol == LIST_MARKER_PLUS ||
        lexer->result_symbol == LIST_MARKER_MINUS || lexer->result_symbol == LIST_MARKER_DOT ||
        lexer->result_symbol == LIST_MARKER_PARENTHESIS) {
      s->in_list = true;
    }
    s->at_start = false;
    return true;
  }

  // CODE_CONTENT: consume everything until we see a code fence close pattern
  // The grammar will handle detecting code_fence_open and code_fence_close
  if (valid_symbols[CODE_CONTENT] && is_newline(lexer->lookahead)) {
    // Only match CODE_CONTENT if we are immediately after the fence line newline
    TSLexer start_state = *lexer;
    lexer->mark_end(lexer);
    bool has_content = false;

    // Skip the newline after the opening fence
    if (lexer->lookahead == '\n') {
      lexer->advance(lexer, false);
    }

    // Check if the very next thing is a closing fence (empty block)
    if (lexer->lookahead == '`' || lexer->lookahead == '~') {
      TSLexer fence_state = *lexer;
      char fence_char = lexer->lookahead;
      int count = 0;
      while (lexer->lookahead == fence_char && count < 5) {
        count++;
        lexer->advance(lexer, false);
      }
      if (count >= 3) {
        *lexer = start_state;
        return false;
      }
      *lexer = fence_state;
    }

    // Consume content until we find ``` or ~~~ at start of line
    bool at_line_start = true;
    while (lexer->lookahead != 0) {
      // Check for fence close pattern at start of line
      if (at_line_start && (lexer->lookahead == '`' || lexer->lookahead == '~')) {
        // Peek ahead to see if this is a fence (3+ chars)
        char fence_char = lexer->lookahead;
        int32_t next = lexer->lookahead;
        int count = 0;

        // Save position
        TSLexer saved_state = *lexer;

        // Count fence chars
        while (next == fence_char && count < 5) {  // limit lookahead
          count++;
          lexer->advance(lexer, false);
          next = lexer->lookahead;
        }

        if (count >= 3) {
          // This looks like a closing fence, stop consuming content
          *lexer = saved_state;  // restore position
          break;
        }

        // Not a fence, restore and continue
        *lexer = saved_state;
      }

      // Track line starts
      if (lexer->lookahead == '\n') {
        at_line_start = true;
      } else if (lexer->lookahead != ' ' && lexer->lookahead != '\t') {
        at_line_start = false;
      }

      has_content = true;
      lexer->advance(lexer, false);
      lexer->mark_end(lexer);
    }

    if (has_content) {
      lexer->result_symbol = CODE_CONTENT;
      s->at_start = false;
      return true;
    }

    *lexer = start_state;
    return false;
  }

  // LIST_CONTINUATION: newline + indentation inside a list item
  if (valid_symbols[LIST_CONTINUATION] && s->in_list) {
    TSLexer saved_state = *lexer;
    bool at_line_start = lexer->get_column(lexer) == 0;
    bool starts_with_indent = lexer->lookahead == ' ' || lexer->lookahead == '\t';

    if (is_newline(lexer->lookahead)) {
      if (lexer->lookahead == '\r') {
        lexer->advance(lexer, false);
        if (lexer->lookahead == '\n') {
          lexer->advance(lexer, false);
        }
      } else {
        lexer->advance(lexer, false);
      }
    } else if (!(at_line_start && starts_with_indent)) {
      *lexer = saved_state;
      return false;
    }

    uint8_t indentation = 0;
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
      lexer->advance(lexer, false);
      indentation++;
    }

    if (indentation == 0 || lexer->lookahead == 0 || is_newline(lexer->lookahead)) {
      *lexer = saved_state;
      return false;
    }

    if (indentation <= 3 && iswdigit(lexer->lookahead)) {
      TSLexer marker_state = *lexer;
      size_t digits = 0;
      while (iswdigit(lexer->lookahead) && digits < 9) {
        digits++;
        lexer->advance(lexer, false);
      }
      if (digits > 0 && (lexer->lookahead == '.' || lexer->lookahead == ')')) {
        lexer->advance(lexer, false);
        if (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
          *lexer = saved_state;
          return false;
        }
      }
      *lexer = marker_state;
    }

    lexer->mark_end(lexer);
    lexer->result_symbol = LIST_CONTINUATION;
    s->at_start = false;
    return true;
  }

  if (valid_symbols[THEMATIC_BREAK] && is_thematic_break_line(lexer)) {
    while (lexer->lookahead != 0 && !is_newline(lexer->lookahead)) {
      lexer->advance(lexer, false);
    }
    lexer->mark_end(lexer);
    lexer->result_symbol = THEMATIC_BREAK;
    s->at_start = false;
    return true;
  }

  if (valid_symbols[HTML_COMMENT] && scan_html_comment(lexer)) {
    lexer->result_symbol = HTML_COMMENT;
    s->at_start = false;
    return true;
  }

  if (valid_symbols[HTML_BLOCK] && scan_html_block(lexer)) {
    lexer->result_symbol = HTML_BLOCK;
    s->at_start = false;
    return true;
  }
  return false;
}
