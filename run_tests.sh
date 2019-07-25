#!/bin/bash
set -e
ninja
for f in build/test/*; do echo && echo "Running $f..." && $f; done
