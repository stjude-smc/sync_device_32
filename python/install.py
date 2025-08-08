#!/usr/bin/env python3
"""
Simple installation script for microsync package.

This script provides an easy way to install the microsync package
either in development mode or as a regular installation.

Usage:
    python install.py [--dev] [--user]

Options:
    --dev    Install in development mode (editable install)
    --user   Install for current user only
"""

import sys
import subprocess
import argparse
from pathlib import Path


def run_command(cmd, description):
    """Run a command and handle errors."""
    print(f"Running: {description}")
    print(f"Command: {' '.join(cmd)}")
    
    try:
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        print("✓ Success!")
        if result.stdout:
            print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print(f"✗ Error: {e}")
        if e.stdout:
            print("STDOUT:", e.stdout)
        if e.stderr:
            print("STDERR:", e.stderr)
        return False


def main():
    parser = argparse.ArgumentParser(
        description="Install microsync package",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    python install.py              # Install normally
    python install.py --dev        # Install in development mode
    python install.py --user       # Install for current user only
    python install.py --dev --user # Install in dev mode for current user
        """
    )
    
    parser.add_argument(
        '--dev', 
        action='store_true',
        help='Install in development mode (editable install)'
    )
    
    parser.add_argument(
        '--user',
        action='store_true', 
        help='Install for current user only'
    )
    
    args = parser.parse_args()
    
    # Check if we're in the right directory
    if not Path('pyproject.toml').exists():
        print("Error: pyproject.toml not found. Please run this script from the python/ directory.")
        sys.exit(1)
    
    # Build the pip install command
    cmd = [sys.executable, '-m', 'pip', 'install']
    
    if args.dev:
        cmd.append('-e')
        cmd.append('.[dev]')  # Include development dependencies
    else:
        cmd.append('.')
    
    if args.user:
        cmd.append('--user')
    
    # Run the installation
    success = run_command(cmd, "Installing microsync package")
    
    if success:
        print("\n🎉 Installation completed successfully!")
        print("\nYou can now use the package:")
        print("  import microsync")
        print("  device = microsync.SyncDevice('COM3')  # or '/dev/ttyUSB0' on Linux")
        
        if args.dev:
            print("\nNote: This is a development installation.")
            print("Changes to the source code will be immediately available.")
    else:
        print("\n❌ Installation failed!")
        sys.exit(1)


if __name__ == '__main__':
    main() 