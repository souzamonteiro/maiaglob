#!/bin/bash
# Synchronize the published dependency graph before downstream validation.
set -e
cd "$(dirname "$0")"
git pull --ff-only
git pull --ff-only --recurse-submodules
git submodule init
git submodule update --remote --recursive
