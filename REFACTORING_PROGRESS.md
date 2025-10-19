# Tree-sitter-markdoc Refactoring Progress

## Summary

Successfully refactored the parser from 72% to 93% test pass rate (99/106 tests passing).

## Major Fixes Implemented

### 1. List Architecture Overhaul
- Replaced `list_item` paragraph nodes with dedicated `list_paragraph` nodes
- Added `list_marker` whitespace requirement to avoid ambiguity with inline formatting
- Fixed list item separation with optional newlines

### 2. Scanner CODE_CONTENT Rewrite
- Complete rewrite of code fence handling in scanner
- Removed stateful fence tracking (in_fence, fence_char, fence_len)
- Scanner now checks column position to avoid consuming info_string
- Correctly handles empty code blocks
- Code content properly stops at closing fence pattern

### 3. Comment Block Fixes
- Allow spaces around 'comment' keyword in comment blocks

### 4. Text Pattern Fixes
- Allow `#` in text pattern to handle invalid heading markers
- Fixed invalid heading test to match Markdown paragraph behavior

### 5. Tag Improvements
- Allow optional newlines inside markdoc tags

## Remaining Failures (6 tests)

### All Tag-related
1. Test #40: Tag with heading inside
2. Test #42: Mixed content with tags and expressions  
3. Test #62: Tag with complex attribute expressions
4. Test #85: Mixed list and tag indentation
5. Test #86: Tag with blank lines in content
6. Test #87: Indented code block in tag

All remaining failures involve Markdoc tags not properly parsing nested content. Tags need architectural improvements to support:
- Headings inside tags
- Code blocks inside tags
- Lists with proper indentation inside tags
- Blank lines within tag content
- Complex expressions in tag attributes

## Test Pass Rate History

- Initial: 72/106 (68%)
- After list fixes: 83/106 (78%)
- After scanner rewrite: 97/106 (92%)
- Current: 100/106 (94%)

## Next Steps

The remaining failures require more complex fixes:

1. **Tag content parsing**: Tags need better support for nested blocks (headings, code, lists with proper indentation)
2. **Expression parsing**: Complex member access chains and attribute expressions need precedence/associativity fixes
3. **Blank line handling in tags**: Tags should allow blank lines within their content

These issues are more architectural and may require careful grammar restructuring to avoid conflicts.
