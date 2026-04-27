#!/usr/bin/env python3
"""
Convert all Mermaid diagrams for Lab 4.2 Dual Actuator Control System.
Uses local mermaid CLI (mmdc) for reliable conversion.
"""

import os
import sys
import subprocess
from pathlib import Path

# Ensure img directory exists
os.makedirs('img', exist_ok=True)

def convert_with_mmdc(input_file, output_file):
    """Convert using local mermaid CLI (mmdc)"""
    try:
        cmd = [
            'mmdc',
            '-i', input_file,
            '-o', output_file,
            '-b', 'transparent',
            '-s', '2'  # Scale factor for better quality
        ]

        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)

        if result.returncode == 0:
            return True, "Success"
        else:
            return False, f"Return code {result.returncode}: {result.stderr}"

    except subprocess.TimeoutExpired:
        return False, "Timeout after 30 seconds"
    except FileNotFoundError:
        return False, "mmdc command not found"
    except Exception as e:
        return False, str(e)

def main():
    # List of all diagram files
    diagrams = [
        '01_architecture',
        '02_component',
        '03_class',
        '04_activity_actuatorcontrol',
        '05_activity_signalconditioning',
        '06_activity_servocontrol',
        '07_activity_display',
        '08_dataflow',
        '09_sequence_dual_control',
        '10_state_machine',
        '11_coordination_logic',
        '12_hardware_connections',
        '13_task_priorities',
        '14_timing_diagram',
        '15_response_times',
        '16_error_handling'
    ]

    print("=" * 70)
    print("Lab 4.2 Dual Actuator Control System - Diagram Converter")
    print("Using local mermaid CLI (mmdc)")
    print("=" * 70)
    print(f"Found {len(diagrams)} Mermaid diagrams to convert\n")

    success_count = 0
    failed_files = []

    for diagram in diagrams:
        input_file = f'{diagram}.mmd'
        output_file = f'img/{diagram}.png'

        # Skip if input file doesn't exist
        if not os.path.exists(input_file):
            print(f"✗ {input_file} not found")
            failed_files.append(diagram)
            continue

        # Skip if already exists and is valid
        if os.path.exists(output_file):
            file_size = os.path.getsize(output_file)
            if file_size > 1000:  # Valid image size
                print(f"⊙ {input_file} - Already exists ({file_size} bytes)")
                success_count += 1
                continue
            else:
                print(f"⚠ {input_file} - Exists but too small ({file_size} bytes), re-converting...")

        # Convert using local mmdc
        success, message = convert_with_mmdc(input_file, output_file)

        if success and os.path.exists(output_file):
            file_size = os.path.getsize(output_file)
            print(f"✓ {input_file} - Converted ({file_size} bytes)")
            success_count += 1
        else:
            print(f"✗ {input_file} - Failed: {message}")
            failed_files.append(diagram)

    print(f"\n{'=' * 70}")
    print(f"Conversion Results: {success_count}/{len(diagrams)} successful")
    print('=' * 70)

    if failed_files:
        print(f"\nFailed files ({len(failed_files)}):")
        for f in failed_files:
            print(f"  ✗ {f}.mmd")
        print(f"\nYou can view the failed diagrams in their .mmd files.")
        return 1
    else:
        print(f"\n✓ All {len(diagrams)} diagrams converted successfully!")
        print(f"✓ Images are ready in the img/ directory")
        return 0

if __name__ == '__main__':
    sys.exit(main())