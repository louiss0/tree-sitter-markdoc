#include <tree_sitter/parser.h>
#include <wctype.h>
#include <string.h>
#include <stdlib.h>

enum TokenType {
  CODE_CONTENT,   // = 0
  NEWLINE,        // = 1
  BLANK_LINE      // = 2
};

// Helper: Check if character is a newline (LF or CR)
static inline bool is_newline(int32_t ch) {
  return ch == '\n' || ch == '\r';
}

// Helper: Check if character is whitespace (space or tab only, not newline)
static inline bool is_space_char(int32_t ch) {
  return ch == ' ' || ch == '\t';
}

// Helper: Check if we're at the start of an inline expression pattern
static inline bool is_inline_expression_start(int32_t ch) {
  return ch == '{';
}

typedef struct {
  bool in_code_block;      // Currently inside any code block
  bool in_fenced_code;     // Currently inside fenced code (``` or ~~~)
  bool in_indented_code;   // Currently inside indented code (4 spaces)
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

// Pack 3 bools into 1 byte: bit0=in_code_block, bit1=in_fenced_code, bit2=in_indented_code
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

// Helper: Check if character starts a block-level element for NEWLINE (paragraph termination)
// This includes list markers because paragraphs should not continue into list items
static inline bool is_paragraph_terminator(int32_t ch) {
  return ch == '#' ||   // Heading
         ch == '-' ||   // List marker
         ch == '*' ||   // List marker
         ch == '+' ||   // List marker
         ch == '_' ||   // Thematic break
         ch == '>' ||   // Blockquote
         ch == '`' ||   // Code fence
         ch == '~' ||   // Code fence
         ch == '<' ||   // HTML
         ch == '{' ||   // Markdoc tag or expression
         (ch >= '0' && ch <= '9');  // Ordered list
}

// Helper: Check if character starts a major block (for BLANK_LINE emission)
// Excludes list markers - consecutive list items don't need blank lines
static inline bool is_major_block_marker(int32_t ch) {
  return ch == '#' ||   // Heading
         ch == '>' ||   // Blockquote  
         ch == '`' ||   // Code fence
         ch == '~' ||   // Code fence
         ch == '<' ||   // HTML
         ch == '{';     // Markdoc tag or expression
}

bool tree_sitter_markdoc_external_scanner_scan(void *payload, TSLexer *lexer,
                                             const bool *valid_symbols) {
  Scanner *scanner = (Scanner *)payload;

  // Process newlines only outside code blocks
  if (!scanner->in_code_block && is_newline(lexer->lookahead)) {
    // Count consecutive newlines
    int newline_count = 0;
    while (is_newline(lexer->lookahead)) {
      if (lexer->lookahead == '\r') {
        lexer->advance(lexer, false);
      }
      if (lexer->lookahead == '\n') {
        lexer->advance(lexer, false);
        newline_count++;
      }
    }
    
    lexer->mark_end(lexer);  // Mark end after consuming newlines
    
    // Look ahead past whitespace
    int32_t next_char = lexer->lookahead;
    while (is_space_char(next_char)) {
      lexer->advance(lexer, false);
      next_char = lexer->lookahead;
    }
    
    // Check if we're at EOF
    if (next_char == 0) {
      // At EOF - don't emit tokens for trailing newlines
      // Let the grammar's repeat(/\n/) handle them
      return false;
    }
    
    // Check what type of boundary we're at
    bool at_major_block = is_major_block_marker(next_char);
    bool at_paragraph_end = is_paragraph_terminator(next_char);
    
    // BLANK_LINE: Emit for true blank lines OR major block boundaries
    // (but NOT for list markers - lists use single newlines)
    if (valid_symbols[BLANK_LINE] && (newline_count >= 2 || at_major_block)) {
      lexer->result_symbol = BLANK_LINE;
      return true;
    }
    
    // NEWLINE: Emit for line continuation (single newline, not at paragraph terminator)
    if (valid_symbols[NEWLINE] && newline_count == 1 && !at_paragraph_end) {
      lexer->result_symbol = NEWLINE;
      return true;
    }
  }

  // CODE_CONTENT: Consume all content until we hit a closing fence
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

  return false;
}
