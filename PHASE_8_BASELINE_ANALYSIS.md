# Phase 8: Complete Architectural Overhaul - Baseline Analysis

## Executive Summary

**Current Status**: 72 tests passing out of 52 actual tests (discrepancy in counting - need to recount)
**Target**: 100% test pass rate
**Approach**: Complete architectural refactoring prioritizing Lists → Edge Cases → HTML → Expressions

## Test Inventory (52 Tests Identified)

### Test List
1. ATX heading levels 4-6
2. Heading without space after marker (invalid)
3. Backtick fence without info string
4. Backtick fence with language
5. Backtick fence with language and attributes
6. Tilde fence without info string
7. Tilde fence with language
8. Tilde fence with language and attributes
9. Multi-line code content
10. Code block with heading after
11. Code block with paragraph after
12. Mixed backticks and content
13. Code block with frontmatter
14. Block tag with paragraph content
15. Block tag with multiple paragraphs
16. Nested block tags
17. Tag with multiple attributes
18. Tag with heading inside
19. Tag with code block inside
20. Mixed content with tags and expressions
21. Tag with frontmatter
22. HTML block tag
23. HTML self-closing tag
24. Markdoc comment block
25. Mixed HTML and Markdoc content
26. Simple unordered list
27. Simple ordered list
28. Mixed unordered markers
29. List after heading
30. List before paragraph
31. Nested unordered list (simple)
32. Nested ordered list
33. Complex member access expressions
34. Function call expressions
35. Object and array expressions
36. Binary and unary expressions
37. Tag with complex attribute expressions
38. Inline formatting in list
39. Alternative tag closing syntax (without slash)
40. Tag with extra whitespace
41. Nested list indentation
42. Mixed list and tag indentation
43. Indented code block in tag
44. Call expression simple
45. Call expression with arguments
46. Member call expression
47. Nested expressions
48. List with paragraphs
49. Mixed blocks and lists
50. List with items separated by newlines
51. Code block with newlines preserved
52. Blank line with only whitespace

## Priority Categorization

### Priority 1: Lists (Est. 14 tests)
**Tests**: 26, 27, 28, 29, 30, 31, 32, 38, 41, 42, 48, 49, 50, possibly more

**Known Issues**:
- `list_item` uses `$.paragraph` instead of `$.list_paragraph` (grammar.js line 353)
- Nested list indentation not properly handled
- List items separated by newlines create ERROR nodes
- Mixed content (lists + paragraphs) parsing fails

**Root Cause**: 
- Architectural issue: Lists need dedicated `list_paragraph` nodes
- Scanner INDENT/DEDENT tokens exist but not fully integrated with list parsing
- List rule (lines 340-348) has conflicts with nested structure support

### Priority 2: Edge Cases (Est. 9 tests)
**Tests**: 2, 12, 43, 51, 52, possibly more

**Known Issues**:
- Code blocks with newlines (test 51)
- Blank lines with whitespace (test 52)
- Mixed content boundaries
- Invalid syntax handling (test 2)

**Root Cause**:
- Boundary condition handling in scanner
- Whitespace normalization issues
- Precedence conflicts at block boundaries

### Priority 3: HTML (Est. 6 tests)
**Tests**: 22, 23, 25, possibly more

**Known Issues**:
- HTML blocks are monolithic tokens (grammar.js lines 370-389)
- Tests expect `html_attr` and `html_content` structured nodes
- No attribute parsing

**Root Cause**:
- `html_block` uses single `token()` capturing entire HTML
- Need to decompose into: open tag + attributes + content + close tag

### Priority 4: Expressions (Est. 5 tests)
**Tests**: 33, 34, 36, 37, 44, 45, 46, 47

**Known Issues**:
- Binary/unary operators defined but may have precedence conflicts
- Complex member access may fail
- Expression hierarchy integration issues

**Root Cause**:
- Precedence values need review (lines 232-239 for binary, 242-245 for unary)
- Conflicts between expression types and other grammar rules

## Current Grammar Architecture

### Key Rules (grammar.js)
```
source_file (lines 39-53)
  - Uses choice($_BLANK_LINE) for block separation
  - Scanner provides context-aware newline tokens

_block (lines 55-66)
  - Choices: comment_block, markdoc_tag, fenced_code_block, heading, 
    thematic_break, blockquote, html_block, list, html_comment, paragraph

list (lines 340-348)
  - Attempts to use INDENT/DEDENT tokens
  - Has prec(2) for nested lists, prec(1) for continuation
  - Currently using $.paragraph in list_item

list_item (lines 351-354)
  - field('marker', $.list_marker)
  - field('content', $.paragraph)  ← WRONG: should be $.list_paragraph

paragraph (lines 413-420)
  - Uses $._NEWLINE for line continuation
  - Works correctly for document-level paragraphs

list_paragraph (lines 423-430)
  - Defined but UNUSED
  - Identical structure to paragraph
  - Should be used in list_item

expression hierarchy (lines 180-200)
  - Top-level: arrow_function, binary_expression, unary_expression, 
    call_expression, member_expression, array_access, _primary_expression
  - Precedence defined for operators

html_block (lines 370-389)
  - Monolithic token() patterns
  - No structured parsing
```

### Conflicts Array (lines 25-36)
```javascript
conflicts: $ => [
  [$.source_file],
  [$.list_item],
  [$.attribute, $.expression],
  [$.paragraph, $._inline_content],
  [$.paragraph],
  [$.attribute, $._primary_expression],
  [$.markdoc_tag, $.paragraph],
  [$.attribute_value, $._primary_expression],
  [$.tag_open, $.tag_close],
  [$.binary_expression]
]
```

## Current Scanner Architecture (src/scanner.c)

### State Machine Components
1. **Indentation Stack** (lines 13-19)
   - Tracks indent levels up to 64 deep
   - Stores indent values in `indent_stack[]`
   - Current length in `indent_len`

2. **Fence Tracking** (lines 16-18)
   - `in_fence`: boolean for code block state
   - `fence_char`: `` ` `` or `~`
   - `fence_len`: number of fence characters

3. **Token Types** (lines 5-11)
   - CODE_CONTENT
   - NEWLINE
   - BLANK_LINE
   - INDENT
   - DEDENT

### Scanner Logic
**INDENT/DEDENT** (lines 85-122):
- Triggered by leading whitespace
- Counts spaces (tabs = 4 spaces)
- Compares to top of indent_stack
- Emits INDENT if indent increases
- Emits DEDENT if indent decreases
- Returns false at EOF or newline

**CODE_CONTENT** (lines 125-195):
- Detects fence start (3+ `` ` `` or `~`)
- Consumes content until closing fence
- Tracks line starts for fence detection
- Sets `in_fence` state

**NEWLINE/BLANK_LINE** (lines 197-253):
- Counts consecutive newlines
- Looks ahead for next character
- Checks if next line is indented
- Emits BLANK_LINE for 2+ newlines
- Emits NEWLINE for single newline (if not followed by indent)

## Test Corpus Analysis

### Test File Structure
```
test/corpus/
├── 01-document-frontmatter.txt
├── 02-headings-paragraphs.txt
├── 03-fenced-code.txt
├── 04-markdoc-tags.txt
├── 05-html-tags-and-comments.txt
├── 05-lists.txt
├── 06-expressions-and-attributes.txt
├── 06-inline-formatting.txt
├── 07-html.txt
├── 07-tag-closing-and-whitespace.txt
├── 08-expressions.txt
├── 08-paragraphs-and-lists.txt
└── 09-newlines-and-blank-lines.txt
```

## Failure Analysis from Baseline

### Sample Failure Patterns

**List Failures**:
```
Expected: (list (list_item (list_marker) (list_paragraph (text))))
Actual:   (list (list_item (list_marker) (paragraph (text))))
          ERROR nodes for nested lists
```

**HTML Failures**:
```
Expected: (html_block (html_attr ...) (html_content ...))
Actual:   (html_block)  ← monolithic token
```

**Expression Failures**:
```
Expected: (binary_expression (expression) (expression))
Actual:   ERROR nodes with UNEXPECTED tokens
```

## Implementation Roadmap

### Phase 1: Lists (Priority 1)
**Target**: Fix 14 list-related test failures
**Time Estimate**: 2-3 hours

1. Change `list_item` rule to use `$.list_paragraph`
2. Fix scanner INDENT/DEDENT integration with lists
3. Handle blank line separation between list items
4. Test nested list indentation

### Phase 2: Edge Cases (Priority 2)
**Target**: Fix 9 edge case failures
**Time Estimate**: 1-2 hours

1. Fix code block newline preservation
2. Fix blank line with whitespace handling
3. Fix mixed content boundary detection
4. Test invalid syntax handling

### Phase 3: HTML (Priority 3)
**Target**: Fix 6 HTML-related failures
**Time Estimate**: 2 hours

1. Decompose `html_block` into structured rules
2. Add `html_attr` parsing
3. Add `html_content` parsing
4. Handle both paired and self-closing tags

### Phase 4: Expressions (Priority 4)
**Target**: Fix 5 expression-related failures
**Time Estimate**: 1 hour

1. Review binary operator precedence
2. Fix unary operator integration
3. Test complex member access
4. Validate expression hierarchy

### Phase 5: Integration & Validation
**Target**: 100% test pass rate
**Time Estimate**: 1 hour

1. Run full test suite
2. Resolve any remaining conflicts
3. Optimize conflicts array
4. Validate parser generation

## Success Criteria

- [x] Baseline documented (this file)
- [ ] 100% test pass rate (52/52 tests passing)
- [ ] No ERROR nodes in any parse tree
- [ ] Parser generation completes without errors
- [ ] All changes committed following conventional commits
- [ ] Stayed on feature/markdoc-1.0.0 branch

## Next Steps

1. Mark Phase 0 as complete
2. Begin Phase 1 (Lists Architecture Overhaul)
3. Make first change: `list_item` to use `list_paragraph`

---

**Document Created**: Phase 8 Baseline Analysis
**Branch**: feature/markdoc-1.0.0
**Commit Status**: Ready for baseline checkpoint commit
