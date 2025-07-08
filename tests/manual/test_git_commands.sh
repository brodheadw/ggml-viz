#!/bin/bash

# Test script to check git commands for untracking build/ directory

echo "=== Testing git commands to untrack build/ directory ==="
echo

# Step 1: Check if build/ directory files are currently tracked
echo "1. Checking if build/ directory files are tracked in git:"
echo "Command: git ls-files build/"
git ls-files build/
echo "Status: $?"
echo

# Step 2: Test the git rm command (dry run)
echo "2. Testing what 'git rm -r --cached build/' would remove:"
echo "Command: git rm -r --cached build/ --dry-run"
git rm -r --cached build/ --dry-run
echo "Status: $?"
echo

# Step 3: Check .gitignore content
echo "3. Checking if .gitignore contains build/:"
echo "Command: grep -n 'build/' .gitignore"
grep -n 'build/' .gitignore
echo "Status: $?"
echo

# Step 4: Show git status
echo "4. Current git status:"
echo "Command: git status"
git status
echo "Status: $?"
echo

echo "=== Test complete ==="