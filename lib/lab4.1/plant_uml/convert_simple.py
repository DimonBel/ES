#!/usr/bin/env python3
"""
Convert simplified sequence diagrams to PNG.
"""

import os
import re
import base64
import requests
from pathlib import Path

# Ensure img directory exists
os.makedirs('img', exist_ok=True)

def extract_mermaid_from_markdown(md_file):
    """Extract all mermaid code blocks from markdown file"""
    with open(md_file, 'r') as f:
        content = f.read()

    # Find all mermaid code blocks
    pattern = r'```mermaid\n(.*?)\n```'
    matches = re.findall(pattern, content, re.DOTALL)

    return matches

def save_mermaid_files(diagrams):
    """Save each diagram as a separate .mmd file"""
    diagram_names = [
        'seq_01_button_control',
        'seq_02_serial_command',
        'seq_03_signal_conditioning',
        'seq_04_display_update',
        'seq_05_multi_input',
        'seq_06_hardware_control',
        'seq_07_i2c_lcd',
        'seq_08_double_click',
        'seq_09_state_change',
        'seq_10_task_scheduling'
    ]

    for i, (diagram, name) in enumerate(zip(diagrams, diagram_names)):
        filename = f'{name}.mmd'
        with open(filename, 'w') as f:
            f.write(diagram)
        print(f"✓ Saved {filename}")

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
    print("Extracting simplified sequence diagrams...")
    diagrams = extract_mermaid_from_markdown('../SEQUENCE_DIAGRAMS.md')
    print(f"✓ Found {len(diagrams)} diagrams")

    print("\nSaving Mermaid files...")
    save_mermaid_files(diagrams)

    diagram_names = [
        'seq_01_button_control',
        'seq_02_serial_command',
        'seq_03_signal_conditioning',
        'seq_04_display_update',
        'seq_05_multi_input',
        'seq_06_hardware_control',
        'seq_07_i2c_lcd',
        'seq_08_double_click',
        'seq_09_state_change',
        'seq_10_task_scheduling'
    ]

    print("\nConverting to PNG using kroki.io...")
    print("This requires internet connection.\n")

    success_count = 0
    failed_files = []

    for name in diagram_names:
        input_file = f'{name}.mmd'
        output_file = f'img/{name}.png'

        if os.path.exists(output_file):
            print(f"⊙ {name} - Already exists")
            success_count += 1
            continue

        success, message = convert_with_kroki(input_file, output_file)
        if success:
            print(f"✓ {name} - Converted")
            success_count += 1
        else:
            print(f"✗ {name} - Failed: {message}")
            failed_files.append(name)

    print(f"\n{'='*60}")
    print(f"Results: {success_count}/{len(diagram_names)} converted")

    if failed_files:
        print(f"\nFailed files ({len(failed_files)}):")
        for f in failed_files:
            print(f"  - {f}")
    else:
        print("\n✓ All simplified sequence diagrams converted!")

if __name__ == '__main__':
    main()