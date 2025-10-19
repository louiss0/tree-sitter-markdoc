#include <tree_sitter/parser.h>
#include <stdlib.h>
#include <string.h>

enum TokenType {
  CODE_CONTENT,
  NEWLINE,
  BLANK_LINE,
  INDENT,
  DEDENT
};

typedef struct {
  uint16_t indent_stack[64];
  uint8_t indent_len;
  bool in_fence;
  char fence_char;
  uint8_t fence_len;
} Scanner;

void *tree_sitter_markdoc_external_scanner_create() {
  Scanner *scanner = (Scanner *)malloc(sizeof(Scanner));
  scanner->indent_len = 1;
  scanner->indent_stack[0] = 0;
  scanner->in_fence = false;
  scanner->fence_char = 0;
  scanner->fence_len = 0;
  return scanner;
}

void tree_sitter_markdoc_external_scanner_destroy(void *payload) {
  free(payload);
}

unsigned tree_sitter_markdoc_external_scanner_serialize(void *payload, char *buffer) {
  Scanner *s = (Scanner *)payload;
  unsigned i = 0;
  unsigned char flags = s->in_fence ? 1 : 0;
  buffer[i++] = flags;
  buffer[i++] = s->fence_char;
  buffer[i++] = s->fence_len;
  buffer[i++] = s->indent_len;
  for (uint8_t j = 0; j < s->indent_len && i + 1 < 255; j++) {
    uint16_t v = s->indent_stack[j];
    buffer[i++] = (char)(v & 0xFF);
    buffer[i++] = (char)((v >> 8) & 0xFF);
  }
  return i;
}

void tree_sitter_markdoc_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  Scanner *s = (Scanner *)payload;
  s->indent_len = 1;
  s->indent_stack[0] = 0;
  s->in_fence = false;
  s->fence_char = 0;
  s->fence_len = 0;
  
  if (length < 4) return;
  
  unsigned i = 0;
  unsigned char flags = (unsigned char)buffer[i++];
  s->in_fence = (flags & 1) != 0;
  s->fence_char = buffer[i++];
  s->fence_len = (uint8_t)buffer[i++];
  s->indent_len = (uint8_t)buffer[i++];
  if (s->indent_len == 0) s->indent_len = 1;
  
  for (uint8_t j = 0; j < s->indent_len && i + 1 <= length && j < 64; j++) {
    uint16_t v = (uint8_t)buffer[i++] | (((uint16_t)(uint8_t)buffer[i++]) << 8);
    s->indent_stack[j] = v;
  }
}

static inline bool is_newline(int32_t ch) {
  return ch == '\n' || ch == '\r';
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
  
  // CODE_CONTENT: handle fenced code blocks
  if (valid_symbols[CODE_CONTENT]) {
    if (!s->in_fence && lexer->get_column(lexer) == 0) {
      // Check if we're starting a code fence
      if (lexer->lookahead == '`' || lexer->lookahead == '~') {
        char fence_char = lexer->lookahead;
        unsigned count = 0;
        
        // Peek ahead to count fence characters
        int32_t ch = lexer->lookahead;
        while (ch == fence_char) {
          count++;
          lexer->advance(lexer, false);
          ch = lexer->lookahead;
        }
        
        if (count >= 3) {
          s->in_fence = true;
          s->fence_char = fence_char;
          s->fence_len = count;
          return false;
        }
      }
    }
    
    // Inside a fence: consume until closing fence
    if (s->in_fence) {
      lexer->mark_end(lexer);
      bool has_content = false;
      bool at_line_start = (lexer->get_column(lexer) == 0);
      
      while (lexer->lookahead != 0) {
        // Check for closing fence at start of line
        if (at_line_start) {
          if (lexer->lookahead == s->fence_char) {
            unsigned count = 0;
            while (lexer->lookahead == s->fence_char) {
              lexer->advance(lexer, false);
              count++;
              if (count >= s->fence_len) break;
            }
            if (count >= s->fence_len) {
              // Found closing fence
              s->in_fence = false;
              s->fence_char = 0;
              s->fence_len = 0;
              break;
            }
            has_content = true;
            at_line_start = false;
            continue;
          }
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
    }
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
  
  return false;
}
