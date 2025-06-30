# Enhanced Error Handling Testing Guide

This guide demonstrates the enhanced error handling with specific error messages for different failure scenarios.

## 🚨 Error Scenarios to Test

### 1. **File Not Found**
**Test:** Try to load a non-existent file
```bash
# In the application File Browser, enter:
/path/to/nonexistent.ggmlviz
```
**Expected Error:**
```
Error: File not found or access denied.

File: /path/to/nonexistent.ggmlviz

Please check:
• File exists
• File permissions  
• Path is correct
```

### 2. **Invalid File Type**
**Test:** Try to load a non-.ggmlviz file
```bash
# Try loading a text file:
echo "test" > test.txt
# Enter: test.txt
```
**Expected Error:**
```
Error: Invalid file type.

Expected a .ggmlviz trace file.
Selected: test.txt
```

### 3. **Empty File Path**
**Test:** Submit empty path in file browser
**Expected Error:**
```
Error: No file path specified.
```

### 4. **Empty File**
**Test:** Create empty .ggmlviz file
```bash
touch empty.ggmlviz
```
**Expected Error:**
```
Error: Empty trace file.

File: empty.ggmlviz

The trace file contains no data. Please ensure the file was generated correctly.
```

### 5. **File Too Small**
**Test:** Create tiny file
```bash
echo "hi" > tiny.ggmlviz
```
**Expected Error:**
```
Error: Invalid trace file.

File: tiny.ggmlviz

File is too small (3 bytes) to contain valid trace data.
```

### 6. **Invalid Magic Header**
**Test:** Create file with wrong header
```bash
echo "BADMAGIC1234567890" > badheader.ggmlviz
```
**Expected Error:**
```
Error: Invalid trace file format.

File: badheader.ggmlviz

This does not appear to be a valid GGML trace file.
Expected magic header 'GGMLVIZ1', found: 'BADMAGIC'
```

### 7. **Corrupted Data**
**Test:** Create file with valid header but bad data
```bash
# Create a file with valid header but corrupted data
printf "GGMLVIZ1\x00\x00\x00\x01" > corrupted.ggmlviz
# Add some random bytes
dd if=/dev/urandom bs=1 count=50 >> corrupted.ggmlviz 2>/dev/null
```
**Expected Error:**
```
Error: Corrupted trace file.

File: corrupted.ggmlviz

The file header is valid but the trace data appears to be corrupted.
The file may have been truncated or damaged.
```

### 8. **Valid File with No Events**
**Test:** Load a valid but empty trace
*This scenario would show a warning but still load*
**Expected Warning:**
```
Warning: Empty trace data.

File: empty_trace.ggmlviz

The trace file loaded successfully but contains no events.
This might indicate:
• No GGML operations were traced
• Tracing was not enabled
• The model ran but no operations occurred
```

## 🧪 Step-by-Step Testing Instructions

### Setup Test Files:
```bash
cd /Users/willb/Vaults/Personal/DevLab/ggml/ggml-viz/build

# Create test files for each error scenario
echo "test" > test.txt
touch empty.ggmlviz  
echo "hi" > tiny.ggmlviz
echo "BADMAGIC1234567890" > badheader.ggmlviz
printf "GGMLVIZ1\x00\x00\x00\x01" > corrupted.ggmlviz
dd if=/dev/urandom bs=1 count=50 >> corrupted.ggmlviz 2>/dev/null
```

### Test Each Scenario:

1. **Launch application:**
   ```bash
   ./bin/ggml-viz
   ```

2. **Test File Browser:**
   - Click "File" → "Open Trace..."
   - Try each test file path
   - Verify error message appears as expected
   - Click "OK" to dismiss error

3. **Test Command Line Loading:**
   ```bash
   ./bin/ggml-viz test.txt          # Invalid file type
   ./bin/ggml-viz empty.ggmlviz     # Empty file
   ./bin/ggml-viz tiny.ggmlviz      # Too small
   ./bin/ggml-viz badheader.ggmlviz # Bad header
   ./bin/ggml-viz corrupted.ggmlviz # Corrupted data
   ```

## 📋 Error Message Quality Checklist

For each error message, verify:

- [ ] **Clear Error Title** - States what went wrong
- [ ] **File Context** - Shows which file caused the issue
- [ ] **Specific Reason** - Explains exactly why it failed
- [ ] **Actionable Guidance** - Tells user what to do next
- [ ] **Professional Formatting** - Clean layout with line breaks
- [ ] **Technical Details** - Includes relevant technical info (file size, magic header, etc.)

## 🎯 Expected Behavior

### **Error Popup Appearance:**
- [ ] Modal popup appears centered on screen
- [ ] Title bar shows "Error"
- [ ] Message is clearly formatted with line breaks
- [ ] "OK" button dismisses the popup
- [ ] Application remains stable after error

### **Error Message Content:**
- [ ] **Descriptive title** (e.g., "Error: File not found")
- [ ] **File identification** (shows problematic file path)
- [ ] **Root cause explanation** (why it failed)
- [ ] **User guidance** (what to check or try)
- [ ] **Technical details** (when relevant)

### **Application State After Error:**
- [ ] Application doesn't crash
- [ ] File browser can be used again
- [ ] Previous trace (if any) remains loaded
- [ ] All UI elements remain functional

## 🔍 Advanced Testing

### **File Permission Testing:**
```bash
# Create file then remove read permission
echo "test" > noperm.ggmlviz
chmod 000 noperm.ggmlviz
# Try to load - should get access denied error
```

### **Large File Testing:**
```bash
# Create very large file to test memory errors
dd if=/dev/zero bs=1M count=1000 of=huge.ggmlviz
# Try to load - may get out of memory error
```

### **Network File Testing:**
- Try loading file from network drive
- Test behavior with slow/unreliable network

## ✅ Success Criteria

All error scenarios should:
1. **Never crash the application**
2. **Show helpful, specific error messages**
3. **Allow user to try again**
4. **Maintain application stability**
5. **Provide clear next steps**

The enhanced error handling makes the application much more user-friendly and professional!