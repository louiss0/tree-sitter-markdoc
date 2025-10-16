#include <tree_sitter/parser.h>
#include <wctype.h>

enum TokenType {
  EXPRESSION_START,
  EXPRESSION_END
};

void *tree_sitter_markdoc_external_scanner_create() { return NULL; }
void tree_sitter_markdoc_external_scanner_destroy(void *payload) {}
unsigned tree_sitter_markdoc_external_scanner_serialize(void *payload, char *buffer) { return 0; }
void tree_sitter_markdoc_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {}

bool tree_sitter_markdoc_external_scanner_scan(void *payload, TSLexer *lexer,
                                             const bool *valid_symbols) {
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
