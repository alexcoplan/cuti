import subprocess
import sys
import os

def debug(msg):
    sys.stderr.write("%s\n" % msg)

def run(cmd):
    debug("running %s" % " ".join(cmd))
    subprocess.check_call(cmd)

run(["ninja"])
debug("")

test_dir = "build/test"
test_exes = os.listdir(test_dir)

for exe in test_exes:
    run(["%s/%s" % (test_dir, exe)])
    debug("")
