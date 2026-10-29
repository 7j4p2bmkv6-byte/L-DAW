# L-DAW 0.2 – Phase 1 + 2

A small DAW built on JUCE + Tracktion Engine (GPL, private use).

## Arrange
- Transport: Space play/stop, Enter return to zero, R record, Loop 1-32 bars, Click, BPM
- VST3 instrument per track (JNO-600, DR-1010, ...), Edit opens the plug-in panel
- MIDI keyboard plays the selected track; R arms it for recording
- Clips: drag = move (snaps to beat, Shift = 1/16), drag to other track = move there,
  Ctrl/Alt-drag = copy, drag right edge = resize, double-click empty lane = new 1-bar clip,
  double-click clip = open editor, Del = delete, Ctrl+D = duplicate, right-click = menu
- Ctrl+Z / Ctrl+Y undo/redo, Ctrl+E show/hide editor

## Piano roll
Click = add note (drag to set length), drag note = move, drag edge = resize,
Shift-drag = velocity, right-click = delete, Alt = fine (no snap). Grid 1/4-1/32.

## Step editor (drums)
16 steps per bar, < > to change bar. Click/drag to paint, Shift = accent.
Right-click a drum name to change its MIDI note (default = General MIDI map).
Clips on tracks with "DR" / "drum" in the name or plug-in open here automatically.

## Build
Upload everything (incl. hidden .github folder) to a private GitHub repo.
Actions -> "Build L-DAW" -> artifact L-DAW-Windows -> L-DAW-0.2.0-Windows-Setup.exe
