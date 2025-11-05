#!/usr/bin/env python3
"""
Check which files still have Serial.print statements
Quick verification script
"""

import re
from pathlib import Path

def check_file(filepath):
    """Check if file has Serial.print statements"""
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    # Find Serial.print statements (excluding ones we want to keep)
    patterns = [
        r'Serial\.println\([^)]*\)',
        r'Serial\.print\([^)]*\)',
        r'Serial\.printf\([^)]*\)',
    ]
    
    matches = []
    for pattern in patterns:
        found = re.finditer(pattern, content)
        for match in found:
            # Get line number
            line_num = content[:match.start()].count('\n') + 1
            line_text = content.split('\n')[line_num - 1].strip()
            
            # Skip if already in DEBUG_MODE block or important message
            if 'DEBUG_MODE' in line_text or 'VERSION_FULL' in line_text:
                continue
            if 'Mode activated' in line_text or 'Upload' in line_text:
                continue
            
            matches.append((line_num, line_text))
    
    return matches

def main():
    src_dir = Path('src')
    
    print("=== Serial.print Check ===\n")
    
    cpp_files = list(src_dir.glob('*.cpp'))
    total_found = 0
    
    for cpp_file in sorted(cpp_files):
        matches = check_file(cpp_file)
        if matches:
            print(f"📄 {cpp_file.name}:")
            for line_num, line_text in matches:
                print(f"  Line {line_num}: {line_text}")
            print()
            total_found += len(matches)
    
    if total_found == 0:
        print("✅ All files refactored! No Serial.print found.")
    else:
        print(f"⚠️  Found {total_found} Serial.print statements to refactor")

if __name__ == '__main__':
    main()
