#!/usr/bin/env python3
"""
Simple build script for microsync documentation.
No external tools required - just Python and pip.

Usage:
    python build_docs.py          # Build documentation
    python build_docs.py clean    # Clean generated files only
    python build_docs.py build    # Build documentation (explicit)
"""

import os
import sys
import subprocess
import shutil
import argparse
from pathlib import Path

def install_deps():
    """Install Sphinx and dependencies from requirements.txt."""
    print("Installing Sphinx and dependencies...")
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "-r", "sphinx_docs/requirements.txt"])
        print("✓ Dependencies installed successfully!")
        return True
    except subprocess.CalledProcessError:
        print("✗ Failed to install dependencies")
        return False

def clean_build():
    """Remove all generated documentation files and directories."""
    sphinx_docs = Path("sphinx_docs")
    
    # List of directories and files to clean
    to_clean = [
        "_build",           # Sphinx build output
        "_doxygen",         # Doxygen XML output
        "_static",          # Generated static files
        "cpp_api",          # Generated C++ API documentation
        "latex",            # LaTeX output (if generated)
        "xml",              # Additional XML output
    ]
    
    print("Cleaning all generated documentation files...")
    
    for item in to_clean:
        item_path = sphinx_docs / item
        if item_path.exists():
            if item_path.is_dir():
                shutil.rmtree(item_path)
                print(f"  ✓ Removed directory: {item}")
            else:
                item_path.unlink()
                print(f"  ✓ Removed file: {item}")
    
    print("✓ All generated files cleaned")

def ensure_directories():
    """Ensure required directories exist."""
    # Ensure _static directory exists
    static_dir = Path("sphinx_docs/_static")
    if not static_dir.exists():
        print("Creating _static directory...")
        static_dir.mkdir(parents=True, exist_ok=True)
        print("✓ _static directory created")
    
    # Ensure _doxygen directory exists for Doxygen output
    doxygen_dir = Path("sphinx_docs/_doxygen")
    if not doxygen_dir.exists():
        print("Creating _doxygen directory...")
        doxygen_dir.mkdir(parents=True, exist_ok=True)
        print("✓ _doxygen directory created")

def build_html():
    """Build HTML documentation."""
    print("Building HTML documentation...")
    
    # Ensure required directories exist
    ensure_directories()
    
    # Add Doxygen to PATH if not already there
    doxygen_path = r"C:\Program Files\doxygen\bin"
    if doxygen_path not in os.environ.get('PATH', ''):
        os.environ['PATH'] = doxygen_path + os.pathsep + os.environ.get('PATH', '')
    
    # Change to docs directory
    os.chdir("sphinx_docs")
    
    try:
        # Run sphinx-build
        subprocess.check_call([
            sys.executable, "-m", "sphinx.cmd.build",
            "-b", "html",
            ".", "_build/html"
        ])
        print("✓ Documentation built successfully!")
        return True
    except subprocess.CalledProcessError:
        print("✗ Build failed")
        return False
    finally:
        # Change back to project root
        os.chdir("..")

def open_browser():
    """Open documentation in browser."""
    html_file = Path("sphinx_docs/_build/html/index.html")
    if html_file.exists():
        print("Opening documentation in browser...")
        if sys.platform == "darwin":  # macOS
            subprocess.run(["open", str(html_file)])
        elif sys.platform.startswith("linux"):  # Linux
            subprocess.run(["xdg-open", str(html_file)])
        elif sys.platform == "win32":  # Windows
            subprocess.run(["start", str(html_file)], shell=True)
        print("✓ Browser opened")
    else:
        print("✗ HTML file not found")

def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser(
        description="Build or clean microsync documentation",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python build_docs.py          # Build documentation
  python build_docs.py clean    # Clean generated files only
  python build_docs.py build    # Build documentation (explicit)
        """
    )
    parser.add_argument(
        'command', 
        nargs='?', 
        default='build',
        choices=['build', 'clean'],
        help='Command to run (default: build)'
    )
    
    args = parser.parse_args()
    
    # Check if docs directory exists
    if not Path("sphinx_docs").exists():
        print("✗ sphinx_docs/ directory not found!")
        print("Please run this script from the project root directory.")
        return 1
    
    if args.command == 'clean':
        print("=== microsync Documentation Cleaner ===\n")
        clean_build()
        print("\n=== Clean Complete! ===")
        return 0
    
    elif args.command == 'build':
        print("=== microsync Documentation Builder ===\n")
        
        # Install dependencies
        if not install_deps():
            return 1
        
        # Clean previous build
        clean_build()
        
        # Build documentation
        if not build_html():
            return 1
        
        # Open in browser
        open_browser()
        
        print("\n=== Build Complete! ===")
        print("Documentation is available at: sphinx_docs/_build/html/index.html")
        return 0

if __name__ == "__main__":
    sys.exit(main()) 