# Technical Specification: Fixing All 28 Failing Tests

## Executive Summary

This document provides a complete technical specification for implementing all required changes to make the 28 failing tests pass in the tree-sitter-markdoc parser. The changes require both grammar modifications and external scanner development.

**Estimated Implementation Time**: 4-6 hours  
**Complexity**: High - requires careful precedence management and scanner integration  
**Risk Level**: Medium - changes to core grammar will require extensive conflict resolution

---

## Part 1: Failing Test Analysis

### Category A: Paragraph Architecture (5 tests)
- **Tests**: 1, 2, 26-28 in corpus
- **Issue**: Multiple consecutive lines within a paragraph are being parsed as separate paragraphs
- **Root Cause**: Current grammar distinguishes single newlines (`\n`) but treats them like paragraph separators
- **Expected Behavior**: 
  - Single newline = line continuation within paragraph
  - Blank line (double newline) = paragraph separator
  - Multi-line paragraphs should have multiple `(text)` nodes under one `(paragraph)`

**Example Test Case (Test #1)**:
```
Input:
Line one
Line two
Line three

Expected Tree:
(source_file
  (paragraph
    (text)      # "Line one"
    (text)      # "Line two"
    (text))     # "Line three"
)

Current Output:
(source_file
  (paragraph (text))
  (paragraph (text))
  (paragraph (text))
)
```

### Category B: Inline Expression Errors (10 tests)
- **Tests**: 3, 4, 38-39, 88-97
- **Issue**: Inline expressions like `{{ user.name }}` create ERROR nodes when expressions contain member access
- **Root Cause**: Parser can't distinguish between:
  - Start of line with member expression (`.` being recognized incorrectly)
  - Inline expressions within paragraphs
- **Expected Behavior**: Parse expressions correctly without ERROR nodes
- **Sub-issues**:
  - Simple expressions work: `{{ name }}` ✓
  - Member expressions fail: `{{ user.name }}` ✗ (creates ERROR)
  - Multiple expressions in paragraph fail

**Example Test Case (Test #38)**:
```
Input:
Text {{ user.name }} here.

Expected:
(paragraph
  (text)
  (inline_expression
    (expression
      (member_expression
        (identifier)
        (identifier))))
  (text))

Current:
(paragraph
  (text))
(ERROR
  (expression (identifier)))
  (text)
  (inline_expression
    (expression
      (member_expression ...)))
```

### Category C: Expression Node Types (10 tests)
- **Tests**: 58-62, 88-97
- **Issue**: Tests expect nodes like `binary_expression`, `unary_expression`, `arrow_function`, `subscript_expression`
- **Root Cause**: Current grammar only has `call_expression`, `member_expression`, `array_access`
- **Missing Operators**:
  - Binary: `||`, `&&`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `+`, `-`, `*`, `/`, `%`
  - Unary: `!`, `-`, `+`, `~`
  - Arrow functions: `(params) => expression`

**Example Test Case (Test #61)**:
```
Input:
{{ age > 18 }}
{{ !isDone && count < 10 }}
{{ -value }}

Expected nodes:
- binary_expression (for `>`, `&&`, `<`)
- unary_expression (for `!`, `-`)

Current state:
- Parser doesn't recognize these operators
- Falls back to identifier parsing only
```

### Category D: HTML Block Restructuring (3 tests)
- **Tests**: 6, 7, 9
- **Issue**: HTML blocks are tokenized as single blob; tests expect `html_attr` and `html_content` sub-nodes
- **Root Cause**: Current `html_block` is a single token matching `<tag ...>content</tag>`
- **Expected Structure**:
  ```
  html_block
    - html_attr (attribute_name, attribute_value)
    - html_attr (attribute_name, attribute_value)
    - html_content (text, nested elements)
  ```

**Example Test Case (Test #6)**:
```
Input:
<div class="container">
Content inside div
</div>

Expected:
(html_block
  (html_attr
    (attribute_name)
    (attribute_value (string)))
  (html_content (text)))

Current:
(html_block)  # entire thing is one opaque token
```

### Category E: Nested List Indentation (4 tests)
- **Tests**: 10, 11, 56-57, 84-87
- **Issue**: Nested lists with indentation create ERROR nodes instead of nested structures
- **Root Cause**: Grammar doesn't track indentation levels; treats indented content as top-level
- **Required Solution**: External scanner for indentation tracking

**Example Test Case (Test #10)**:
```
Input:
- Parent item
  - Child item
  - Another child

Expected:
(list
  (list_item
    (list_marker)
    (paragraph (text))
    (list
      (list_item
        (list_marker)
        (paragraph (text)))
      (list_item
        (list_marker)
        (paragraph (text))))))

Current:
(list
  (list_item (list_marker) (paragraph (text))))
(ERROR))
  (text)
  (list ...)
```

### Category F: List Paragraphs (5 tests)
- **Tests**: 27, 33-34, 99-100
- **Issue**: List items should use `list_paragraph` nodes, not `paragraph` nodes
- **Root Cause**: Current `_list_item_content` uses `$.paragraph`
- **Expected**: `list_paragraph` node type for better semantics

### Category G: HTML Attributes in Inline Context (2 tests)
- **Tests**: 7, 45
- **Issue**: Inline HTML like `<img src="..." alt="..." />` needs `html_attr` nodes
- **Expected**: `html_inline` should contain `html_attr` children

### Category H: Advanced Expressions (3 tests)
- **Tests**: 44, 48-49
- **Issue**: Complex nested expressions and special constructs
- **Examples**: Arrow functions in attributes, complex boolean logic

---

## Part 2: Implementation Roadmap

### Phase 1: Newline/Blank Line Distinction (30 minutes)

**Objective**: Fix multi-line paragraph parsing by changing block separators from single newlines to blank lines.

**Root Cause**: In `source_file`, blocks are separated by `repeat1(/\n/)` which treats ANY newline as a separator. This causes each line to become a separate paragraph.

**Key Semantic Change**:
- **Single newline** (`\n`): Soft line break; continues the same paragraph
- **Blank line** (`\n\n+`): Hard break; separates paragraphs/blocks

**Example**:
```markdown
Line one
Line two
Line three

New paragraph
```

**Expected AST**:
```
(source_file
  (paragraph (text) (text) (text))
  (paragraph (text)))
```

**Affected Tests**: 1, 2, 26-28

**Changes Required - MINIMAL AND SURGICAL**:

In `grammar.js`, modify the `source_file` rule (lines 36-52).

**ONLY change line 44**:

**BEFORE** (current line 44):
```javascript
repeat(seq(
  repeat1(/\n/),        // <-- THIS LINE
  choice(
```

**AFTER** (new line 44):
```javascript
repeat(seq(
  /\n\n+/,              // <-- CHANGE TO THIS
  choice(
```

**Why this works**:
1. `repeat1(/\n/)` accepts ANY 1+ newlines between blocks (even single newlines)
2. `/\n\n+/` requires exactly 2+ newlines (a blank line) to separate blocks
3. Single `\n` is no longer a block separator
4. The `paragraph` rule (line 374-381) already supports multi-line: `repeat(seq(/\n/, ...))`
5. Result: consecutive lines stay in the same paragraph until a blank line is encountered

**No other changes needed**. The paragraph rule already handles multi-line content correctly once the block separator changes.

**Benefits**: 
- Fixes tests 1, 2, 26-28 (5 tests)
- Enables proper multi-line paragraph handling
- Foundation for Phase 5 (list_paragraph support)
- No regressions expected (only makes block separation more strict)

---

### Phase 2: External Scanner for Indentation (3 hours)

**Objective**: Track indentation levels for nested lists and code blocks

**File to Create**: `src/scanner.c`

**Scanner Responsibilities**:
1. Maintain indentation stack (spaces; tabs = 4 spaces)
2. Emit tokens:
   - `_INDENT`: When indentation increases on new line
   - `_DEDENT`: When indentation decreases
   - `_NEWLINE`: Single newline
   - `_BLANK_LINE`: 2+ newlines

**Implementation Outline**:
```c
#include <tree_sitter/parser.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

typedef struct {
  uint32_t *indent_stack;
  uint32_t indent_length;
  uint32_t indent_capacity;
} Scanner;

// Scanner state machine:
// 1. On newline, scan leading whitespace
// 2. Compare to top of indent_stack
// 3. If greater: emit _INDENT, push to stack
// 4. If less: emit _DEDENT(s), pop from stack
// 5. If equal: emit _NEWLINE
// 6. Track consecutive newlines for _BLANK_LINE

void *tree_sitter_markdoc_external_scanner_create() {
  Scanner *scanner = malloc(sizeof(Scanner));
  scanner->indent_stack = malloc(sizeof(uint32_t) * 10);
  scanner->indent_length = 0;
  scanner->indent_capacity = 10;
  scanner->indent_stack[0] = 0; // Base level
  return scanner;
}

bool tree_sitter_markdoc_external_scanner_scan(
  void *payload,
  TSLexer *lexer,
  const bool *valid_symbols
) {
  Scanner *scanner = (Scanner *)payload;
  
  // Implementation details:
  // - Track line start positions
  // - Calculate indentation from whitespace
  // - Emit appropriate tokens
  // - Handle edge cases (tabs vs spaces, blank lines)
  
  // Pseudo-code for main logic:
  if (lexer->eof(lexer)) return false;
  
  if (lexer->lookahead == '\n') {
    lexer->advance(lexer, false);
    uint32_t spaces = 0;
    while (lexer->lookahead == ' ') {
      spaces++;
      lexer->advance(lexer, false);
    }
    // ... compare spaces to stack, emit tokens
  }
  
  return true;
}

// Additional required functions:
// - tree_sitter_markdoc_external_scanner_destroy
// - tree_sitter_markdoc_external_scanner_serialize
// - tree_sitter_markdoc_external_scanner_deserialize
```

**Key Algorithms**:

```
IndentLevel = count_leading_spaces(line)

while (IndentLevel < stack.top()):
    emit(DEDENT)
    stack.pop()

if (IndentLevel > stack.top()):
    emit(INDENT)
    stack.push(IndentLevel)
else:
    emit(NEWLINE)
```

**Integration with Grammar**:
```javascript
externals: $ => [
  $._code_content,
  $._indent,
  $._dedent,
  $._newline,
  $._blank_line
]

list: $ => seq(
  $.list_item,
  repeat(seq($._newline, $.list_item)),
  optional($._blank_line)
),

list_item: $ => seq(
  $.list_marker,
  $._list_item_content,
  optional(seq(
    $._indent,
    $.list,           // Nested list
    $._dedent
  ))
)
```

**Benefits**:
- Fixes tests 10, 11, 56-57, 84-87 (nested lists)
- Enables proper indentation-based code blocks

---

### Phase 3: Expression Hierarchy Restructuring (2 hours)

**Objective**: Add complete operator support with correct precedence

**Precedence Table** (higher number = tighter binding):
```
1.  arrow_function      (right assoc)
2.  logical_or          (left assoc)
3.  logical_and         (left assoc)
4.  equality            (left assoc)  ==, !=
5.  relational          (left assoc)  <, >, <=, >=
6.  additive            (left assoc)  +, -
7.  multiplicative      (left assoc)  *, /, %
8.  unary               (right assoc) !, -, +, ~
9.  postfix             (left assoc)  ., (), []
10. primary             (base)
```

**Grammar Implementation**:

```javascript
// Replace old expression rule
expression: $ => $.arrow_or_logical_or,

// New hierarchy
arrow_or_logical_or: $ => prec.right(1, choice(
  $.arrow_function,
  $.logical_or
)),

arrow_function: $ => prec.right(1, seq(
  '(',
  optional(seq(
    $.identifier,
    repeat(seq(',', /[ \t]*/, $.identifier))
  )),
  ')',
  '=>',
  $.expression
)),

logical_or: $ => prec.left(2, seq(
  $.logical_and,
  repeat(seq('||', $.logical_and))
)),

logical_and: $ => prec.left(3, seq(
  $.equality,
  repeat(seq('&&', $.equality))
)),

equality: $ => prec.left(4, seq(
  $.relational,
  repeat(seq(choice('==', '!='), $.relational))
)),

relational: $ => prec.left(5, seq(
  $.additive,
  repeat(seq(choice('<', '>', '<=', '>='), $.additive))
)),

additive: $ => prec.left(6, seq(
  $.multiplicative,
  repeat(seq(choice('+', '-'), $.multiplicative))
)),

multiplicative: $ => prec.left(7, seq(
  $.unary,
  repeat(seq(choice('*', '/', '%'), $.unary))
)),

unary: $ => prec.right(8, choice(
  seq(choice('!', '-', '+', '~'), $.unary),
  $.postfix
)),

postfix: $ => prec.left(9, seq(
  $.primary,
  repeat(choice(
    seq('.', $.identifier),
    seq('(', optional(seq(
      $.expression,
      repeat(seq(',', /[ \t]*/, $.expression))
    )), ')'),
    seq('[', $.expression, ']')
  ))
)),

primary: $ => choice(
  $.call_expression,
  $.member_expression,
  $.subscript_expression,
  $.identifier,
  $.string,
  $.number,
  $.boolean,
  $.array_literal,
  $.object_literal,
  $.variable
)
```

**Node Aliases for Test Compatibility**:
```javascript
// Rename existing rules
call_expression: $ => ... // Already exists, keep as-is
member_expression: $ => ... // Already exists, keep as-is
subscript_expression: $ => $.array_access // Alias for compatibility
binary_expression: $ => ... // Created above as part of hierarchy
unary_expression: $ => ... // Created above as part of hierarchy
array: $ => $.array_literal // Alias
object: $ => $.object_literal // Alias
```

**Conflict Resolution**:
```javascript
conflicts: $ => [
  // ... existing conflicts
  [$.expression, $.arrow_function],
  [$.expression, $.binary_expression],
  [$.expression, $.unary_expression],
  [$.logical_or, $.expression],
  [$.logical_and, $.expression],
  // ... etc for each level
]
```

**Benefits**:
- Fixes tests 58-62 (binary/unary operators)
- Fixes tests 88-97 (call/member/subscript expressions)
- Enables arrow function support (test 44)

---

### Phase 4: HTML Block Restructuring (1 hour)

**Objective**: Parse HTML tags into structured nodes instead of monolithic tokens

**New Rules**:

```javascript
html_block: $ => choice(
  $.html_paired_block,
  $.html_self_closing_block
),

html_paired_block: $ => seq(
  $.html_open_tag,
  optional($.html_content),
  $.html_close_tag
),

html_self_closing_block: $ => $.html_self_closing_tag,

html_open_tag: $ => seq(
  '<',
  field('tag_name', $.html_tag_name),
  repeat($.html_attr),
  '>'
),

html_close_tag: $ => seq(
  '</',
  field('tag_name', $.html_tag_name),
  '>'
),

html_self_closing_tag: $ => seq(
  '<',
  field('tag_name', $.html_tag_name),
  repeat($.html_attr),
  '/>'
),

html_attr: $ => seq(
  field('attribute_name', /[a-zA-Z][a-zA-Z0-9:-]*/),
  optional(seq(
    '=',
    field('attribute_value', choice(
      $.html_quoted_attr_value,
      $.html_unquoted_attr_value
    ))
  ))
),

html_quoted_attr_value: $ => seq(
  '"',
  alias(/[^"\\]*(\\.[^"\\]*)*/,  $.string),
  '"'
),

html_unquoted_attr_value: $ => alias(/[^\s>\/]+/, $.string),

html_tag_name: $ => /[a-zA-Z][a-zA-Z0-9]*/,

html_content: $ => repeat1(choice(
  $.html_text,
  $.html_inline,
  $.markdoc_tag
)),

html_text: $ => token(/[^<]+/),

html_inline: $ => choice(
  // Inline HTML element like <span>...</span>
  seq(
    '<',
    /[a-zA-Z][a-zA-Z0-9]*/,
    repeat($.html_attr),
    '>',
    optional(/[^<]*/),
    '</',
    /[a-zA-Z][a-zA-Z0-9]*/,
    '>'
  ),
  // Self-closing inline like <br />
  seq(
    '<',
    /[a-zA-Z][a-zA-Z0-9]*/,
    repeat($.html_attr),
    '/>'
  )
)
```

**Update `_block` Choice**:
```javascript
_block: $ => choice(
  $.markdoc_tag,
  $.fenced_code_block,
  $.heading,
  $.html_block,
  $.list,
  $.html_comment,
  $.paragraph
)
```

**Benefits**:
- Fixes tests 6, 7, 9 (HTML block structure)
- Enables proper HTML attribute parsing
- Allows HTML content to contain Markdoc tags

---

### Phase 5: List Paragraph Support (30 minutes)

**Objective**: Use `list_paragraph` instead of `paragraph` in list items

**Changes**:

```javascript
list_paragraph: $ => prec.left(1, seq(
  $._inline_first,
  repeat($._inline_content),
  repeat(seq(
    $._newline,
    seq($._inline_first, repeat($._inline_content))
  ))
)),

_list_item_content: $ => choice(
  $.list_paragraph,  // Changed from $.paragraph
  $.list,
  $.fenced_code_block,
  $.html_block
)
```

**Benefits**:
- Fixes tests 27, 33-34, 99-100
- Distinguishes list items from document paragraphs semantically

---

### Phase 6: Comment Block Support (30 minutes)

**Objective**: Parse Markdoc comment blocks

**Add Rule**:

```javascript
comment_block: $ => seq(
  token('{% comment %}'),
  repeat(choice(
    token(/[^%]+/),
    token(/%(?!.*\/comment)/)
  )),
  token('{% /comment %}')
),

_block: $ => choice(
  $.markdoc_tag,
  $.comment_block,  // Add before paragraph
  $.fenced_code_block,
  $.heading,
  $.html_block,
  $.list,
  $.html_comment,
  $.paragraph
)
```

**Note**: Tree Sitter doesn't support lookahead, so this is tricky. Alternative: tokenize entire block including content.

**Benefits**:
- Fixes test 8 (Markdoc comment block)
- Provides user-facing comment capability

---

## Part 3: Implementation Checklist

### Pre-Implementation
- [ ] Backup current grammar.js and working tests
- [ ] Create feature branch (or work on current branch per user request)
- [ ] Read Tree Sitter documentation on external scanners
- [ ] Review conflicting grammar patterns

### Scanner Development (src/scanner.c)
- [ ] Set up C project structure with tree-sitter headers
- [ ] Implement indentation stack management
- [ ] Write indent/dedent token emission logic
- [ ] Handle edge cases (tabs, mixed indentation, EOF)
- [ ] Test scanner in isolation
- [ ] Compile and verify no warnings

### Grammar Modification (grammar.js)
- [ ] Add externals for scanner tokens
- [ ] Restructure expression hierarchy with correct precedence
- [ ] Add HTML parsing rules
- [ ] Add list_paragraph rule
- [ ] Update conflicts array as needed
- [ ] Add comment_block support
- [ ] Verify no new grammar syntax errors

### Testing & Iteration
- [ ] Run `pnpm gen` after each major section
- [ ] Run `pnpm test` after each section
- [ ] Track test progression (should see 28 → ~25 → ~20 → ... → 0)
- [ ] Fix regressions as they appear
- [ ] Document breaking points for recovery

### Validation
- [ ] All 100 tests passing
- [ ] No ERROR nodes in any parse tree
- [ ] Run linter if available
- [ ] Manual review of complex test outputs
- [ ] Performance check (parse time reasonable)

### Commit
- [ ] Single atomic commit with message:
  ```
  refactor(grammar): major overhaul for all expression types and parsing architecture
  ```
- [ ] Include both grammar.js and src/scanner.c changes

---

## Part 4: Testing Strategy

### Test Verification During Implementation

**Phase 1 Results** (After newline distinction):
- ✓ Tests 1-2, 26-28 should pass (paragraph multi-line)
- ~ Tests 3-4 might still have inline expression issues
- No regressions expected

**Phase 2 Results** (After scanner):
- ✓ Tests 10-11, 56-57, 84-87 should pass (nested lists)
- May introduce conflicts, be prepared to adjust

**Phase 3 Results** (After expression hierarchy):
- ✓ Tests 58-62, 88-97 should pass (operators and expressions)
- ✓ Tests 38-39 might pass (member expressions)
- Likely to regress some tests due to precedence conflicts
- Requires extensive conflict resolution

**Phase 4 Results** (After HTML restructuring):
- ✓ Tests 6-7, 9 should pass (HTML blocks)
- ✓ Tests 44, 48-49 might pass (complex expressions)
- No major regressions expected

**Phase 5 & 6** (After list_paragraph and comments):
- ✓ Tests 27, 33-34, 99-100 should pass
- ✓ Test 8 should pass (if comment_block implemented)

### Regression Recovery

If a phase introduces regressions:

1. **Identify**: Which tests broke that previously passed?
2. **Analyze**: What grammar change caused it?
3. **Options**:
   - Add conflicts (if ambiguity)
   - Adjust precedence numbers
   - Reorder choice alternatives
   - Revert and try different approach

---

## Part 5: Known Challenges & Solutions

### Challenge 1: Precedence Conflicts
**Problem**: Adding binary/unary operators creates shift/reduce conflicts  
**Solution**: 
- Use `prec.left()` and `prec.right()` consistently
- Add explicit conflicts array entries
- May need to adjust precedence numbers empirically

### Challenge 2: Inline Expression Parsing
**Problem**: `{{ member.expr }}` at start of line confuses parser  
**Solution**:
- Ensure `inline_expression` is higher priority in `_inline_first`
- Use token precedence: `token(prec(5, '{{'))`
- Consider lookahead alternatives (though Tree Sitter doesn't support)

### Challenge 3: Scanner Integration
**Problem**: C code can be error-prone, subtle bugs in indentation logic  
**Solution**:
- Start with simple implementation
- Add test cases to verify indent/dedent emission
- Use defensive programming (bounds checks, null checks)
- Log intermediate state for debugging

### Challenge 4: HTML Parsing Ambiguity
**Problem**: `<` could start HTML or be comparison operator  
**Solution**:
- Prioritize HTML blocks in `_block` (parsed at statement level, not expression)
- Use `<` operator only inside `{{ }}` (expression context)
- No ambiguity if these contexts are separate

### Challenge 5: Comment Block Tokenization
**Problem**: Can't use lookahead to find `{% /comment %}`  
**Solution**: Two options:
- Option A: Tokenize entire block in C scanner
- Option B: Use liberal token matching and rely on grammar structure

---

## Part 6: Code Organization

### File Structure After Implementation
```
tree-sitter-markdoc/
├── grammar.js           # (modified) Core grammar with new rules
├── src/
│   ├── scanner.c        # (NEW) External scanner for indentation
│   ├── grammar.json     # (auto-generated)
│   ├── node-types.json  # (auto-generated)
│   └── parser.c         # (auto-generated)
├── binding.gyp          # May need updates for scanner.c compilation
└── test/
    └── corpus/          # All test files
```

### Building with Scanner

The `binding.gyp` file likely already includes compilation flags for C code:
```python
{
  'targets': [
    {
      'target_name': 'tree_sitter_markdoc_binding',
      'sources': [
        'src/parser.c',
        'src/scanner.c'        # Add this line if not present
      ]
    }
  ]
}
```

---

## Part 7: Success Criteria

**Primary Goal**: All 100 tests passing
- [ ] `pnpm test` shows 0 failures
- [ ] No ERROR nodes in any parse tree
- [ ] All node types match corpus expectations exactly

**Secondary Goals**:
- [ ] Code compiles without warnings
- [ ] Parser generates in < 10 seconds
- [ ] Single clean commit
- [ ] Comprehensive inline comments explaining complex precedence

**Performance Benchmarks** (optional):
- Parse time should be reasonable (< 100ms for typical 10KB file)
- Memory usage should not spike

---

## Part 8: References & Resources

### Key Documentation
- [Tree Sitter Grammar Documentation](https://tree-sitter.github.io/tree-sitter/creating-parsers)
- [Tree Sitter External Scanners](https://tree-sitter.github.io/tree-sitter/creating-parsers#external-scanners)
- [Precedence in Tree Sitter](https://tree-sitter.github.io/tree-sitter/grammar-dsl#precedence)

### Similar Implementations
- `tree-sitter-python`: Indentation-sensitive parser with external scanner
- `tree-sitter-go`: Complex expression hierarchies
- `tree-sitter-javascript`: Arrow functions and operators

### Debugging Tips
```bash
# Generate verbose output
tree-sitter parse --debug file.md

# Check grammar syntax
tree-sitter generate

# Run specific test
tree-sitter test --filter "test name"

# Check conflicts
tree-sitter generate 2>&1 | grep -i conflict
```

---

## Conclusion

This specification provides a complete roadmap for fixing all 28 failing tests. The implementation requires:

1. **Understanding** the current grammar limitations
2. **Developing** an external scanner for indentation tracking
3. **Restructuring** the expression grammar carefully to manage precedence
4. **Testing** incrementally to catch regressions early
5. **Committing** atomically once all tests pass

The work is achievable in 4-6 hours with this structured approach. Key success factors are:
- Managing conflicts through careful precedence assignment
- Testing after each major section
- Keeping scanner logic simple and well-commented
- Leveraging Tree Sitter's conflict resolution tools

Good luck with the implementation!
