#include "tree_sitter/parser.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

enum TokenType {
  CODE_CONTENT,
  FRONTMATTER_DELIM,
  LIST_CONTINUATION,
  UNORDERED_LIST_MARKER,
  ORDERED_LIST_MARKER,
  INDENTED_UNORDERED_LIST_MARKER,
  INDENTED_ORDERED_LIST_MARKER,
  SOFT_LINE_BREAK,
  THEMATIC_BREAK,
  HTML_COMMENT,
  HTML_BLOCK
};

typedef struct {
  bool at_start;
  bool in_frontmatter;
} Scanner;

void *tree_sitter_markdoc_external_scanner_create() {
  Scanner *scanner = (Scanner *)malloc(sizeof(Scanner));
  scanner->at_start = true;
  scanner->in_frontmatter = false;
  return scanner;
}

void tree_sitter_markdoc_external_scanner_destroy(void *payload) {
  free(payload);
}

unsigned tree_sitter_markdoc_external_scanner_serialize(void *payload, char *buffer) {
  Scanner *s = (Scanner *)payload;
  unsigned i = 0;
  if (i < 255) {
    buffer[i++] = (char)(s->at_start ? 1 : 0);
    buffer[i++] = (char)(s->in_frontmatter ? 1 : 0);
  }
  return i;
}

void tree_sitter_markdoc_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  Scanner *s = (Scanner *)payload;
  s->at_start = true;
  s->in_frontmatter = false;

  unsigned i = 0;
  if (i < length) {
    s->at_start = buffer[i++] != 0;
  }
  if (i < length) {
    s->in_frontmatter = buffer[i++] != 0;
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

static bool scan_frontmatter_closing_delimiter(TSLexer *lexer, int32_t marker);

static bool scan_unordered_or_thematic(Scanner *s, TSLexer *lexer, const bool *valid_symbols, unsigned indent) {
  bool wants_list = valid_symbols[UNORDERED_LIST_MARKER] || valid_symbols[INDENTED_UNORDERED_LIST_MARKER];
  bool wants_frontmatter = valid_symbols[FRONTMATTER_DELIM] && s->at_start;
  if (!valid_symbols[THEMATIC_BREAK] && !wants_list && !wants_frontmatter) {
    return false;
  }

  int32_t marker = lexer->lookahead;
  if (marker != '*' && marker != '-' && marker != '_') {
    return false;
  }

  unsigned marker_count = 0;
  while (lexer->lookahead == marker) {
    lexer->advance(lexer, false);
    marker_count++;
  }

  if (marker_count == 0) {
    return false;
  }

  bool has_space = false;
  while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
    lexer->advance(lexer, false);
    has_space = true;
  }

  if (has_space) {
    lexer->mark_end(lexer);
  }

  unsigned break_count = marker_count;
  while (lexer->lookahead == marker || lexer->lookahead == ' ' || lexer->lookahead == '\t') {
    if (lexer->lookahead == marker) {
      break_count++;
    }
    lexer->advance(lexer, false);
  }

  bool line_end = lexer->lookahead == '\n' || lexer->lookahead == '\r' || lexer->lookahead == 0;
  bool has_content = !line_end;
  bool frontmatter_candidate = wants_frontmatter && marker == '-' && indent == 0 &&
    marker_count == 3 && break_count == 3 && line_end;

  if (frontmatter_candidate) {
    if (!has_space) {
      lexer->mark_end(lexer);
    }

    if (scan_frontmatter_closing_delimiter(lexer, marker)) {
      lexer->result_symbol = FRONTMATTER_DELIM;
      s->in_frontmatter = true;
      s->at_start = false;
      return true;
    }

    if (valid_symbols[THEMATIC_BREAK] && indent < 4) {
      lexer->result_symbol = THEMATIC_BREAK;
      return true;
    }

    return false;
  }

  if (valid_symbols[THEMATIC_BREAK] && line_end && break_count >= 3 && indent < 4) {
    lexer->result_symbol = THEMATIC_BREAK;
    lexer->mark_end(lexer);
    return true;
  }
  if ((!has_space && !has_content) || (marker != '*' && marker != '-')) {
    return false;
  }

  if (marker == '-' && marker_count > 1) {
    return false;
  }

  bool is_indented = indent > 0 || (marker == '*' && marker_count > 1);
  if (is_indented) {
    if (!valid_symbols[INDENTED_UNORDERED_LIST_MARKER]) {
      return false;
    }
    lexer->result_symbol = INDENTED_UNORDERED_LIST_MARKER;
  } else {
    if (!valid_symbols[UNORDERED_LIST_MARKER]) {
      return false;
    }
    lexer->result_symbol = UNORDERED_LIST_MARKER;
  }

  return true;
}

static bool scan_unordered_list_plus(TSLexer *lexer, const bool *valid_symbols, unsigned indent) {
  bool wants_list = valid_symbols[UNORDERED_LIST_MARKER] || valid_symbols[INDENTED_UNORDERED_LIST_MARKER];
  if (!wants_list || lexer->lookahead != '+') {
    return false;
  }

  lexer->advance(lexer, false);
  bool has_space = lexer->lookahead == ' ' || lexer->lookahead == '\t';
  while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
    lexer->advance(lexer, false);
  }
  bool has_content = lexer->lookahead != '\n' && lexer->lookahead != '\r' && lexer->lookahead != 0;
  if (!has_space && !has_content) {
    return false;
  }

  if (indent > 0) {
    if (!valid_symbols[INDENTED_UNORDERED_LIST_MARKER]) {
      return false;
    }
    lexer->result_symbol = INDENTED_UNORDERED_LIST_MARKER;
  } else {
    if (!valid_symbols[UNORDERED_LIST_MARKER]) {
      return false;
    }
    lexer->result_symbol = UNORDERED_LIST_MARKER;
  }

  lexer->mark_end(lexer);
  return true;
}

static bool scan_ordered_list_marker(TSLexer *lexer, const bool *valid_symbols, unsigned indent) {
  bool wants_list = valid_symbols[ORDERED_LIST_MARKER] || valid_symbols[INDENTED_ORDERED_LIST_MARKER];
  if (!wants_list) {
    return false;
  }

  unsigned digits = 0;
  while (is_digit_ch(lexer->lookahead) && digits < 9) {
    lexer->advance(lexer, false);
    digits++;
  }

  if (digits == 0) {
    return false;
  }

  if (lexer->lookahead != '.' && lexer->lookahead != ')') {
    return false;
  }
  lexer->advance(lexer, false);

  bool has_space = lexer->lookahead == ' ' || lexer->lookahead == '\t';
  while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
    lexer->advance(lexer, false);
  }
  bool has_content = lexer->lookahead != '\n' && lexer->lookahead != '\r' && lexer->lookahead != 0;
  if (!has_space && !has_content) {
    return false;
  }

  if (indent > 0) {
    if (!valid_symbols[INDENTED_ORDERED_LIST_MARKER]) {
      return false;
    }
    lexer->result_symbol = INDENTED_ORDERED_LIST_MARKER;
  } else {
    if (!valid_symbols[ORDERED_LIST_MARKER]) {
      return false;
    }
    lexer->result_symbol = ORDERED_LIST_MARKER;
  }

  lexer->mark_end(lexer);
  return true;
}


static bool scan_literal(TSLexer *lexer, const char *text);
static bool is_thematic_break_line(TSLexer *lexer);
static bool scan_frontmatter_delimiter(TSLexer *lexer);
static bool scan_soft_line_break(TSLexer *lexer);
static bool is_heading_marker_line(TSLexer *lexer);
static bool is_blockquote_line(TSLexer *lexer);
static bool is_fenced_code_line(TSLexer *lexer);
static bool is_list_marker_line(TSLexer *lexer);
static bool is_markdoc_block_tag_line(TSLexer *lexer);
static bool scan_unordered_or_thematic(Scanner *s, TSLexer *lexer, const bool *valid_symbols, unsigned indent);
static bool scan_unordered_list_plus(TSLexer *lexer, const bool *valid_symbols, unsigned indent);
static bool scan_ordered_list_marker(TSLexer *lexer, const bool *valid_symbols, unsigned indent);

static bool scan_frontmatter_delimiter(TSLexer *lexer) {
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

  if (!is_newline(lexer->lookahead) && lexer->lookahead != 0) {
    *lexer = saved_state;
    return false;
  }

  lexer->mark_end(lexer);
  return true;
}

static bool scan_frontmatter_closing_delimiter(TSLexer *lexer, int32_t marker) {
  if (!is_newline(lexer->lookahead)) {
    return false;
  }

  if (lexer->lookahead == '\r') {
    lexer->advance(lexer, false);
    if (lexer->lookahead == '\n') {
      lexer->advance(lexer, false);
    }
  } else {
    lexer->advance(lexer, false);
  }

  for (;;) {
    if (lexer->lookahead == 0) {
      return false;
    }

    if (lexer->lookahead == marker) {
      unsigned count = 0;
      while (lexer->lookahead == marker && count < 3) {
        lexer->advance(lexer, false);
        count++;
      }

      if (count == 3) {
        while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
          lexer->advance(lexer, false);
        }

        if (lexer->lookahead == 0 || is_newline(lexer->lookahead)) {
          return true;
        }
      }
    }

    while (!is_newline(lexer->lookahead) && lexer->lookahead != 0) {
      lexer->advance(lexer, false);
    }

    if (lexer->lookahead == '\r') {
      lexer->advance(lexer, false);
      if (lexer->lookahead == '\n') {
        lexer->advance(lexer, false);
      }
    } else if (lexer->lookahead == '\n') {
      lexer->advance(lexer, false);
    }
  }
}

static bool is_heading_marker_line(TSLexer *lexer) {
  if (lexer->get_column(lexer) != 0) {
    return false;
  }

  TSLexer saved_state = *lexer;
  unsigned count = 0;
  while (lexer->lookahead == '#' && count < 6) {
    lexer->advance(lexer, false);
    count++;
  }

  bool ok = count > 0 && count <= 6 && (lexer->lookahead == ' ' || lexer->lookahead == '\t');
  *lexer = saved_state;
  return ok;
}

static bool is_blockquote_line(TSLexer *lexer) {
  if (lexer->get_column(lexer) != 0) {
    return false;
  }

  return lexer->lookahead == '>';
}

static bool is_fenced_code_line(TSLexer *lexer) {
  if (lexer->get_column(lexer) != 0) {
    return false;
  }

  int32_t marker = lexer->lookahead;
  if (marker != '`' && marker != '~') {
    return false;
  }

  TSLexer saved_state = *lexer;
  unsigned count = 0;
  while (lexer->lookahead == marker && count < 3) {
    lexer->advance(lexer, false);
    count++;
  }

  bool ok = count >= 3;
  *lexer = saved_state;
  return ok;
}

static bool scan_soft_line_break(TSLexer *lexer) {
  if (!is_newline(lexer->lookahead)) {
    return false;
  }

  TSLexer saved_state = *lexer;
  if (lexer->lookahead == '\r') {
    lexer->advance(lexer, false);
    if (lexer->lookahead == '\n') {
      lexer->advance(lexer, false);
    }
  } else {
    lexer->advance(lexer, false);
  }

  TSLexer line_state = *lexer;
  lexer->mark_end(lexer);

  TSLexer blank_state = *lexer;
  while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
    lexer->advance(lexer, false);
  }
  if (lexer->lookahead == 0 || is_newline(lexer->lookahead)) {
    *lexer = saved_state;
    return false;
  }
  *lexer = blank_state;

  if (is_heading_marker_line(lexer) || is_blockquote_line(lexer) ||
      is_fenced_code_line(lexer) || is_thematic_break_line(lexer) ||
      is_list_marker_line(lexer) || is_markdoc_block_tag_line(lexer)) {
    *lexer = saved_state;
    return false;
  }

  *lexer = line_state;
  return true;
}

static bool is_list_marker_line(TSLexer *lexer) {
  TSLexer saved_state = *lexer;

  while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
    lexer->advance(lexer, false);
  }

  int32_t first = lexer->lookahead;
  if (first == '*' || first == '+' || first == '-') {
    lexer->advance(lexer, false);
    bool ok = lexer->lookahead == ' ' || lexer->lookahead == '\t';
    *lexer = saved_state;
    return ok;
  }

  if (is_digit_ch(first)) {
    size_t digits = 0;
    while (is_digit_ch(lexer->lookahead) && digits < 9) {
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

static bool is_markdoc_block_tag_line(TSLexer *lexer) {
  if (lexer->get_column(lexer) != 0) {
    return false;
  }

  TSLexer saved_state = *lexer;
  if (lexer->lookahead != '{') {
    return false;
  }

  lexer->advance(lexer, false);
  if (lexer->lookahead != '%') {
    *lexer = saved_state;
    return false;
  }
  lexer->advance(lexer, false);

  while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
    lexer->advance(lexer, false);
  }

  bool found_close = false;
  while (lexer->lookahead != 0 && !is_newline(lexer->lookahead)) {
    if (lexer->lookahead == '%') {
      TSLexer percent_state = *lexer;
      lexer->advance(lexer, false);
      if (lexer->lookahead == '}') {
        lexer->advance(lexer, false);
        while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
          lexer->advance(lexer, false);
        }
        found_close = true;
        break;
      }
      *lexer = percent_state;
    }
    lexer->advance(lexer, false);
  }

  bool ok = found_close && (lexer->lookahead == 0 || is_newline(lexer->lookahead));
  *lexer = saved_state;
  return ok;
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

bool tree_sitter_markdoc_external_scanner_scan(void *payload, TSLexer *lexer,
                                              const bool *valid_symbols) {
  Scanner *s = (Scanner *)payload;

  if (lexer->get_column(lexer) == 0) {
    if (valid_symbols[FRONTMATTER_DELIM] && s->in_frontmatter) {
      if (scan_frontmatter_delimiter(lexer)) {
        lexer->result_symbol = FRONTMATTER_DELIM;
        s->in_frontmatter = false;
        s->at_start = false;
        return true;
      }
    }

    TSLexer list_state = *lexer;
    unsigned indent = 0;
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
      lexer->advance(lexer, false);
      indent++;
    }

    int32_t marker = lexer->lookahead;
    if (marker == '*' || marker == '-' || marker == '_') {
      if (scan_unordered_or_thematic(s, lexer, valid_symbols, indent)) {
        s->at_start = false;
        return true;
      }
    } else if (marker == '+') {
      if (scan_unordered_list_plus(lexer, valid_symbols, indent)) {
        s->at_start = false;
        return true;
      }
    } else if (is_digit_ch(marker)) {
      if (scan_ordered_list_marker(lexer, valid_symbols, indent)) {
        s->at_start = false;
        return true;
      }
    }

    *lexer = list_state;
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
  if (valid_symbols[LIST_CONTINUATION]) {
    TSLexer saved_state = *lexer;
    bool at_line_start = lexer->get_column(lexer) == 0;
    bool starts_with_indent = lexer->lookahead == ' ' || lexer->lookahead == '\t';
    TSLexer line_state = *lexer;

    if (is_newline(lexer->lookahead)) {
      if (lexer->lookahead == '\r') {
        lexer->advance(lexer, false);
        if (lexer->lookahead == '\n') {
          lexer->advance(lexer, false);
        }
      } else {
        lexer->advance(lexer, false);
      }
      line_state = *lexer;
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

    TSLexer marker_state = *lexer;
    *lexer = line_state;
    bool starts_block = is_list_marker_line(lexer) || is_thematic_break_line(lexer);
    *lexer = marker_state;
    if (starts_block) {
      *lexer = saved_state;
      return false;
    }

    lexer->mark_end(lexer);
    lexer->result_symbol = LIST_CONTINUATION;
    s->at_start = false;
    return true;
  }

  if (valid_symbols[SOFT_LINE_BREAK] && scan_soft_line_break(lexer)) {
    lexer->result_symbol = SOFT_LINE_BREAK;
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
