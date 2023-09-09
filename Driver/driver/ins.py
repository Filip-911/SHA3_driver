import sys
import subprocess

if len(sys.argv) != 1:
    print("Usage: python script.py")
    sys.exit(1)

module_name = "sha3"

# Load the kernel module
insmod_command = "sudo insmod sha3.ko"
try:
    subprocess.run(insmod_command, shell=True, check=True)
    print("Loaded sha3.ko successfully")
except subprocess.CalledProcessError as e:
    print("Error loading {sha3}.ko: {e}")
    sys.exit(1)

# Set permissions on /dev/argument
chmod_command = "sudo chmod 777 /dev/sha3"
try:
    subprocess.run(chmod_command, shell=True, check=True)
    print("Changed permissions on /dev/{sha3} successfully")
except subprocess.CalledProcessError as e:
    print("Error changing permissions on /dev/{sha3}: {e}")
    sys.exit(1)

