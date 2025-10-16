#include <tree_sitter/parser.h>
#include <wctype.h>
#include <string.h>
#include <stdlib.h>

enum TokenType {
  EXPRESSION_START,
  EXPRESSION_END,
  CODE_CONTENT
};

typedef struct {
  bool in_code_block;
} Scanner;

void *tree_sitter_markdoc_external_scanner_create() {
  Scanner *scanner = (Scanner *)malloc(sizeof(Scanner));
  scanner->in_code_block = false;
  return scanner;
}

void tree_sitter_markdoc_external_scanner_destroy(void *payload) {
  Scanner *scanner = (Scanner *)payload;
  free(scanner);
}

unsigned tree_sitter_markdoc_external_scanner_serialize(void *payload, char *buffer) {
  Scanner *scanner = (Scanner *)payload;
  buffer[0] = scanner->in_code_block ? 1 : 0;
  return 1;
}

void tree_sitter_markdoc_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  Scanner *scanner = (Scanner *)payload;
  if (length > 0) {
    scanner->in_code_block = buffer[0] == 1;
  } else {
    scanner->in_code_block = false;
  }
}

bool tree_sitter_markdoc_external_scanner_scan(void *payload, TSLexer *lexer,
                                             const bool *valid_symbols) {
  Scanner *scanner = (Scanner *)payload;
  
  // CODE_CONTENT: Consume all content until we hit a closing fence
  // The grammar ensures this is only called between code fences
  if (valid_symbols[CODE_CONTENT]) {
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

  if (valid_symbols[EXPRESSION_START]) {
    // Fail fast if first char isn't {
    if (lexer->lookahead != '{') {
      return false;
    }

    lexer->advance(lexer, false);
    // Fail fast if second char isn't {
    if (lexer->lookahead != '{') {
      return false;
    }

    lexer->advance(lexer, false);
    lexer->result_symbol = EXPRESSION_START;
    return true;
  }

  if (valid_symbols[EXPRESSION_END]) {
    // Fail fast if first char isn't }
    if (lexer->lookahead != '}') {
      return false;
    }

    lexer->advance(lexer, false);
    // Fail fast if second char isn't }
    if (lexer->lookahead != '}') {
      return false;
    }

    lexer->advance(lexer, false);
    lexer->result_symbol = EXPRESSION_END;
    return true;
  }

  return false;
}
