#!/usr/bin/env python3
"""
Version Sync Tool
Automatically synchronizes version across all project files
"""

import re
import json

def extract_version_from_header():
    """Extract version from include/version.h"""
    with open('include/version.h', 'r', encoding='utf-8') as f:
        content = f.read()
        match = re.search(r'#define\s+PROJECT_VERSION\s+"([^"]+)"', content)
        if match:
            return match.group(1)
    return None

def update_python_server(version):
    """Update Python server version"""
    with open('server/system_monitor_server.py', 'r', encoding='utf-8') as f:
        content = f.read()
    
    updated = re.sub(
        r'System Monitor Server v[\d.]+',
        f'System Monitor Server v{version}',
        content
    )
    
    with open('server/system_monitor_server.py', 'w', encoding='utf-8') as f:
        f.write(updated)
    
    print(f"✓ Updated server/system_monitor_server.py to v{version}")

def update_library_json(version):
    """Update library.json version"""
    with open('lib/components/library.json', 'r', encoding='utf-8') as f:
        data = json.load(f)
    
    data['version'] = version
    
    with open('lib/components/library.json', 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2)
    
    print(f"✓ Updated lib/components/library.json to v{version}")

def main():
    print("="*50)
    print("Version Sync Tool")
    print("="*50)
    
    version = extract_version_from_header()
    if not version:
        print("✗ Could not extract version from include/version.h")
        return
    
    print(f"Current version: {version}")
    print("\nSyncing...")
    
    try:
        update_python_server(version)
        update_library_json(version)
        print("\n✓ All files synchronized successfully!")
    except Exception as e:
        print(f"\n✗ Error: {e}")

if __name__ == '__main__':
    main()
