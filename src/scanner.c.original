#include <tree_sitter/parser.h>
#include <wctype.h>
#include <string.h>
#include <stdlib.h>

enum TokenType {
  CODE_CONTENT,
  NEWLINE,
  BLANK_LINE
};

typedef struct {
  bool in_code_block;
  bool in_fenced_code;
  bool in_indented_code;
} Scanner;

void *tree_sitter_markdoc_external_scanner_create() {
  Scanner *scanner = (Scanner *)malloc(sizeof(Scanner));
  scanner->in_code_block = false;
  scanner->in_fenced_code = false;
  scanner->in_indented_code = false;
  return scanner;
}

void tree_sitter_markdoc_external_scanner_destroy(void *payload) {
  Scanner *scanner = (Scanner *)payload;
  free(scanner);
}

unsigned tree_sitter_markdoc_external_scanner_serialize(void *payload, char *buffer) {
  Scanner *scanner = (Scanner *)payload;
  buffer[0] = (scanner->in_code_block ? 1 : 0) |
              (scanner->in_fenced_code ? 2 : 0) |
              (scanner->in_indented_code ? 4 : 0);
  return 1;
}

void tree_sitter_markdoc_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  Scanner *scanner = (Scanner *)payload;
  if (length > 0) {
    scanner->in_code_block = (buffer[0] & 1) != 0;
    scanner->in_fenced_code = (buffer[0] & 2) != 0;
    scanner->in_indented_code = (buffer[0] & 4) != 0;
  } else {
    scanner->in_code_block = false;
    scanner->in_fenced_code = false;
    scanner->in_indented_code = false;
  }
}

static bool is_newline(int32_t ch) {
  return ch == '\n' || ch == '\r';
}

static bool is_space_char(int32_t ch) {
  return ch == ' ' || ch == '\t';
}

bool tree_sitter_markdoc_external_scanner_scan(void *payload, TSLexer *lexer,
                                             const bool *valid_symbols) {
  Scanner *scanner = (Scanner *)payload;
  
  // Handle NEWLINE token
  if (valid_symbols[NEWLINE] && !scanner->in_code_block) {
    if (is_newline(lexer->lookahead)) {
      // Advance past CR if present
      if (lexer->lookahead == '\r') {
        lexer->advance(lexer, false);
      }
      // Consume LF
      if (lexer->lookahead == '\n') {
        lexer->advance(lexer, true);
        lexer->result_symbol = NEWLINE;
        return true;
      }
    }
  }
  
  // Handle BLANK_LINE token
  if (valid_symbols[BLANK_LINE] && !scanner->in_code_block) {
    // Save position
    int32_t start_col = lexer->get_column(lexer);
    
    // Check if we're at line start
    if (start_col == 0) {
      // Scan through spaces/tabs only
      while (is_space_char(lexer->lookahead)) {
        lexer->advance(lexer, false);
      }
      
      // Check if we hit a newline
      if (is_newline(lexer->lookahead)) {
        // Advance past CR if present
        if (lexer->lookahead == '\r') {
          lexer->advance(lexer, false);
        }
        // Consume LF
        if (lexer->lookahead == '\n') {
          lexer->advance(lexer, true);
          lexer->result_symbol = BLANK_LINE;
          return true;
        }
      }
    }
  }
  
  // CODE_CONTENT: Consume all content until we hit a closing fence
  // The grammar ensures this is only called between code fences
  if (valid_symbols[CODE_CONTENT]) {
    // Only emit CODE_CONTENT when inside code blocks
    if (!scanner->in_code_block) {
      return false;
    }
    
    lexer->mark_end(lexer);
    bool has_content = false;
    bool at_line_start = true;
    
    while (lexer->lookahead != 0) {
      // Track if we're at the start of a line
      if (at_line_start && lexer->get_column(lexer) == 0) {
        // Check for closing fence: ``` or ~~~
        if (lexer->lookahead == '`' || lexer->lookahead == '~') {
          char fence_char = lexer->lookahead;
          lexer->advance(lexer, false);
          
          if (lexer->lookahead == fence_char) {
            lexer->advance(lexer, false);
            if (lexer->lookahead == fence_char) {
              // Found closing fence! Stop here without consuming it
              break;
            }
          }
          // False alarm - not a fence, mark as content and continue
          has_content = true;
          at_line_start = false;
          continue;
        }
      }
      
      // Track newlines
      if (lexer->lookahead == '\n') {
        at_line_start = true;
      } else if (lexer->lookahead != ' ' && lexer->lookahead != '\t' && 
                 lexer->lookahead != '\r') {
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

  return false;
}
