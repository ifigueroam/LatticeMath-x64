"""
format_docs.py
--------------
Purpose: Automated Documentation Synchronization & Formatting.

This script enforces the project's strict 105-column wrapping limit and 
sanitizes terminology to maintain a formal, third-person active voice 
across all Markdown documentation.
"""

import re
import textwrap
import sys
import os

# Identify project root relative to script location
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)

def wrap_text(text, max_len=105):
    lines = text.split('\n')
    out = []
    in_code_block = False
    for line in lines:
        if line.startswith('```'):
            in_code_block = not in_code_block
            out.append(line)
            continue
        if in_code_block or line.startswith('|') or line.startswith('    ') or line.startswith('\t') or "http" in line:
            out.append(line)
            continue
        
        # If it's a list item, we should preserve the indentation for the wrapped lines
        match = re.match(r'^(\s*[-*+0-9.]+\s+)(.*)', line)
        if match:
            prefix = match.group(1)
            rest = match.group(2)
            # wrap only the rest if it's too long
            if len(line) > max_len:
                wrapped = textwrap.wrap(line, width=max_len, subsequent_indent=" " * len(prefix))
                out.extend(wrapped)
            else:
                out.append(line)
        else:
            if len(line) > max_len:
                wrapped = textwrap.wrap(line, width=max_len)
                out.extend(wrapped)
            else:
                out.append(line)
    return '\n'.join(out)

def replace_bad_words(text):
    text = re.sub(r'(?i)\bthe assistant\b', 'the development team', text)
    text = re.sub(r'(?i)\bthe model\b', 'the framework', text)
    text = re.sub(r'(?i)\bthe system\b', 'the framework', text)
    return text

doc_files = [
    "Docs/DEVLOG.md", 
    "Docs/TRACKLOG.md", 
    "README.md", 
    "Research/RESEARCH.md", 
    "Research/APPLYRESEARCH.md"
]

for rel_path in doc_files:
    file_path = os.path.join(PROJECT_ROOT, rel_path)
    if not os.path.exists(file_path):
        print(f"Warning: {file_path} not found.")
        continue
        
    with open(file_path, 'r') as f:
        content = f.read()
    
    content = replace_bad_words(content)
    content = wrap_text(content)
    
    with open(file_path, 'w') as f:
        f.write(content)
print("Documentation formatting and synchronization complete.")
