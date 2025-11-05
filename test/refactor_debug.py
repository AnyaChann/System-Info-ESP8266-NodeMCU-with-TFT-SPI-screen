#!/usr/bin/env python3
"""
Auto-refactor Serial.print to DEBUG_* macros
Author: AnyaChann
Version: 1.0
"""

import re
import os
from pathlib import Path

# Module prefix mapping based on filename
MODULE_PREFIX = {
    'button_handler': 'BTN',
    'network_manager': 'NET',
    'config_manager': 'CFG',
    'config_storage': 'STOR',
    'config_validator': 'VAL',
    'ota_manager': 'OTA',
    'ota_web_manager': 'OTA',
    'display_manager': 'DISP',
    'main': 'MAIN',
}

# Messages to KEEP as Serial.print (important for user)
KEEP_MESSAGES = [
    r'Mode activated',
    r'Upload started',
    r'Upload success',
    r'VERSION_FULL',
    r'BUILD_DATE',
    r'BUILD_TIME',
]

def should_keep_message(line):
    """Check if this Serial.print should be kept as-is"""
    for pattern in KEEP_MESSAGES:
        if re.search(pattern, line):
            return True
    return False

def get_module_prefix(filename):
    """Get module prefix from filename"""
    name = Path(filename).stem
    return MODULE_PREFIX.get(name, 'LOG')

def add_config_include(content):
    """Add #include "config.h" if not present"""
    if '#include "config.h"' in content:
        return content
    
    # Find first #include and add config.h before it
    match = re.search(r'(#include\s+[<"])', content)
    if match:
        pos = match.start()
        return content[:pos] + '#include "config.h"\n' + content[pos:]
    
    return content

def refactor_serial_print(content, prefix):
    """Replace Serial.print* with DEBUG_* macros"""
    lines = content.split('\n')
    new_lines = []
    
    for line in lines:
        # Skip if already using DEBUG_* or in #ifdef DEBUG_MODE block
        if 'DEBUG_PRINT' in line or 'DEBUG_PRINTLN' in line or 'DEBUG_PRINTF' in line:
            new_lines.append(line)
            continue
        
        # Skip if this is an important message
        if should_keep_message(line):
            new_lines.append(line)
            continue
        
        # Replace Serial.println
        if 'Serial.println(' in line:
            # Extract the message
            match = re.search(r'Serial\.println\((.*?)\);', line)
            if match:
                msg = match.group(1)
                # Add prefix if it's a string literal
                if 'F("' in msg or '("' in msg:
                    msg = re.sub(r'F\("', f'F("[{prefix}] ', msg)
                    msg = re.sub(r'^\("', f'"[{prefix}] ', msg)
                line = re.sub(r'Serial\.println\((.*?)\);', f'DEBUG_PRINTLN({msg});', line)
        
        # Replace Serial.print
        elif 'Serial.print(' in line:
            match = re.search(r'Serial\.print\((.*?)\);', line)
            if match:
                msg = match.group(1)
                # Add prefix for first print in line
                if 'F("' in msg or '("' in msg:
                    msg = re.sub(r'F\("', f'F("[{prefix}] ', msg)
                    msg = re.sub(r'^\("', f'"[{prefix}] ', msg)
                line = re.sub(r'Serial\.print\((.*?)\);', f'DEBUG_PRINT({msg});', line)
        
        # Replace Serial.printf
        elif 'Serial.printf(' in line:
            match = re.search(r'Serial\.printf\("(.*?)"', line)
            if match:
                msg = match.group(1)
                new_msg = f'[{prefix}] {msg}'
                line = re.sub(r'Serial\.printf\("(.*?)"', f'Serial.printf("{new_msg}"', line)
                line = line.replace('Serial.printf', 'DEBUG_PRINTF')
        
        new_lines.append(line)
    
    return '\n'.join(new_lines)

def process_file(filepath):
    """Process a single C++ file"""
    print(f"Processing: {filepath}")
    
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Check if already refactored
    if 'DEBUG_PRINTLN' in content and 'DEBUG_PRINT' in content:
        print(f"  ✓ Already refactored, skipping...")
        return
    
    # Get module prefix
    prefix = get_module_prefix(filepath)
    print(f"  Using prefix: [{prefix}]")
    
    # Add config.h include
    content = add_config_include(content)
    
    # Refactor Serial.print
    content = refactor_serial_print(content, prefix)
    
    # Write back
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"  ✓ Done!")

def main():
    """Main function"""
    src_dir = Path('src')
    
    if not src_dir.exists():
        print("Error: src/ directory not found!")
        return
    
    print("=== Auto-refactor Debug System ===\n")
    
    # Find all .cpp files
    cpp_files = list(src_dir.glob('*.cpp'))
    
    print(f"Found {len(cpp_files)} C++ files\n")
    
    for cpp_file in cpp_files:
        try:
            process_file(cpp_file)
            print()
        except Exception as e:
            print(f"  ✗ Error: {e}\n")
    
    print("=== Refactor Complete! ===")
    print("\nNext steps:")
    print("1. Review changes with: git diff")
    print("2. Build project: pio run")
    print("3. Toggle debug in include/config.h")

if __name__ == '__main__':
    main()
