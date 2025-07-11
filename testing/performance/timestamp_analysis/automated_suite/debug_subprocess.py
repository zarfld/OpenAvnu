#!/usr/bin/env python3
"""Debug subprocess issue with gPTP"""

import subprocess
import shlex
import os

def test_subprocess_call(cmd_str):
    print(f"Testing command: {cmd_str}")
    print(f"Current working directory: {os.getcwd()}")
    
    try:
        # Method 1: Using shlex.split with Windows compatibility
        if os.name == 'nt':  # Windows
            cmd_args = shlex.split(cmd_str, posix=False)
        else:
            cmd_args = shlex.split(cmd_str)
        print(f"Parsed args: {cmd_args}")
        print(f"File exists check: {os.path.exists(cmd_args[0])}")
        
        process = subprocess.Popen(
            cmd_args,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True
        )
        
        # Read a few lines to test
        for i, line in enumerate(process.stdout):
            print(f"Output line {i}: {line.strip()}")
            if i >= 3:  # Just read a few lines
                break
                
        process.terminate()
        print("SUCCESS: Subprocess worked!")
        
    except Exception as e:
        print(f"ERROR: {e}")
        print(f"Exception type: {type(e)}")

if __name__ == "__main__":
    # Test the command that's failing
    cmd = r"D:\Repos\OpenAvnu\build-intel-hal\thirdparty\gptp\RelWithDebInfo\gptp.exe -h"
    test_subprocess_call(cmd)
