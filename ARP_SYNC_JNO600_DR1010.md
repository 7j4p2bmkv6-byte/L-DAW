# Making the JNO-600 arpeggiator (and DR-1010 sequencer) follow the DAW tempo

L-DAW already sends tempo and song position to every plug-in through the standard VST3 play head.
The arpeggiator has to *read* it. Replace the internal arp clock in processBlock with this:

```cpp
// --- read host tempo / position --------------------------------------------
double bpm = 120.0, ppq = 0.0;
bool hostPlaying = false;

if (auto* ph = getPlayHead())
    if (auto pos = ph->getPosition())
    {
        if (auto b = pos->getBpm())          bpm = *b;
        if (auto p = pos->getPpqPosition())  ppq = *p;
        hostPlaying = pos->getIsPlaying();
    }

// arp rate as note value: 1/4 = 1.0, 1/8 = 0.5, 1/16 = 0.25, 1/32 = 0.125 (beats)
const double stepBeats      = arpRateInBeats;
const double samplesPerBeat = getSampleRate() * 60.0 / bpm;
const double samplesPerStep = samplesPerBeat * stepBeats;

for (int i = 0; i < numSamples; ++i)
{
    bool newStep = false;

    if (hostPlaying)
    {
        // locked to the DAW grid: steps land exactly on 1/16s of the song
        const auto step = (int64_t) std::floor ((ppq + i / samplesPerBeat) / stepBeats);
        newStep = (step != lastStep);
        lastStep = step;
    }
    else
    {
        // DAW stopped: free-run, but at the DAW tempo
        if ((freePhase += 1.0) >= samplesPerStep) { freePhase -= samplesPerStep; newStep = true; }
        lastStep = -1;
    }

    if (newStep)
        arp.advance (i);   // trigger next arp note at sample offset i
}
```

Add `int64_t lastStep = -1; double freePhase = 0.0;` as members, and change the arp RATE
control from Hz to note values (1/4, 1/8, 1/16, 1/32, plus triplets if wanted).
The same pattern works for the DR-1010 step sequencer.
