import os
import sys
import shutil
import subprocess
import ctypes
import time
import json

def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False

def run_as_admin():
    # Re-run the program with admin rights
    ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, " ".join(sys.argv), None, 1)

def kill_process(proc_name):
    print(f"Stopping {proc_name}...")
    subprocess.call(['taskkill', '/F', '/IM', proc_name], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

def install():
    # Check for admin rights
    if not is_admin():
        print("Requesting administrator privileges...")
        run_as_admin()
        return

    # Define paths
    # Determine Program Files directory (x86) for 32-bit app on 64-bit OS, or normal Program Files
    program_files = os.environ.get('ProgramFiles(x86)', os.environ.get('ProgramFiles'))
    INSTALL_DIR = os.path.join(program_files, 'PIME')
    
    FILES_MAPPING = [
        ('build/PIMELauncher.exe', 'PIMELauncher.exe'),
        ('build/PIMETextService.dll', 'PIMETextService.dll'),
        ('backends.json', 'backends.json'),
        ('version.txt', 'version.txt'),
        ('LICENSE.txt', 'LICENSE.txt'),
        ('README.md', 'README.md')
    ]

    print(f"Installing PIME to: {INSTALL_DIR}")

    # 1. Stop running processes
    kill_process('PIMELauncher.exe')
    # Give it a moment to release file locks
    time.sleep(1)

    # 2. Create installation directory
    if not os.path.exists(INSTALL_DIR):
        try:
            os.makedirs(INSTALL_DIR)
        except OSError as e:
            print(f"Error creating directory {INSTALL_DIR}: {e}")
            input("Press Enter to exit...")
            return

    # 3. Unregister old DLL if it exists (to release lock)
    dll_path = os.path.join(INSTALL_DIR, 'PIMETextService.dll')
    if os.path.exists(dll_path):
        print("Unregistering old DLL...")
        subprocess.call(['regsvr32', '/u', '/s', dll_path])
        time.sleep(0.5)

    # 4. Copy files
    print("Copying files...")
    for src, dst_name in FILES_MAPPING:
        dst = os.path.join(INSTALL_DIR, dst_name)
        
        # Handle source file location (some might be in root, some in build)
        if not os.path.exists(src):
            # Try looking in root if not found in specific path (or vice versa if needed)
            if os.path.exists(os.path.basename(src)):
                src = os.path.basename(src)
            else:
                print(f"Warning: Source file '{src}' not found. Skipping.")
                continue

        try:
            shutil.copy2(src, dst)
            print(f"  Copied: {dst_name}")
        except Exception as e:
            # Try renaming the old file if it's locked (WinError 32)
            if isinstance(e, OSError) and e.winerror == 32:
                print(f"  File locked: {dst_name}. Trying to rename old file...")
                try:
                    old_file = dst + ".old." + str(int(time.time()))
                    if os.path.exists(dst):
                        os.rename(dst, old_file)
                        print(f"  Renamed locked file to: {os.path.basename(old_file)}")
                    shutil.copy2(src, dst)
                    print(f"  Copied: {dst_name}")
                    # Try to schedule deletion on reboot (optional, or just leave it)
                    # MoveFileEx with MOVEFILE_DELAY_UNTIL_REBOOT requires ctypes, simpler to just leave it for now
                except Exception as e2:
                    print(f"  Failed to replace locked file {dst_name}: {e2}")
            else:
                print(f"  Error copying {dst_name}: {e}")

    # 4.1 Copy backend directories if they exist (python, node)
    for backend_dir in ['python', 'node']:
        src_dir = backend_dir
        dst_dir = os.path.join(INSTALL_DIR, backend_dir)
        if os.path.exists(src_dir):
            print(f"Copying {backend_dir} directory...")
            try:
                if os.path.exists(dst_dir):
                    shutil.rmtree(dst_dir)
                shutil.copytree(src_dir, dst_dir)
                print(f"  Copied directory: {backend_dir}")
            except Exception as e:
                print(f"  Error copying directory {backend_dir}: {e}")
        else:
            print(f"Note: Backend directory '{backend_dir}' not found in source. PIME may need it to function.")

    # 5. Register new DLL
    print("Registering PIMETextService.dll...")
    if os.path.exists(dll_path):
        ret = subprocess.call(['regsvr32', '/s', dll_path])
        if ret == 0:
            print("  Registration successful.")
        else:
            print(f"  Registration failed (Exit code: {ret}).")
    else:
        print("  Error: PIMETextService.dll not found in install directory.")

    # 6. Start PIMELauncher
    launcher_path = os.path.join(INSTALL_DIR, 'PIMELauncher.exe')
    if os.path.exists(launcher_path):
        print("Starting PIMELauncher...")
        # Use Popen to run detached
        subprocess.Popen([launcher_path], cwd=INSTALL_DIR)
        print("PIME started.")

    # 7. Force update user settings to horizontal
    try:
        user_settings_path = os.path.join(os.environ["LOCALAPPDATA"], "PIME", "PIMESettings.json")
        if os.path.exists(user_settings_path):
            print("Updating user settings to horizontal layout...")
            with open(user_settings_path, "r") as f:
                settings = json.load(f)
            
            settings["horizontal"] = True
            
            with open(user_settings_path, "w") as f:
                json.dump(settings, f)
            print("User settings updated.")
    except Exception as e:
        print(f"Warning: Could not update user settings: {e}")
    
    print("\nInstallation finished successfully!")
    print("Note: You may need to restart your applications (e.g., Notepad) to see the input method.")
    input("Press Enter to close...")

if __name__ == '__main__':
    install()
