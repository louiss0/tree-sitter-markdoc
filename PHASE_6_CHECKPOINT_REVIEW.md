# Phase 6 Checkpoint Review

**Status:** ✅ Scaffolding Phase Complete  
**Commit:** `f6c1bf2` - feat(scanner): scaffold external scanner with NEWLINE BLANK_LINE  
**Branch:** `feature/markdoc-1.0.0`  

---

## Quick Summary

All foundational scaffolding for Phase 6 (Option A) is complete:

- ✅ Grammar: Added `$._NEWLINE` and `$._BLANK_LINE` external tokens
- ✅ Scanner: Enhanced state machine with 3 bool flags (bitfield serialized)
- ✅ Token Enum: Added NEWLINE (=1) and BLANK_LINE (=2) token types
- ✅ Scanning Logic: NEWLINE, BLANK_LINE detection + CODE_CONTENT gating
- ✅ Builds: Parser regenerates cleanly, no new conflicts
- ✅ Tests: 67/200 passing (no regression from scaffolding)

---

## Key Changes

### Grammar (grammar.js)
```javascript
externals: $ => [
  $._code_content,
  $._NEWLINE,          // NEW
  $._BLANK_LINE        // NEW
],
```

### Scanner (src/scanner.c)

**State:**
```c
typedef struct {
  bool in_code_block;      // Existing
  bool in_fenced_code;     // NEW - for context awareness
  bool in_indented_code;   // NEW - for context awareness
} Scanner;
```

**Tokens:**
- `NEWLINE` (= 1): Emits on `\n` or `\r\n` outside code blocks
- `BLANK_LINE` (= 2): Emits on lines with only whitespace then newline
- `CODE_CONTENT` (= 0): Now gated to only emit inside code blocks

**Key Helpers:**
- `is_newline(ch)`: Detects `\n` or `\r`
- `is_space_char(ch)`: Detects ` ` or `\t`

---

## What's NOT Wired Yet (Expected in Phase 6B)

- ❌ Grammar rules don't use `$._NEWLINE` yet (still use `/\n/`)
- ❌ Grammar rules don't use `$._BLANK_LINE` yet
- ❌ Inline expression lookahead for `{{`, `{%`, `{#` not added
- ❌ State flags `in_fenced_code` / `in_indented_code` not updated

**This explains why tests haven't improved:** Tokens are scanned but not referenced by grammar.

---

## Code Quality Assessment

### Strengths
✅ Bitfield serialization (1 byte = 3 flags) - standard and efficient  
✅ CRLF handling - both `\r\n` and `\n` supported  
✅ Valid symbols gating - prevents parser conflicts  
✅ Follows tree-sitter patterns and conventions  

### Areas to Watch
⚠️ BLANK_LINE column check (`start_col == 0`) - may need validation during grammar wiring  
⚠️ State flags exist but unused - will be activated in Phase 6B  
⚠️ Inline expression lookahead incomplete - scheduled for Phase 6C  

---

## Test Expectation Path

```
Phase 6A (Current):  67/200  ← Scaffolding complete
Phase 6B (Grammar):  72/200  ← Wire NEWLINE/BLANK_LINE → +5 tests
Phase 6C (Inline):   76/200  ← Add expr lookahead → +4 tests
Phase 6D (Polish):   77/200  ← Edge cases → +1 test
Goal:               ≥75/100 (reached by Phase 6C)
```

---

## Validation Checklist

Before Phase 6B, confirm:

- [x] Tests at 67/200 (no regression)
- [x] Parser builds cleanly
- [x] No new grammar conflicts
- [ ] Serialization works (roundtrip save/restore)
- [ ] Scanner returns false for edge cases

---

## Approval Questions

1. **Bitfield serialization:** Acceptable approach for state?
2. **BLANK_LINE logic:** Column check safe, or use different approach?
3. **Timeline:** Continue to Phase 6B now?

---

**Ready to proceed with Grammar Wiring (Phase 6B)?**


---

## Phase 6B Attempt: Grammar Wiring (ecceb96)

### What Was Done

Executed full grammar rewrite to replace all `/\n/` regex patterns with `$._NEWLINE` tokens:
- 16 instances of `/\n/` replaced across grammar rules
- Fixed yaml_content regex pattern that included `\n` terminator  
- Parser regenerated cleanly with no new conflicts
- Build succeeds

### Test Results

**Regression:** 67/200 → 55/200 tests passing (-12 tests)

### Root Cause Analysis

The grammar now requests `$._NEWLINE` tokens throughout, but the parser isn't receiving them from the scanner. Likely causes:

1. **Index Mismatch:** Scanner's `valid_symbols[NEWLINE]` may not align with parser's expected index
   - Scanner enum: `CODE_CONTENT=0, NEWLINE=1, BLANK_LINE=2`
   - Parser likely passes different indices based on its internal symbol table

2. **Parser State Issue:** `valid_symbols[NEWLINE]` may never be true during parse
   - Tree-sitter's LR parser sets `valid_symbols` based on parse states
   - Grammar rewrite changed parse states; NEWLINE may not be requested

3. **Gating Logic:** Scanner checks `!scanner->in_code_block` before emitting
   - Initially `in_code_block` is false, so condition should pass
   - But `in_code_block` is never updated (no state transitions implemented yet)

### Recommended Fix Path

**Option 1 (Correct):** Use parser-agnostic lookahead
```c
// Instead of: if (valid_symbols[NEWLINE])
// Try: if (lexer->lookahead is newline) { emit token}
// Let parser decide if it wants the token via backtracking
```

**Option 2 (Debug):** Add diagnostic logging
```c
fprintf(stderr, "Scanner: lookahead=%d, valid_symbols[0..3]={%d,%d,%d,%d}\n",
  lexer->lookahead, valid_symbols[0], valid_symbols[1], valid_symbols[2], valid_symbols[3]);
```

**Option 3 (Safer Path):** Revert to Option B approach
- Undo grammar rewrite (git revert ecceb96)
- Keep `parser.js` using `/\n/` regex
- Only implement CODE_CONTENT gating (Option A core fix)
- Fix inline expression conflicts without full NEWLINE refactor

### Lessons Learned

1. **Tree-sitter External Scanners:** Require precise `valid_symbols` alignment with parser's symbol table
2. **Wholesale Grammar Changes:** Risky due to parse state redistribution; incremental changes safer
3. **Testing Complexity:** Simple test count doesn't indicate parse tree correctness

---

## Recommendation

**Choose One:**
1. **Deep Dive (4+ hours):** Debug scanner/parser alignment, get NEWLINE tokens flowing
2. **Strategic Pivot (2 hours):** Revert, implement CODE_CONTENT fix only (Option B), achieve quick wins
3. **Hybrid (3 hours):** Revert, implement CODE_CONTENT + selective NEWLINE wiring (Phase rules only)

---

**Current Status:** 2 commits toward Phase 6, infrastructure solid but integration incomplete.

