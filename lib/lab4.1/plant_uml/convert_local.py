#!/usr/bin/env python3
"""
Convert Mermaid diagrams using multiple online APIs.
"""

import os
import re
import base64
import requests
from pathlib import Path

# Ensure img directory exists
os.makedirs('img', exist_ok=True)

def convert_with_mermaid_ink(input_file, output_file):
    """Convert using mermaid.ink API"""
    try:
        with open(input_file, 'r') as f:
            mermaid_code = f.read()

        encoded = base64.urlsafe_b64encode(mermaid_code.encode()).decode()
        url = f"https://mermaid.ink/img/{encoded}"

        response = requests.get(url, timeout=30)

        if response.status_code == 200:
            with open(output_file, 'wb') as f:
                f.write(response.content)
            return True, "Success"
        else:
            return False, f"HTTP {response.status_code}"

    except Exception as e:
        return False, str(e)

def convert_with_kroki(input_file, output_file):
    """Convert using kroki.io API"""
    try:
        with open(input_file, 'r') as f:
            mermaid_code = f.read()

        url = "https://kroki.io/mermaid/png"
        response = requests.post(url, data=mermaid_code.encode(), timeout=30)

        if response.status_code == 200:
            with open(output_file, 'wb') as f:
                f.write(response.content)
            return True, "Success"
        else:
            return False, f"HTTP {response.status_code}"

    except Exception as e:
        return False, str(e)

def main():
    # Get all .mmd files
    mmd_files = sorted([f for f in os.listdir('.') if f.endswith('.mmd')])

    print(f"Found {len(mmd_files)} Mermaid files")
    print("Attempting conversion with multiple APIs...\n")

    success_count = 0
    failed_files = []

    for mmd_file in mmd_files:
        output_file = f"img/{mmd_file.replace('.mmd', '.png')}"

        # Skip if already exists
        if os.path.exists(output_file):
            print(f"⊙ {mmd_file} - Already exists")
            success_count += 1
            continue

        # Try mermaid.ink first
        success, message = convert_with_mermaid_ink(mmd_file, output_file)
        if success:
            print(f"✓ {mmd_file} - Converted (mermaid.ink)")
            success_count += 1
            continue

        # Try kroki.io as fallback
        print(f"  ⊙ mermaid.ink failed: {message}, trying kroki.io...")
        success, message = convert_with_kroki(mmd_file, output_file)
        if success:
            print(f"✓ {mmd_file} - Converted (kroki.io)")
            success_count += 1
            continue

        # Both failed
        print(f"✗ {mmd_file} - Failed: {message}")
        failed_files.append(mmd_file)

    print(f"\n{'='*60}")
    print(f"Results: {success_count}/{len(mmd_files)} converted")

    if failed_files:
        print(f"\nFailed files ({len(failed_files)}):")
        for f in failed_files:
            print(f"  - {f}")
    else:
        print("\n✓ All diagrams converted successfully!")

if __name__ == '__main__':
    main()