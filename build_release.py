#!/usr/bin/env python3

import sys
import os
import shutil

if sys.version_info[0] < 3:
    print("Python 3 is required to run the script and not Python "+str(sys.version_info[0]))
    sys.exit(1)

def fatal_error(message):
    sys.stderr.write(message+"\n")
    sys.stderr.write("\n\nBUILD FAILED!!! LOOK AT MESSAGES ABOVE\n\n")
    sys.exit(1)

def run_command(command):
    if os.system(command):
        fatal_error("Failed to execute '"+command+"'")

def check_cd(path):
    if os.chdir(path):
        fatal_error("Failed to cd into '"+path+"'")

project_home = os.path.dirname(os.path.realpath(__file__))

if os.path.isdir("build_release"):
    shutil.rmtree("build_release")
os.makedirs("build_release")
check_cd("build_release")
run_command("cmake -DCMAKE_BUILD_TYPE=Release ..")
run_command("make -j8")
check_cd(project_home)
shutil.copyfile("build_release/jni/libJNIBoofCPP.dylib","jni/src/main/resources/natives/");

