#include <tree_sitter/parser.h>
#include <wctype.h>
#include <string.h>

enum TokenType {
  EXPRESSION_START,
  EXPRESSION_END,
  CODE_CONTENT
};

void *tree_sitter_markdoc_external_scanner_create() { return NULL; }
void tree_sitter_markdoc_external_scanner_destroy(void *payload) {}
unsigned tree_sitter_markdoc_external_scanner_serialize(void *payload, char *buffer) { return 0; }
void tree_sitter_markdoc_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {}

bool tree_sitter_markdoc_external_scanner_scan(void *payload, TSLexer *lexer,
                                             const bool *valid_symbols) {
  if (valid_symbols[CODE_CONTENT]) {
    // Scan code content until we hit a line that starts with ``` or ~~~
    lexer->mark_end(lexer);
    bool has_content = false;
    
    while (lexer->lookahead != 0) {
      // Check if we're at start of line with fence marker
      if (lexer->get_column(lexer) == 0) {
        // Check for ``` or ~~~
        if (lexer->lookahead == '`') {
          lexer->advance(lexer, false);
          if (lexer->lookahead == '`') {
            lexer->advance(lexer, false);
            if (lexer->lookahead == '`') {
              // Found closing fence, stop here
              break;
            }
          }
          has_content = true;
          continue;
        } else if (lexer->lookahead == '~') {
          lexer->advance(lexer, false);
          if (lexer->lookahead == '~') {
            lexer->advance(lexer, false);
            if (lexer->lookahead == '~') {
              // Found closing fence, stop here
              break;
            }
          }
          has_content = true;
          continue;
        }
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
