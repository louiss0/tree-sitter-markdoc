# Phase 6: Test-Driven Fix for 66/100→75+

**Status**: Active  
**Baseline**: 66/100 tests passing (from commit c7ba0d4)  
**Goal**: Reach 75/100 passing tests through targeted grammar fixes

## Investigation Summary

### Problem Identified: inline_expression Parser State Issue

When parsing `Hello {{ user.name }}`:

1. Parser correctly lexes `Hello `, `{{`, and `user`
2. After reducing `user` to `expression`, parser reaches state 382
3. Lookahead `.` should trigger member access continuation
4. **Instead**: External lexer `_code_content` is invoked, consuming `.name }} \n` as code
5. **Result**: ERROR node replaces inline_expression

**Root Cause**: Parser state 382 lacks proper state transition for `.` when in the  `inline_expression` context. The external scanner takes precedence, breaking expression parsing.

### Failed Tests by Category

**Expression Tests (14 failures)**:
- 38, 39, 42: Inline expressions with operators/member access
- 58, 59, 60, 61, 62: Complex expressions, binary/unary
- 88, 89, 94, 95, 96, 97: Member, call expressions

**Paragraph Tests (3 failures)**:
- 12, 13, 98: Multi-line paragraphs

**List Tests (10 failures)**:
- 56, 57, 84, 85, 87, 99, 100: Nesting, indentation

**HTML Tests (2 failures)**:
- 44, 45, 49: HTML tags in text

## Phase 6 Strategy

**Approach**: Minimal surgical fixes to grammar, test after each change.

### 6.1: Disable External Scanner Conflict (PRIORITY)

**Issue**: External `_code_content` scanner interferes with expression parsing in inline_expression contexts.

**Fix Options**:
1. Remove `_code_content` external - check if `code_fence_close` can work without it
2. Add state constraints to prevent external scanner activation mid-expression  
3. Simplify inline_expression to not require full expression parsing (risky)

**Action**: Test option 1 first (safest - external scanner might not be needed)

### 6.2: Fix Member Expression Lookahead

**Issue**: After `identifier` reduces to `expression`, the `.` lookahead doesn't trigger member_expression rule

**Fix**: Ensure `expression` rule properly prioritizes member_expression, or make member_expression non-ambiguous

### 6.3: Test-Driven Fixes for Remaining Failures

Once inline_expression works (38, 39, 42):
- Fix binary/unary operators (58-62, 88-97)
- Address multi-line paragraphs (12-13, 98)
- Fix list indentation (56-57, 84-85, 99-100)

### 6.4: Regression Testing

Run full test suite after each change to ensure 66/100 baseline maintained.

## Next Immediate Action

1. Inspect `_code_content` external scanner - is it necessary?
2. Try removing it or constraining its scope
3. Re-test "Inline expression with property access" (test 38)
4. If fix works → commit and proceed to 6.2
5. If fails → revert and try option 2

