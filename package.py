import os
import shutil
import zipfile
import sys

def package():
    # Output file
    zip_filename = "PIME_Setup.zip"
    
    # Files to include (source path, path in zip)
    # Note: install.py expects files in the same directory as itself when running from the extracted folder.
    files_to_include = [
        ('build/PIMELauncher.exe', 'PIMELauncher.exe'),
        ('build/PIMETextService.dll', 'PIMETextService.dll'),
        ('backends.json', 'backends.json'),
        ('version.txt', 'version.txt'),
        ('LICENSE.txt', 'LICENSE.txt'),
        ('README.md', 'README.md'),
        ('install.py', 'install.py'), # Include the installer script
    ]
    
    dirs_to_include = [
        'python',
        'node'
    ]
    
    print(f"Creating {zip_filename}...")
    
    try:
        with zipfile.ZipFile(zip_filename, 'w', zipfile.ZIP_DEFLATED) as zf:
            # Add individual files
            for src, dst in files_to_include:
                if os.path.exists(src):
                    print(f"  Adding {src} as {dst}...")
                    zf.write(src, dst)
                else:
                    print(f"  Warning: {src} not found!")

            # Add directories
            for d in dirs_to_include:
                if os.path.exists(d):
                    print(f"  Adding directory {d}...")
                    for root, dirs, files in os.walk(d):
                        for file in files:
                            file_path = os.path.join(root, file)
                            # Archive name should be relative to the project root, but we want it under the root of the zip
                            # Actually, we want it to preserve the folder structure relative to the install root.
                            # e.g. python/lib/foo.py -> python/lib/foo.py
                            arcname = os.path.relpath(file_path, os.getcwd())
                            # Make sure we only include files inside the target dir
                            if arcname.startswith(d):
                                zf.write(file_path, arcname)
                else:
                    print(f"  Warning: Directory {d} not found!")
                    
    except Exception as e:
        print(f"Error creating zip file: {e}")
        return

    print(f"\nPackage created successfully: {os.path.abspath(zip_filename)}")
    print("To install, extract this zip file and run 'python install.py'.")

if __name__ == "__main__":
    package()
