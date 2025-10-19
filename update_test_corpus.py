#!/usr/bin/env python3
"""
Update all test corpus files to use list_paragraph instead of paragraph
in list_item nodes.
"""

import re
from pathlib import Path

corpus_dir = Path("test/corpus")

# Pattern to match list_item with paragraph (across multiple lines)
pattern = re.compile(
    r'(\(list_item\s+\(list_marker\)\s+)\(paragraph',
    re.MULTILINE
)

replacement = r'\1(list_paragraph'

for corpus_file in corpus_dir.glob("*.txt"):
    print(f"Processing {corpus_file}...")
    content = corpus_file.read_text(encoding="utf-8")
    updated_content = pattern.sub(replacement, content)
    
    if content != updated_content:
        corpus_file.write_text(updated_content, encoding="utf-8")
        print(f"  ✓ Updated {corpus_file.name}")
    else:
        print(f"  - No changes needed for {corpus_file.name}")

print("\nDone!")
