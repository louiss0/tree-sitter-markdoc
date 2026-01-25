#include "tree_sitter/parser.h"
#include <stdlib.h>
#include <wctype.h>
#include <stdbool.h>
#include <string.h>

enum TokenType {
  CODE_CONTENT,
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
  PARAGRAPH_CONTINUATION,
  TAG_START,
  EM_OPEN_STAR,
  EM_CLOSE_STAR,
  STRONG_OPEN_STAR,
  STRONG_CLOSE_STAR,
  EM_OPEN_UNDERSCORE,
  EM_CLOSE_UNDERSCORE,
  STRONG_OPEN_UNDERSCORE,
  STRONG_CLOSE_UNDERSCORE,
  RAW_DELIM,
  TEXT,
  HTML_COMMENT,
  HTML_BLOCK
};

typedef struct {
  int32_t prev;  // previous non-newline char for flanking detection
  bool in_list;
} Scanner;

void *tree_sitter_markdoc_external_scanner_create() {
  Scanner *scanner = (Scanner *)malloc(sizeof(Scanner));
  scanner->prev = 0;
  scanner->in_list = false;
  return scanner;
}

void tree_sitter_markdoc_external_scanner_destroy(void *payload) {
  free(payload);
}

unsigned tree_sitter_markdoc_external_scanner_serialize(void *payload, char *buffer) {
  Scanner *s = (Scanner *)payload;
  unsigned i = 0;
  // Serialize prev character (4 bytes, little-endian)
  if (i + 4 < 255) {
    int32_t p = s->prev;
    buffer[i++] = (char)(p & 0xFF);
    buffer[i++] = (char)((p >> 8) & 0xFF);
    buffer[i++] = (char)((p >> 16) & 0xFF);
    buffer[i++] = (char)((p >> 24) & 0xFF);
  }
  if (i < 255) {
    buffer[i++] = (char)(s->in_list ? 1 : 0);
  }
  return i;
}

void tree_sitter_markdoc_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  Scanner *s = (Scanner *)payload;
  s->prev = 0;
  s->in_list = false;

  unsigned i = 0;
  // Deserialize prev character (4 bytes, little-endian)
  if (i + 4 <= length) {
    int32_t p = 0;
    p |= (int32_t)(uint8_t)buffer[i++];
    p |= (int32_t)(uint8_t)buffer[i++] << 8;
    p |= (int32_t)(uint8_t)buffer[i++] << 16;
    p |= (int32_t)(uint8_t)buffer[i++] << 24;
    s->prev = p;
  }
  if (i < length) {
    s->in_list = buffer[i++] != 0;
  }
}

static inline bool is_newline(int32_t ch) {
  return ch == '\n' || ch == '\r';
}

static inline bool is_space_ch(int32_t c) {
  return c == ' ' || c == '\t';
}

static inline bool is_word_ch(int32_t c) {
  return iswalnum(c) || c == '_';
}

static inline bool is_punct_ch(int32_t c) {
  return iswpunct(c);
}
static inline bool is_text_start_char(int32_t c) {
  if (c == 0 || is_newline(c)) return false;
  switch (c) {
    case '{':
    case '<':
    case '[':
    case '!':
    case '`':
    case '*':
    case '_':
      return false;
    default:
      return true;
  }
}

static inline bool is_text_continue_char(int32_t c) {
  if (c == 0 || is_newline(c)) return false;
  switch (c) {
    case '{':
    case '<':
    case '[':
    case '!':
    case '`':
    case '*':
    case '_':
      return false;
    default:
      return true;
  }
}

static bool scan_literal(TSLexer *lexer, const char *text);

static bool is_frontmatter_delimiter_line(TSLexer *lexer) {
  if (lexer->get_column(lexer) != 0 || lexer->lookahead != '-') {
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

static bool is_braced_line_without_trailing_content(TSLexer *lexer) {
  TSLexer saved_state = *lexer;
  uint8_t indentation = 0;

  while ((lexer->lookahead == ' ' || lexer->lookahead == '\t') && indentation < 4) {
    lexer->advance(lexer, false);
    indentation++;
  }

  if (indentation >= 4) {
    *lexer = saved_state;
    return false;
  }

  if (lexer->lookahead != '{') {
    *lexer = saved_state;
    return false;
  }
  lexer->advance(lexer, false);
  if (lexer->lookahead != '%') {
    *lexer = saved_state;
    return false;
  }
  lexer->advance(lexer, false);

  bool found_close = false;
  while (lexer->lookahead != 0 && !is_newline(lexer->lookahead)) {
    if (scan_brace_close(lexer)) {
      found_close = true;
      break;
    }
    lexer->advance(lexer, false);
  }

  if (!found_close) {
    *lexer = saved_state;
    return false;
  }

  while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
    lexer->advance(lexer, false);
  }

  bool ok = lexer->lookahead == 0 || is_newline(lexer->lookahead);
  *lexer = saved_state;
  return ok;
}

static bool is_tag_line(TSLexer *lexer) {
  TSLexer saved_state = *lexer;
  uint8_t indentation = 0;

  while ((lexer->lookahead == ' ' || lexer->lookahead == '\t') && indentation < 4) {
    lexer->advance(lexer, false);
    indentation++;
  }

  if (indentation >= 4) {
    *lexer = saved_state;
    return false;
  }

  if (lexer->lookahead != '{') {
    *lexer = saved_state;
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

  TSLexer keyword_state = *lexer;
  if (scan_literal(lexer, "comment")) {
    *lexer = saved_state;
    return false;
  }
  *lexer = keyword_state;

  if (lexer->lookahead == '/') {
    lexer->advance(lexer, false);
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
      lexer->advance(lexer, false);
    }
  }

  if (!iswalpha(lexer->lookahead) && lexer->lookahead != '_') {
    *lexer = saved_state;
    return false;
  }

  lexer->advance(lexer, false);
  while (iswalpha(lexer->lookahead) || iswdigit(lexer->lookahead) ||
         lexer->lookahead == '_' || lexer->lookahead == '-') {
    lexer->advance(lexer, false);
  }

  bool found_close = false;
  while (lexer->lookahead != 0 && !is_newline(lexer->lookahead)) {
    if (scan_brace_close(lexer)) {
      found_close = true;
      break;
    }
    lexer->advance(lexer, false);
  }

  if (!found_close) {
    *lexer = saved_state;
    return false;
  }

  while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
    lexer->advance(lexer, false);
  }

  bool ok = lexer->lookahead == 0 || is_newline(lexer->lookahead);
  *lexer = saved_state;
  return ok;
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

static bool scan_html_tag_name(TSLexer *lexer, char *buffer, size_t buffer_len, size_t *name_len) {
  if (!iswalpha(lexer->lookahead)) {
    return false;
  }

  size_t i = 0;
  while (iswalpha(lexer->lookahead) || iswdigit(lexer->lookahead)) {
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
  if (lexer->get_column(lexer) != 0 || lexer->lookahead != '<') {
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
        break;
      }
      *lexer = slash_state;
    }

    if (lexer->lookahead == '>') {
      lexer->advance(lexer, false);
      lexer->mark_end(lexer);
      break;
    }

    lexer->advance(lexer, false);
  }

  if (self_closing) {
    return true;
  }

  if (lexer->lookahead == 0) {
    *lexer = saved_state;
    return false;
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

    lexer->advance(lexer, false);
  }

  *lexer = saved_state;
  return false;
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

  if (is_thematic_break_line(lexer)) {
    *lexer = saved_state;
    return false;
  }

  int32_t first = lexer->lookahead;
  if (first == '*' || first == '+') {
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

  if (is_thematic_break_line(lexer)) {
    *lexer = saved_state;
    return false;
  }

  int32_t first = lexer->lookahead;
  if (first == '*' || first == '+') {
    if (indentation != 0) {
      *lexer = saved_state;
      return false;
    }
    enum TokenType symbol = first == '*' ? LIST_MARKER_STAR : LIST_MARKER_PLUS;
    enum TokenType dont_interrupt_symbol =
        first == '*' ? LIST_MARKER_STAR_DONT_INTERRUPT : LIST_MARKER_PLUS_DONT_INTERRUPT;
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

static bool is_paragraph_continuation_line(TSLexer *lexer, const bool *valid_symbols) {
  TSLexer saved_state = *lexer;

  if (is_frontmatter_delimiter_line(lexer)) {
    *lexer = saved_state;
    return false;
  }

  *lexer = saved_state;
  if (is_thematic_break_line(lexer)) {
    *lexer = saved_state;
    return false;
  }

  *lexer = saved_state;
  bool list_marker_valid = valid_symbols[LIST_MARKER_PLUS] || valid_symbols[LIST_MARKER_STAR] ||
                           valid_symbols[LIST_MARKER_DOT] || valid_symbols[LIST_MARKER_PARENTHESIS];
  if (list_marker_valid && is_simple_list_marker_start(lexer)) {
    *lexer = saved_state;
    return false;
  }

  *lexer = saved_state;
  if (is_braced_line_without_trailing_content(lexer)) {
    *lexer = saved_state;
    return false;
  }

  *lexer = saved_state;
  while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
    lexer->advance(lexer, false);
  }

  if (lexer->lookahead == 0 || is_newline(lexer->lookahead)) {
    *lexer = saved_state;
    return false;
  }

  if (lexer->lookahead == '-') {
    TSLexer dash_state = *lexer;
    lexer->advance(lexer, false);
    if (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
      *lexer = saved_state;
      return false;
    }
    *lexer = dash_state;
  }

  if (lexer->lookahead == '>') {
    *lexer = saved_state;
    return false;
  }

  if (lexer->lookahead == '#') {
    int count = 0;
    while (lexer->lookahead == '#' && count < 6) {
      lexer->advance(lexer, false);
      count++;
    }
    if (count > 0 && (lexer->lookahead == ' ' || lexer->lookahead == '\t')) {
      *lexer = saved_state;
      return false;
    }
  }

  if (lexer->lookahead == '`' || lexer->lookahead == '~') {
    int32_t fence = lexer->lookahead;
    int count = 0;
    while (lexer->lookahead == fence && count < 3) {
      lexer->advance(lexer, false);
      count++;
    }
    if (count >= 3) {
      *lexer = saved_state;
      return false;
    }
  }

  if (lexer->lookahead == '<') {
    TSLexer html_state = *lexer;
    bool is_html = scan_html_block(lexer) || scan_html_comment(lexer);
    *lexer = html_state;
    if (is_html) {
      *lexer = saved_state;
      return false;
    }
  }

  *lexer = saved_state;
  return true;
}

bool tree_sitter_markdoc_external_scanner_scan(void *payload, TSLexer *lexer,
                                              const bool *valid_symbols) {
  Scanner *s = (Scanner *)payload;

  if (lexer->get_column(lexer) == 0 && is_frontmatter_delimiter_line(lexer)) {
    return false;
  }

  if (lexer->get_column(lexer) == 0 &&
      (lexer->lookahead == '*' || lexer->lookahead == '+')) {
    TSLexer saved_state = *lexer;
    int32_t first = lexer->lookahead;
    lexer->advance(lexer, false);
    if (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
      while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
        lexer->advance(lexer, false);
      }
      lexer->mark_end(lexer);
      lexer->result_symbol = first == '*' ? LIST_MARKER_STAR : LIST_MARKER_PLUS;
      s->prev = ' ';
      s->in_list = true;
      return true;
    }
    *lexer = saved_state;
  }

  // CODE_CONTENT: consume everything until we see a code fence close pattern
  // The grammar will handle detecting code_fence_open and code_fence_close
  if (valid_symbols[CODE_CONTENT]) {
    // Only match CODE_CONTENT if we are immediately after the fence line newline
    if (!is_newline(lexer->lookahead)) {
      return false;
    }

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
      return true;
    }

    *lexer = start_state;
    return false;
  }

  if (lexer->get_column(lexer) == 0 && scan_simple_list_marker(lexer, valid_symbols)) {
    s->prev = ' ';
    if (lexer->result_symbol == LIST_MARKER_STAR || lexer->result_symbol == LIST_MARKER_PLUS ||
        lexer->result_symbol == LIST_MARKER_DOT || lexer->result_symbol == LIST_MARKER_PARENTHESIS) {
      s->in_list = true;
    }
    return true;
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
    s->prev = '\n';
    return true;
  }

  if (valid_symbols[THEMATIC_BREAK] && is_thematic_break_line(lexer)) {
    while (lexer->lookahead != 0 && !is_newline(lexer->lookahead)) {
      lexer->advance(lexer, false);
    }
    lexer->mark_end(lexer);
    lexer->result_symbol = THEMATIC_BREAK;
    return true;
  }

  if (valid_symbols[PARAGRAPH_CONTINUATION] && is_newline(lexer->lookahead)) {
    TSLexer saved_state = *lexer;

    if (lexer->lookahead == '\r') {
      lexer->advance(lexer, false);
      if (lexer->lookahead == '\n') {
        lexer->advance(lexer, false);
      }
    } else {
      lexer->advance(lexer, false);
    }

    lexer->mark_end(lexer);

    TSLexer line_state = *lexer;
    TSLexer marker_state = *lexer;
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
      lexer->advance(lexer, false);
    }

    if (lexer->lookahead == 0 || is_newline(lexer->lookahead)) {
      *lexer = saved_state;
      return false;
    }

    if (lexer->lookahead == '-' || lexer->lookahead == '*' || lexer->lookahead == '+') {
      lexer->advance(lexer, false);
      bool marker_line = lexer->lookahead == ' ' || lexer->lookahead == '\t';
      *lexer = marker_state;
      if (marker_line) {
        *lexer = saved_state;
        return false;
      }
    } else if (iswdigit(lexer->lookahead)) {
      while (iswdigit(lexer->lookahead)) {
        lexer->advance(lexer, false);
      }
      if (lexer->lookahead == '.' || lexer->lookahead == ')') {
        lexer->advance(lexer, false);
        if (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
          *lexer = saved_state;
          return false;
        }
      }
      *lexer = marker_state;
    } else {
      *lexer = marker_state;
    }

    if (!is_newline(lexer->lookahead) && lexer->lookahead != 0 &&
        is_paragraph_continuation_line(lexer, valid_symbols)) {
      *lexer = line_state;
      lexer->result_symbol = PARAGRAPH_CONTINUATION;
      s->prev = '\n';
      return true;
    }

    *lexer = saved_state;
  }

  if (valid_symbols[HTML_COMMENT] && scan_html_comment(lexer)) {
    lexer->result_symbol = HTML_COMMENT;
    s->prev = 0;
    return true;
  }

  if (valid_symbols[HTML_BLOCK] && scan_html_block(lexer)) {
    lexer->result_symbol = HTML_BLOCK;
    s->prev = 0;
    return true;
  }

  if (valid_symbols[TAG_START] && is_tag_line(lexer)) {
    uint8_t indentation = 0;
    while ((lexer->lookahead == ' ' || lexer->lookahead == '\t') && indentation < 4) {
      lexer->advance(lexer, false);
      indentation++;
    }
    if (lexer->lookahead == '{') {
      lexer->advance(lexer, false);
      if (lexer->lookahead == '%') {
        lexer->advance(lexer, false);
        lexer->mark_end(lexer);
        lexer->result_symbol = TAG_START;
        s->prev = '%';
        return true;
      }
    }
  }

  if (valid_symbols[TEXT] && is_frontmatter_delimiter_line(lexer)) {
    return false;
  }

  if (valid_symbols[TEXT] && lexer->get_column(lexer) == 0 &&
      (lexer->lookahead == ' ' || lexer->lookahead == '\t')) {
    TSLexer saved_state = *lexer;

    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
      lexer->advance(lexer, false);
    }

    int32_t first = lexer->lookahead;

    if (first == 0) {
      *lexer = saved_state;
      return false;
    } else if (is_newline(first)) {
      *lexer = saved_state;
      return false;
    } else {
      bool is_block = false;

      if (!is_block) {
        TSLexer themed_state = *lexer;
        if (is_thematic_break_line(lexer)) {
          is_block = true;
        }
        *lexer = themed_state;
      }

      if (first == '>' || first == '<') {
        is_block = true;
      } else if (first == '{') {
        lexer->advance(lexer, false);
        if (lexer->lookahead == '{' || lexer->lookahead == '%') {
          is_block = true;
        }
      } else if (first == '#') {
        lexer->advance(lexer, false);
        if (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
          is_block = true;
        }
      } else if (first == '`' || first == '~') {
        int count = 0;
        int32_t ch = first;
        while (lexer->lookahead == ch && count < 3) {
          lexer->advance(lexer, false);
          count++;
        }
        if (count >= 3) {
          is_block = true;
        }
      } else if (first == '*' || first == '+') {
        lexer->advance(lexer, false);
        if (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
          is_block = true;
        }
      } else if (iswdigit(first)) {
        while (iswdigit(lexer->lookahead)) {
          lexer->advance(lexer, false);
        }
        if (lexer->lookahead == '.' || lexer->lookahead == ')') {
          lexer->advance(lexer, false);
          if (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
            is_block = true;
          }
        }
      }

      *lexer = saved_state;

      if (!is_block) {
        lexer->mark_end(lexer);
        int32_t last = 0;
        while (is_text_continue_char(lexer->lookahead)) {
          last = lexer->lookahead;
          lexer->advance(lexer, false);
          lexer->mark_end(lexer);
        }

        if (last != 0) {
          s->prev = last;
          lexer->result_symbol = TEXT;
          return true;
        }
      }
    }
  }

  if (valid_symbols[TEXT] && lexer->get_column(lexer) == 0) {
    TSLexer list_state = *lexer;
    if (is_simple_list_marker_start(lexer)) {
      *lexer = list_state;
      return false;
    }
    *lexer = list_state;
  }

  if (valid_symbols[TEXT] && is_text_start_char(lexer->lookahead)) {
    uint32_t column = lexer->get_column(lexer);
    int32_t first = lexer->lookahead;

    if (column == 0) {
      if (is_frontmatter_delimiter_line(lexer)) {
        return false;
      }

      if (first == '>' || first == '{' || first == '<') {
        return false;
      }

      if (first == '#' ) {
        TSLexer saved_state = *lexer;
        uint32_t count = 0;
        while (lexer->lookahead == '#' && count < 6) {
          lexer->advance(lexer, false);
          count++;
        }
        if (count > 0 && (lexer->lookahead == ' ' || lexer->lookahead == '\t')) {
          *lexer = saved_state;
          return false;
        }
        *lexer = saved_state;
      }

      if (is_thematic_break_line(lexer)) {
        return false;
      }

      if (first == '*' || first == '+' || first == '_') {
        TSLexer saved_state = *lexer;
        *lexer = saved_state;
        if (first == '*' || first == '+') {
          lexer->advance(lexer, false);
          int32_t next = lexer->lookahead;
          *lexer = saved_state;
          if (next == ' ' || next == '\t') {
            return false;
          }
        }
      }

      if (iswdigit(first)) {
        TSLexer saved_state = *lexer;
        while (iswdigit(lexer->lookahead)) {
          lexer->advance(lexer, false);
        }
        if (lexer->lookahead == '.' || lexer->lookahead == ')') {
          lexer->advance(lexer, false);
          if (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
            *lexer = saved_state;
            return false;
          }
        }
        *lexer = saved_state;
      }

      if (first == '`' || first == '~') {
        TSLexer saved_state = *lexer;
        int count = 0;
        while (lexer->lookahead == first && count < 3) {
          lexer->advance(lexer, false);
          count++;
        }
        if (count >= 3) {
          *lexer = saved_state;
          return false;
        }
        *lexer = saved_state;
      }
    }

    lexer->mark_end(lexer);
    int32_t last = 0;
    while (is_text_continue_char(lexer->lookahead)) {
      last = lexer->lookahead;
      lexer->advance(lexer, false);
      lexer->mark_end(lexer);
    }

    if (last != 0) {
      s->prev = last;
      if (column == 0 && first != ' ' && first != '\t') {
        s->in_list = false;
      }
      lexer->result_symbol = TEXT;
      return true;
    }
  }

  // EMPHASIS DELIMITERS: handle * and _ with flanking rules
  if (lexer->lookahead == '*' || lexer->lookahead == '_') {
    if (lexer->get_column(lexer) == 0 && lexer->lookahead == '*') {
      TSLexer saved_state = *lexer;
      lexer->advance(lexer, false);
      bool is_list = lexer->lookahead == ' ' || lexer->lookahead == '\t';
      *lexer = saved_state;
      if (is_list) {
        return false;
      }
    }
    if (!valid_symbols[EM_OPEN_STAR] && !valid_symbols[EM_CLOSE_STAR] &&
        !valid_symbols[STRONG_OPEN_STAR] && !valid_symbols[STRONG_CLOSE_STAR] &&
        !valid_symbols[EM_OPEN_UNDERSCORE] && !valid_symbols[EM_CLOSE_UNDERSCORE] &&
        !valid_symbols[STRONG_OPEN_UNDERSCORE] && !valid_symbols[STRONG_CLOSE_UNDERSCORE] &&
        !valid_symbols[RAW_DELIM]) {
      if (!is_newline(lexer->lookahead) && lexer->lookahead != 0) {
        s->prev = lexer->lookahead;
      }
      return false;
    }

    int32_t delim = lexer->lookahead;
    int32_t prev = s->prev;

    lexer->advance(lexer, false);
    int32_t after_first = lexer->lookahead;
    bool has_second = (after_first == delim);

    int32_t next_after_run = after_first;
    if (has_second) {
      lexer->advance(lexer, false);
      next_after_run = lexer->lookahead;
    }

    bool next_is_space = is_space_ch(next_after_run) || is_newline(next_after_run) || next_after_run == 0;
    bool prev_is_space = is_space_ch(prev) || prev == 0 || is_newline(prev);
    bool next_is_word = is_word_ch(next_after_run);
    bool prev_is_word = is_word_ch(prev) || (!is_space_ch(prev) && !is_newline(prev) && prev != 0);

    bool left_flanking = next_is_word && (prev_is_space || is_punct_ch(prev));
    bool right_flanking = prev_is_word && (next_is_space || is_punct_ch(next_after_run));

    if (!has_second && next_is_space) {
      left_flanking = false;
    }

    lexer->mark_end(lexer);

    if (has_second) {
      if (left_flanking && !right_flanking) {
        if (delim == '*' && valid_symbols[STRONG_OPEN_STAR]) {
          lexer->result_symbol = STRONG_OPEN_STAR;
          s->prev = delim;
          return true;
        }
        if (delim == '_' && valid_symbols[STRONG_OPEN_UNDERSCORE]) {
          lexer->result_symbol = STRONG_OPEN_UNDERSCORE;
          s->prev = delim;
          return true;
        }
      }
      if (right_flanking) {
        if (delim == '*' && valid_symbols[STRONG_CLOSE_STAR]) {
          lexer->result_symbol = STRONG_CLOSE_STAR;
          s->prev = delim;
          return true;
        }
        if (delim == '_' && valid_symbols[STRONG_CLOSE_UNDERSCORE]) {
          lexer->result_symbol = STRONG_CLOSE_UNDERSCORE;
          s->prev = delim;
          return true;
        }
      }
    }

    if (left_flanking && !right_flanking) {
      if (delim == '*' && valid_symbols[EM_OPEN_STAR]) {
        lexer->result_symbol = EM_OPEN_STAR;
        s->prev = delim;
        return true;
      }
      if (delim == '_' && valid_symbols[EM_OPEN_UNDERSCORE]) {
        lexer->result_symbol = EM_OPEN_UNDERSCORE;
        s->prev = delim;
        return true;
      }
    }
    if (right_flanking) {
      if (delim == '*' && valid_symbols[EM_CLOSE_STAR]) {
        lexer->result_symbol = EM_CLOSE_STAR;
        s->prev = delim;
        return true;
      }
      if (delim == '_' && valid_symbols[EM_CLOSE_UNDERSCORE]) {
        lexer->result_symbol = EM_CLOSE_UNDERSCORE;
        s->prev = delim;
        return true;
      }
    }

    if (valid_symbols[RAW_DELIM]) {
      lexer->result_symbol = RAW_DELIM;
      s->prev = delim;
      return true;
    }

    s->prev = delim;
    return false;
  }

  return false;
}
