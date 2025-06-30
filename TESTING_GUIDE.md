# GGML Visualizer - Complete Testing Guide

This guide walks you through testing every feature of the GGML Visualizer GUI application.

## Prerequisites

1. **Build the application:**
   ```bash
   cd /Users/willb/Vaults/Personal/DevLab/ggml/ggml-viz/build
   make -j4
   ```

2. **Generate test data:**
   ```bash
   ./bin/test_trace_reader
   ls -la *.ggmlviz
   ```

3. **Launch the application:**
   ```bash
   ./bin/ggml-viz test_trace.ggmlviz
   ```

---

## 🧪 TESTING CHECKLIST

### 1. Application Launch & Basic UI

**✅ Test Launch:**
- [ ] Application opens without crashes
- [ ] Window title shows "GGML Visualizer"
- [ ] Window is resizable (1280x720 default)
- [ ] Menu bar is visible at top

**✅ Test Menu Bar:**
- [ ] **File Menu:**
  - [ ] "Open Trace..." option available
  - [ ] "Close Trace" option (enabled when trace loaded)
  - [ ] "Exit" option closes application
- [ ] **View Menu:**
  - [ ] Timeline checkbox (toggles Timeline View)
  - [ ] Graph checkbox (toggles Graph View)
  - [ ] Tensor Inspector checkbox
  - [ ] Memory View checkbox
  - [ ] Demo Window checkbox
- [ ] **Help Menu:**
  - [ ] About option available

### 2. File Loading & TraceReader Integration

**✅ Test File Browser:**
- [ ] Click "File" → "Open Trace..."
- [ ] File browser dialog appears
- [ ] Can enter file path in text field
- [ ] "Open" and "Cancel" buttons work
- [ ] Error handling for invalid files

**✅ Test Trace Loading:**
- [ ] Load `test_trace.ggmlviz`
- [ ] Status in menu bar shows "Loaded: test_trace.ggmlviz (X events)"
- [ ] No error popups appear
- [ ] All view panels become enabled

**✅ Test Error Handling:**
- [ ] Try loading non-existent file → Error popup appears
- [ ] Try loading invalid file → Proper error message
- [ ] Error popup has "OK" button that closes it

### 3. Timeline View Widget - Visual Timeline Tab

**✅ Launch Timeline Testing:**
- [ ] Click "View" → ensure "Timeline" is checked
- [ ] Timeline View window appears
- [ ] Contains tabs: "Visual Timeline", "Events", "Op Timings"

**✅ Test Visual Timeline Canvas:**
- [ ] Switch to "Visual Timeline" tab
- [ ] Timeline controls visible at top:
  - [ ] Zoom slider (0.1x to 10x)
  - [ ] Scroll slider
  - [ ] "Labels" checkbox
  - [ ] "Threads" checkbox
  - [ ] Duration display

**✅ Test Timeline Rendering:**
- [ ] Time ruler at top with markers
- [ ] Operation blocks rendered as colored rectangles
- [ ] Thread lanes (if "Threads" enabled)
- [ ] Grid lines between thread lanes
- [ ] Operation labels (if "Labels" enabled)

**✅ Test Timeline Interactions:**
- [ ] **Mouse Wheel Zoom:**
  - [ ] Scroll up → Zooms in
  - [ ] Scroll down → Zooms out
  - [ ] Zoom centers around mouse position
- [ ] **Click Selection:**
  - [ ] Click on operation block → Highlights with yellow border
  - [ ] Selection syncs with other tabs
- [ ] **Hover Tooltips:**
  - [ ] Hover over operation → Tooltip appears
  - [ ] Shows: Operation name, Duration, Start time, Thread
- [ ] **Controls:**
  - [ ] Zoom slider adjusts zoom level
  - [ ] Scroll slider moves horizontal position
  - [ ] Labels checkbox toggles text display
  - [ ] Threads checkbox toggles lane separation

### 4. Timeline View Widget - Events Tab

**✅ Test Events List:**
- [ ] Switch to "Events" tab
- [ ] List shows all events with format: "ID: EVENT_TYPE (label)"
- [ ] Event types display correctly (GRAPH_BEGIN, OP_BEGIN, etc.)
- [ ] Large lists use clipping for performance

**✅ Test Event Interactions:**
- [ ] Click on event → Highlights in list
- [ ] Selection syncs with Visual Timeline tab
- [ ] Hover over event → Tooltip with details
- [ ] Scrolling works smoothly through large event lists

### 5. Timeline View Widget - Op Timings Tab

**✅ Test Operation Analysis:**
- [ ] Switch to "Op Timings" tab
- [ ] Three-column table: Operation | Duration | % of Total
- [ ] Operations sorted by duration (longest first)
- [ ] Duration displayed in ms with 3 decimal places
- [ ] Percentage calculations are correct

### 6. Compute Graph Visualization Widget

**✅ Launch Graph Testing:**
- [ ] Click "View" → ensure "Graph" is checked
- [ ] Graph View window appears
- [ ] Shows graph event count at top

**✅ Test Graph Controls:**
- [ ] **Layout Controls:**
  - [ ] "Auto Layout" button reorganizes nodes
  - [ ] "Reset View" button resets zoom/pan
- [ ] **Display Options:**
  - [ ] "Op Types" checkbox toggles operation type display
  - [ ] "Timing" checkbox toggles duration display
- [ ] **Zoom Control:**
  - [ ] Zoom slider (0.1x to 5.0x)

**✅ Test Graph Canvas:**
- [ ] Dark background (25, 25, 25)
- [ ] Grid pattern visible
- [ ] Nodes rendered as rounded rectangles
- [ ] Different colors for different operation types:
  - [ ] Green for ADD/SUB
  - [ ] Red for MUL/DIV
  - [ ] Blue for CONV
  - [ ] Purple for LINEAR
  - [ ] Yellow for SOFTMAX
  - [ ] Cyan for RELU
  - [ ] Light blue for NORM
  - [ ] Gray for UNKNOWN

**✅ Test Graph Node Rendering:**
- [ ] **Node Content (when zoomed in):**
  - [ ] Node label (operation name)
  - [ ] Operation type (if enabled)
  - [ ] Duration timing (if enabled)
- [ ] **Visual States:**
  - [ ] Normal state with operation color
  - [ ] Hover state with brightened color
  - [ ] Selected state with yellow border
- [ ] **Node Layout:**
  - [ ] Auto-layout organizes in layers
  - [ ] Nodes connected with curved lines
  - [ ] Arrows show data flow direction

**✅ Test Graph Interactions:**
- [ ] **Mouse Controls:**
  - [ ] Click on node → Selects with yellow border
  - [ ] Drag empty space → Pans the graph
  - [ ] Mouse wheel → Zooms in/out
  - [ ] Zoom centers around mouse position
- [ ] **Hover Tooltips:**
  - [ ] Hover over node → Tooltip appears
  - [ ] Shows: Node name, Type, Duration, Input/Output counts
- [ ] **Connection Rendering:**
  - [ ] Bezier curves between connected nodes
  - [ ] Arrows point in direction of data flow
  - [ ] Lines appear behind nodes (proper z-order)

### 7. Tensor Inspector Panel

**✅ Test Inspector Functionality:**
- [ ] Click "View" → ensure "Tensor Inspector" is checked
- [ ] Tensor Inspector window appears
- [ ] Shows "No trace loaded" when no file loaded
- [ ] Shows event details when event selected in Timeline

**✅ Test Event Details Display:**
- [ ] Select event in Timeline → Details appear in Inspector
- [ ] Shows: Event type, Timestamp, Thread ID, Label
- [ ] Updates when different events selected
- [ ] Handles events without labels gracefully

### 8. Memory View Panel

**✅ Test Memory View:**
- [ ] Click "View" → ensure "Memory View" is checked
- [ ] Memory View window appears
- [ ] Shows "No trace loaded" when no file loaded
- [ ] Shows placeholder text for future implementation

### 9. Error Handling & Edge Cases

**✅ Test Error Scenarios:**
- [ ] Load corrupted trace file → Error popup
- [ ] Load empty trace file → Graceful handling
- [ ] Close trace (File → Close Trace) → All views reset
- [ ] Resize window → UI adapts properly
- [ ] Extreme zoom levels → No crashes or visual artifacts

**✅ Test UI State Management:**
- [ ] Toggle view panels on/off → State preserved
- [ ] Selection sync between Timeline and Graph views
- [ ] Zoom/pan state preserved when switching tabs
- [ ] Application closes cleanly (File → Exit)

### 10. Performance Testing

**✅ Test with Large Data:**
- [ ] Load largest available trace file
- [ ] Timeline scrolling remains smooth
- [ ] Graph pan/zoom performs well
- [ ] No memory leaks during extended use
- [ ] UI remains responsive during operations

---

## 🎯 Advanced Feature Testing

### Timeline Widget Advanced Features:

**✅ Thread Visualization:**
- [ ] Multiple thread lanes visible
- [ ] Operations color-coded by thread
- [ ] Thread separators clearly visible
- [ ] Thread checkbox toggles lane display

**✅ Zoom & Pan Precision:**
- [ ] Smooth zoom transitions
- [ ] Zoom-to-mouse-cursor functionality
- [ ] Pan boundaries prevent getting lost
- [ ] Reset controls return to sensible defaults

### Graph Widget Advanced Features:

**✅ Layout Algorithm:**
- [ ] Auto-layout creates sensible node arrangement
- [ ] Dependency-based layering works correctly
- [ ] Disconnected nodes handled gracefully
- [ ] Large graphs remain readable

**✅ Visual Quality:**
- [ ] Text scaling with zoom level
- [ ] Connection curves are smooth
- [ ] No visual artifacts at extreme zoom
- [ ] Color consistency across features

---

## 📝 Testing Notes Template

Use this template to document any issues found:

```
**Issue:** [Brief description]
**Steps to Reproduce:** 
1. 
2. 
3. 

**Expected:** [What should happen]
**Actual:** [What actually happens]
**Severity:** [Low/Medium/High/Critical]
```

---

## ✅ Completion Checklist

Mark each major feature area as tested:

- [ ] Application Launch & Basic UI
- [ ] File Loading & TraceReader Integration  
- [ ] Timeline View - Visual Timeline Tab
- [ ] Timeline View - Events Tab
- [ ] Timeline View - Op Timings Tab
- [ ] Compute Graph Visualization
- [ ] Tensor Inspector Panel
- [ ] Memory View Panel
- [ ] Error Handling & Edge Cases
- [ ] Performance Testing

**Overall Assessment:** ⭐⭐⭐⭐⭐ (Rate 1-5 stars)

**Ready for Production:** ✅ Yes / ❌ No

---

**Testing completed by:** [Your name]  
**Date:** [Date]  
**Build version:** [Git commit or version]