#include <tree_sitter/parser.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>
#include <stdbool.h>

enum TokenType {
  CODE_CONTENT,
  NEWLINE,
  BLANK_LINE,
  INDENT,
  DEDENT,
  EM_OPEN_STAR,
  EM_CLOSE_STAR,
  STRONG_OPEN_STAR,
  STRONG_CLOSE_STAR,
  EM_OPEN_UNDERSCORE,
  EM_CLOSE_UNDERSCORE,
  STRONG_OPEN_UNDERSCORE,
  STRONG_CLOSE_UNDERSCORE,
  RAW_DELIM
};

typedef struct {
  uint16_t indent_stack[64];
  uint8_t indent_len;
  int32_t prev;  // previous non-newline char for flanking detection
} Scanner;

void *tree_sitter_markdoc_external_scanner_create() {
  Scanner *scanner = (Scanner *)malloc(sizeof(Scanner));
  scanner->indent_len = 1;
  scanner->indent_stack[0] = 0;
  scanner->prev = 0;
  return scanner;
}

void tree_sitter_markdoc_external_scanner_destroy(void *payload) {
  free(payload);
}

unsigned tree_sitter_markdoc_external_scanner_serialize(void *payload, char *buffer) {
  Scanner *s = (Scanner *)payload;
  unsigned i = 0;
  buffer[i++] = s->indent_len;
  for (uint8_t j = 0; j < s->indent_len && i + 1 < 255; j++) {
    uint16_t v = s->indent_stack[j];
    buffer[i++] = (char)(v & 0xFF);
    buffer[i++] = (char)((v >> 8) & 0xFF);
  }
  // Serialize prev character (4 bytes, little-endian)
  if (i + 4 < 255) {
    int32_t p = s->prev;
    buffer[i++] = (char)(p & 0xFF);
    buffer[i++] = (char)((p >> 8) & 0xFF);
    buffer[i++] = (char)((p >> 16) & 0xFF);
    buffer[i++] = (char)((p >> 24) & 0xFF);
  }
  return i;
}

void tree_sitter_markdoc_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  Scanner *s = (Scanner *)payload;
  s->indent_len = 1;
  s->indent_stack[0] = 0;
  s->prev = 0;
  
  if (length < 1) return;
  
  unsigned i = 0;
  s->indent_len = (uint8_t)buffer[i++];
  if (s->indent_len == 0) s->indent_len = 1;
  
  for (uint8_t j = 0; j < s->indent_len && i + 1 <= length && j < 64; j++) {
    uint16_t v = (uint8_t)buffer[i++] | (((uint16_t)(uint8_t)buffer[i++]) << 8);
    s->indent_stack[j] = v;
  }
  
  // Deserialize prev character (4 bytes, little-endian)
  if (i + 4 <= length) {
    int32_t p = 0;
    p |= (int32_t)(uint8_t)buffer[i++];
    p |= (int32_t)(uint8_t)buffer[i++] << 8;
    p |= (int32_t)(uint8_t)buffer[i++] << 16;
    p |= (int32_t)(uint8_t)buffer[i++] << 24;
    s->prev = p;
  }
}

static inline bool is_newline(int32_t ch) {
  return ch == '\n' || ch == '\r';
}

static inline bool is_space_ch(int32_t c) {
  return c == ' ' || c == '\t';
}

static inline bool is_punct_ch(int32_t c) {
  return c && !iswalnum(c) && !is_space_ch(c) && !is_newline(c);
}

static inline bool is_word_ch(int32_t c) {
  return iswalnum(c) || c == '_';
}

static bool has_valid_closing_delim(TSLexer *lexer, int32_t delim, int count) {
  TSLexer scan = *lexer;
  int run = 0;

  while (scan.lookahead != 0 && !is_newline(scan.lookahead)) {
    if (scan.lookahead == delim) {
      run++;
      if (run >= count) {
        scan.advance(&scan, false);
        int32_t after = scan.lookahead;
        bool after_is_space = is_space_ch(after) || is_newline(after) || after == 0;
        bool after_is_punct = is_punct_ch(after);
        if (after_is_space || after_is_punct) {
          return true;
        }
        run = 0;
        continue;
      }
    } else {
      run = 0;
    }
    scan.advance(&scan, false);
  }

  return false;
}

bool tree_sitter_markdoc_external_scanner_scan(void *payload, TSLexer *lexer,
                                              const bool *valid_symbols) {
  Scanner *s = (Scanner *)payload;
  
  // INDENT/DEDENT: detect indentation changes
  // Key: we need to handle spaces AFTER emitting the token so extras can skip them
  if ((valid_symbols[INDENT] || valid_symbols[DEDENT]) && (lexer->lookahead == ' ' || lexer->lookahead == '\t')) {
    // Count indentation (peek)
    lexer->mark_end(lexer);
    uint16_t indent = 0;
    while (lexer->lookahead == ' ' || lexer->lookahead == '\t') {
      indent += (lexer->lookahead == ' ') ? 1 : 4;
      lexer->advance(lexer, false);
    }
    
    // If at EOF or newline, don't emit INDENT/DEDENT
    if (lexer->lookahead == 0 || is_newline(lexer->lookahead)) {
      return false;
    }
    
    uint16_t current = s->indent_stack[s->indent_len - 1];
    
    if (indent > current && valid_symbols[INDENT]) {
      if (s->indent_len < 64) {
        s->indent_stack[s->indent_len++] = indent;
      }
      // Mark end to consume the spaces we peeked at
      lexer->mark_end(lexer);
      lexer->result_symbol = INDENT;
      return true;
    }
    
    if (indent < current && valid_symbols[DEDENT]) {
      if (s->indent_len > 1) {
        s->indent_len--;
      }
      // Mark end to consume the spaces we peeked at
      lexer->mark_end(lexer);
      lexer->result_symbol = DEDENT;
      return true;
    }
    
    return false;
  }
  
  // CODE_CONTENT: consume everything until we see a code fence close pattern
  // The grammar will handle detecting code_fence_open and code_fence_close
  if (valid_symbols[CODE_CONTENT]) {
    // Only match CODE_CONTENT if we're at the start of a line or have already seen a newline
    // This ensures the info_string on the fence-opening line can be parsed first
    if (lexer->get_column(lexer) != 0 && lexer->lookahead != '\n') {
      // We're in the middle of a line, probably on the fence-opening line with info_string
      // Don't match CODE_CONTENT yet, let the grammar parse info_string first
      return false;
    }
    
    lexer->mark_end(lexer);
    bool has_content = false;
    
    // Skip the newline after the opening fence
    if (lexer->lookahead == '\n') {
      lexer->advance(lexer, false);
      lexer->mark_end(lexer);
    }
    
    // Check if the very next thing is a closing fence (empty block)
    if (lexer->lookahead == '`' || lexer->lookahead == '~') {
      char fence_char = lexer->lookahead;
      int count = 0;
      while (lexer->lookahead == fence_char && count < 5) {
        count++;
        lexer->advance(lexer, false);
      }
      if (count >= 3) {
        // Empty code block, don't emit CODE_CONTENT
        return false;
      }
      // Not a fence, reset (will re-parse below)
      lexer->mark_end(lexer);
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
    
    return false;
  }
  
  // NEWLINE and BLANK_LINE: handle line breaks
  if ((valid_symbols[NEWLINE] || valid_symbols[BLANK_LINE]) && is_newline(lexer->lookahead)) {
    int newline_count = 0;
    
    // Consume newlines (handle CRLF)
    while (is_newline(lexer->lookahead)) {
      if (lexer->lookahead == '\r') {
        lexer->advance(lexer, false);
        if (lexer->lookahead == '\n') {
          lexer->advance(lexer, false);
        }
        newline_count++;
      } else if (lexer->lookahead == '\n') {
        lexer->advance(lexer, false);
        newline_count++;
      }
    }
    
    lexer->mark_end(lexer);
    
    // Look at next character (without consuming spaces yet)
    int32_t next_char = lexer->lookahead;
    
    // At EOF: don't emit tokens
    if (next_char == 0) {
      return false;
    }
    
    // Check if next line is indented more than current level
    // This signals end of paragraph and start of nested block
    uint16_t current_indent = s->indent_stack[s->indent_len - 1];
    uint16_t next_indent = 0;
    
    int32_t temp_ch = next_char;
    while (temp_ch == ' ' || temp_ch == '\t') {
      next_indent += (temp_ch == ' ') ? 1 : 4;
      // Note: can't advance lexer here, just counting
      if (temp_ch == ' ') temp_ch = ' '; else temp_ch = '\t';
      break; // Just peek one char
    }
    
    bool next_more_indented = (next_char == ' ' || next_char == '\t');
    
    // Emit BLANK_LINE for multiple newlines
    if (newline_count >= 2 && valid_symbols[BLANK_LINE]) {
      lexer->result_symbol = BLANK_LINE;
      return true;
    }
    
    // Emit NEWLINE for single newline, UNLESS next line is indented
    if (newline_count == 1 && valid_symbols[NEWLINE] && !next_more_indented) {
      lexer->result_symbol = NEWLINE;
      return true;
    }
    
    return false;
  }
  
  // EMPHASIS DELIMITERS: allow open/close based on next-char only
  if (lexer->lookahead == '*' || lexer->lookahead == '_') {
    // Check if any emphasis token is valid before scanning
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
    lexer->advance(lexer, false);
    int32_t after_first = lexer->lookahead;
    
    bool has_second = (after_first == delim);
    int32_t next_after_run = after_first;
    
    TSLexer scan = *lexer;
    if (has_second) {
      scan.advance(&scan, false);
      next_after_run = scan.lookahead;
    }
    
    bool next_is_space = is_space_ch(next_after_run) || is_newline(next_after_run) || next_after_run == 0;
    bool next_is_punct = is_punct_ch(next_after_run);
    bool has_close = has_second
      ? has_valid_closing_delim(&scan, delim, 2)
      : has_valid_closing_delim(lexer, delim, 1);
    bool prev_is_space = is_space_ch(s->prev) || is_newline(s->prev) || s->prev == 0;
    bool can_open = !next_is_space && has_close;
    bool can_close = !prev_is_space && (next_is_space || next_is_punct);
    
    // Emit strong if we have 2+ and conditions allow
    if (has_second) {
      if (can_close) {
        if (delim == '*' && valid_symbols[STRONG_CLOSE_STAR]) {
          lexer->advance(lexer, false);
          lexer->mark_end(lexer);
          lexer->result_symbol = STRONG_CLOSE_STAR;
          s->prev = delim;
          return true;
        }
        if (delim == '_' && valid_symbols[STRONG_CLOSE_UNDERSCORE]) {
          lexer->advance(lexer, false);
          lexer->mark_end(lexer);
          lexer->result_symbol = STRONG_CLOSE_UNDERSCORE;
          s->prev = delim;
          return true;
        }
      }
      if (can_open) {
        if (delim == '*' && valid_symbols[STRONG_OPEN_STAR]) {
          lexer->advance(lexer, false);
          lexer->mark_end(lexer);
          lexer->result_symbol = STRONG_OPEN_STAR;
          s->prev = delim;
          return true;
        }
        if (delim == '_' && valid_symbols[STRONG_OPEN_UNDERSCORE]) {
          lexer->advance(lexer, false);
          lexer->mark_end(lexer);
          lexer->result_symbol = STRONG_OPEN_UNDERSCORE;
          s->prev = delim;
          return true;
        }
      }
    }

    lexer->mark_end(lexer);
    if (can_close) {
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
    if (can_open) {
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
    
    // Not markup - emit as raw
    if (valid_symbols[RAW_DELIM]) {
      lexer->result_symbol = RAW_DELIM;
      s->prev = delim;
      return true;
    }
    
    s->prev = delim;
    return false;
  }

  // Update prev for any other character
  if (!is_newline(lexer->lookahead) && lexer->lookahead != 0) {
    s->prev = lexer->lookahead;
  }

  return false;
}
