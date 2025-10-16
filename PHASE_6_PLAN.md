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

## Investigation Results

### External Scanner Analysis

**Finding**: The `_code_content` external scanner IS necessary for code fence handling. Removing it causes regression from 66→54 tests.

**Issue**: Parser state conflict between:
- `inline_expression` context (should use member_expression with `.`)
- `code` rule context (can use external scanner)

When parser sees `{{ user . `, both `member_expression` and `_code_content` are valid, causing ERROR nodes.

**Solution Complexity**: High - requires detailed state machine analysis or grammar restructuring.

### Multi-line Paragraph Analysis

**Current Behavior**: Paragraphs are still being split by single newlines due to block separator `repeat1(/\n/)`
- Test 12 shows last paragraph HAS multi-line support, earlier ones don't
- This is the fundamental issue identified in Phase 1

**Why It's Stuck**: 
- `/repeat1(/\n/)/` separator: Breaks paragraphs into lines
- `/\n\n+/` separator: Breaks consecutive headings/lists
- **Cannot fix without external scanner**

### Critical Insight

**Both inline expressions AND multi-line paragraphs are blocked by external scanner issues:**
1. Expressions: scanner interferes with member access parsing
2. Paragraphs: separator needs context awareness scanner can provide

## Recommended Path Forward

**Option A** (Recommended): Implement proper Phase 2 external scanner
- Fix current `_code_content` scanner to not interfere with expressions
- Add `_NEWLINE` and `_BLANK_LINE` tokens for paragraph control
- This solves BOTH inline expression AND multi-line paragraph issues
- Estimated: 4-6 hours

**Option B**: Continue patching around scanner
- Focus on tests NOT affected by scanner conflicts (HTML, operators, etc.)
- Could reach 70-75/100 but with fundamental issues remaining
- Estimated: 2-3 hours, limited success

**Option C**: Revert to earlier stable commit and take simpler approach
- Current state is complex; may be better to restart from a known good point
- Estimated: Unknown, depends on finding stable baseline

## Next Immediate Action

Given findings, recommend proceeding with **Option A**: Implement proper Phase 2 external scanner that:
1. Fixes `_code_content` to not interfere with expression parsing
2. Adds proper `_NEWLINE` and `_BLANK_LINE` token handling
3. Allows both multi-line paragraphs AND single-newline block separation

