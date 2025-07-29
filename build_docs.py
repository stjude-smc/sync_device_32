#!/usr/bin/env python3
"""
Simple build script for sync_device_32 documentation.
No external tools required - just Python and pip.
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path

def install_deps():
    """Install Sphinx and theme."""
    print("Installing Sphinx and dependencies...")
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "sphinx", "sphinx-rtd-theme"])
        print("✓ Dependencies installed successfully!")
        return True
    except subprocess.CalledProcessError:
        print("✗ Failed to install dependencies")
        return False

def clean_build():
    """Remove previous build."""
    build_dir = Path("sphinx_docs/_build")
    if build_dir.exists():
        print("Cleaning previous build...")
        shutil.rmtree(build_dir)
        print("✓ Build cleaned")

def ensure_static_dir():
    """Ensure _static directory exists."""
    static_dir = Path("sphinx_docs/_static")
    if not static_dir.exists():
        print("Creating _static directory...")
        static_dir.mkdir(parents=True, exist_ok=True)
        print("✓ _static directory created")

def build_html():
    """Build HTML documentation."""
    print("Building HTML documentation...")
    
    # Ensure _static directory exists
    ensure_static_dir()
    
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
    print("=== sync_device_32 Documentation Builder ===\n")
    
    # Check if docs directory exists
    if not Path("sphinx_docs").exists():
        print("✗ sphinx_docs/ directory not found!")
        print("Please run this script from the project root directory.")
        return 1
    
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