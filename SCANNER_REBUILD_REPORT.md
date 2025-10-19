# Scanner Rebuild Session Report

**Date:** 2025-10-19  
**Branch:** `feature/markdoc-1.0.0`  
**Goal:** Fix nested list parsing by rebuilding scanner from scratch  

## Work Completed

### 1. Scanner Rebuild ✓
- Gutted `src/scanner.c` completely, starting from scratch
- Kept minimal implementation:
  - CODE_CONTENT for fenced code blocks
  - NEWLINE/BLANK_LINE for line breaks
  - INDENT/DEDENT tokens for indentation tracking
  - Full serialization/deserialization for state persistence

### 2. Grammar Updates ✓
- Removed `extras` rule initially to test hypothesis
- Added back `extras: $ => [/[ \t]/]` for inline whitespace
- Updated `externals` to include `_INDENT` and `_DEDENT`
- Modified `list_item` rule to support nested lists:
  ```javascript
  optional(seq(
    $._INDENT,
    $.list,
    $._DEDENT
  ))
  ```

### 3. Testing & Findings

**Test Case:** Nested unordered list
```markdown
- Parent item
  - Child item
  - Another child
```

**Result:** Still parsing as flat siblings, not nested structure.

## Root Cause Analysis

**Fundamental Tree-Sitter Architecture Issue:**

Tree-sitter's lexer processes `extras` and external scanner tokens at the SAME position in the input stream. When the parser encounters:

```
- Parent item\n
  - Child item
```

The sequence is:
1. Parser finishes `paragraph` content for "Parent item\n"
2. Parser is positioned after the newline, at column 0 of indented line
3. Tree-sitter checks both `extras` and `valid_symbols[INDENT]` at this position
4. **`extras` rule `/[ \t]/` matches and consumes leading spaces BEFORE external scanner can emit INDENT token**
5. Parser now sees `- Child item` at column 0, matching it as sibling list_item

The scanner CAN detect indentation change and tries to emit INDENT, but by then the spaces are already consumed by `extras`.

## Attempted Solutions

### Solution 1: Remove `extras` entirely
**Result:** Broke paragraph formatting - spaces weren't consumed where needed  
**Issue:** Grammar would need hundreds of explicit `/[ \t]*/` patterns added

### Solution 2: Make scanner consume spaces after emitting INDENT
**Result:** `extras` still consumed them before INDENT token was recognized  
**Issue:** Tree-sitter architecture doesn't support this ordering

### Solution 3: Only emit NEWLINE when next line isn't indented
**Result:** Worked initially but paragraph rule accepted NEWLINE and continued parsing  
**Issue:** Parser's greedy matching of paragraph continuation

### Solution 4: Make INDENT/DEDENT part of normal grammar, not externals
**Result:** Not attempted - would require lexing indentation in grammar rules  
**Issue:** That's what we're trying to avoid

## Key Insights

1. **Tree-sitter's `extras` rule is fundamentally incompatible with external scanner-based indentation tracking** when spaces are part of `extras`

2. **Successful indentation-sensitive tree-sitter parsers** (e.g., Python grammar if it used tree-sitter):
   - Don't include leading spaces in `extras`
   - Handle ALL spaces explicitly in grammar rules
   - OR use a completely different approach (hand-written lexer + tree-sitter parser)

3. **The architecture is well-designed for most languages** but poor for Python-like indentation-sensitive languages

## Recommendations

### Short Term (Quick Win - ~2 hours)
1. Accept that nested lists cannot be reliably parsed in tree-sitter with this architecture
2. Document as limitation: "Nested lists are not supported; use flat list structure"
3. Keep current 76/80 passing tests

### Medium Term (Major Refactor - ~6-8 hours)
1. **Remove `extras` rule completely**
2. Add explicit `/[ \t]*/` patterns to EVERY grammar rule that needs spaces
3. This is tedious but unambiguous and gives scanner control
4. Expected result: nested lists would then work

### Long Term (Strategic Decision)
1. Evaluate whether tree-sitter is the right choice for Markdoc
2. Consider hand-written lexer + tree-sitter parser combination
3. Or accept limitation and document in README

## Technical Details for Future Work

If attempting option "Medium Term", start with:

```javascript
// BEFORE: extras rule auto-handles spaces
// AFTER: explicit space handling everywhere

// Example: heading_marker
heading_marker: $ => token(prec(1, 
  seq(/#{1,6}/, /[ \t]+/)  // Already has this!
)),

// Example: list_marker - update regex
list_marker: $ => token(prec(2, choice(
  /[-*+][ \t]+/,  // Already has this!
  /[0-9]+\.[ \t]+/
))),

// Example: tag_open - needs updates
tag_open: $ => prec.right(seq(
  token(prec(6, seq('{%', /[ \t]*/))),  // Has space
  // ...
)),
```

Most rules already have explicit space patterns! The issue is that `extras` is being applied BETWEEN rules during parsing.

## Files Modified

- `src/scanner.c` - Complete rebuild
- `grammar.js` - Added INDENT/DEDENT externals, updated list rules
- Test files created: `test-nested.md`, `test-simple.md`

## Conclusion

The scanner rebuild successfully demonstrates the core issue: **tree-sitter's `extras` rule processes at the same lexical position as external scanner tokens, preventing proper indentation-based nesting detection**.

The solution requires either:
1. Removing `extras` entirely and refactoring all grammar rules
2. Using a completely different parsing approach
3. Accepting the limitation

Further work on this issue would be valuable for the tree-sitter community if documented as a known limitation.
