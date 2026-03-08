#!/usr/bin/env python3
"""
Convert all Mermaid diagrams including new detailed ones.
"""

import os
import re
import subprocess
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
        '01_architecture',
        '02_component',
        '03_class',
        '04_activity_detect',
        '05_activity_display',
        '06_activity_led',
        '07_dataflow',
        '08_layers',
        '09_detailed_architecture',
        '10_sequence_sound_sensor',
        '11_sequence_lcd',
        '12_sequence_led',
        '13_dataflow_input',
        '14_dataflow_processing',
        '15_dataflow_storage',
        '16_dataflow_display',
        '17_dataflow_led',
        '18_dataflow_sync',
        '19_sequence_synchronization'
    ]

    for i, (diagram, name) in enumerate(zip(diagrams, diagram_names)):
        filename = f'{name}.mmd'
        with open(filename, 'w') as f:
            f.write(diagram)
        print(f"✓ Saved {filename}")

def convert_with_api(diagram_names):
    """Convert using online API"""
    import requests
    import base64

    success_count = 0
    for name in diagram_names:
        input_file = f'{name}.mmd'
        output_file = f'img/{name}.png'

        if not os.path.exists(input_file):
            print(f"✗ {input_file} not found")
            continue

        with open(input_file, 'r') as f:
            mermaid_code = f.read()

        try:
            # Encode the mermaid code
            encoded = base64.urlsafe_b64encode(mermaid_code.encode()).decode()

            # Use mermaid.ink API
            url = f"https://mermaid.ink/img/{encoded}"

            response = requests.get(url, timeout=30)

            if response.status_code == 200:
                with open(output_file, 'wb') as f:
                    f.write(response.content)
                print(f"✓ Converted {name} to PNG")
                success_count += 1
            else:
                print(f"✗ Failed to convert {name}: HTTP {response.status_code}")

        except Exception as e:
            print(f"✗ Error converting {name}: {e}")

    return success_count

def main():
    print("Extracting Mermaid diagrams from Markdown...")
    diagrams = extract_mermaid_from_markdown('diagrams.md')
    print(f"✓ Found {len(diagrams)} diagrams")

    print("\nSaving Mermaid files...")
    save_mermaid_files(diagrams)

    diagram_names = [
        '01_architecture',
        '02_component',
        '03_class',
        '04_activity_detect',
        '05_activity_display',
        '06_activity_led',
        '07_dataflow',
        '08_layers',
        '09_detailed_architecture',
        '10_sequence_sound_sensor',
        '11_sequence_lcd',
        '12_sequence_led',
        '13_dataflow_input',
        '14_dataflow_processing',
        '15_dataflow_storage',
        '16_dataflow_display',
        '17_dataflow_led',
        '18_dataflow_sync',
        '19_sequence_synchronization'
    ]

    print("\nConverting to PNG using online API...")
    print("This requires internet connection.\n")

    success_count = convert_with_api(diagram_names)

    print(f"\n✓ Successfully converted {success_count}/{len(diagram_names)} diagrams")

    if success_count == len(diagram_names):
        print("\nAll diagrams are ready in the img/ directory!")
    else:
        print("\nSome diagrams failed. You can still view them in diagrams_viewer.html")

if __name__ == '__main__':
    main()