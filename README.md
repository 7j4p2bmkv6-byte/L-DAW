# L-DAW 0.4 - automation, Reason-style mixer, audio tracks

## Automation
- "A" button on a track -> pick any parameter (instrument, channel EQ/dynamics, inserts, volume/pan) -> lane opens under the track
- Draw: click = add point, drag = move (snaps to 1/16, Alt = free), right-click = delete. "Clear" empties a lane, "X" hides it
- Record: turn on A.Write, press Play and move a knob/fader (plug-in window, mixer, track header). Lanes with new data open automatically
- A.Read switches playback of automation on/off
- Hardware knobs (MIDI CC) are recorded into the MIDI clip when recording MIDI

## Mixer (F3 = below the tracks, F9 = full window)
- Sections DYNAMICS (compressor), EQ (4 bands, gain + freq), INSERTS (4 slots), FX SEND (4 sends), pan, metal fader, LED meter, MUTE/SOLO
- Drag the bar above the mixer to resize; sections fold away automatically when there is little room (SHOW buttons on the left)
- Master: master compressor (thres/ratio/attack/release) + inserts + meter

## Audio tracks
- "+ Track" -> Audio track. "In:" chooses the input, R arms, Rec records (enable inputs under Audio / MIDI)
- Import: "Import audio", double-click an empty part of an audio track, or drag files onto the timeline
- Enter the original BPM (guessed from names like "loop_124bpm.wav") -> "Follow project tempo" time-stretches the clip
- Double-click / right-click an audio clip to change its tempo settings
