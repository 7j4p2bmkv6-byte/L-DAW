#pragma once
#include <JuceHeader.h>

namespace te = tracktion;

namespace ldaw
{
    constexpr int rowHeight    = 58;
    constexpr int rulerHeight  = 26;
    constexpr int headerWidth  = 560;

    namespace colours
    {
        const juce::Colour background  { 0xff1b1c1f };
        const juce::Colour panel       { 0xff26272b };
        const juce::Colour panelLight  { 0xff303137 };
        const juce::Colour accent      { 0xffe8752a };   // vintage orange
        const juce::Colour clip        { 0xff3f8fd2 };
        const juce::Colour text        { 0xffe6e6e6 };
        const juce::Colour dimText     { 0xff9a9ca3 };
        const juce::Colour record      { 0xffd8383a };
        const juce::Colour play        { 0xff3fbf6a };
    }

    inline double beatOf (te::Edit& e, te::TimePosition t)  { return e.tempoSequence.toBeats (t).inBeats(); }
    inline te::TimePosition timeOf (te::Edit& e, double b)  { return e.tempoSequence.toTime (te::BeatPosition::fromBeats (b)); }

    inline te::MidiClip* findMidiClip (te::Edit* e, te::EditItemID id)
    {
        if (e == nullptr || ! id.isValid()) return nullptr;
        return dynamic_cast<te::MidiClip*> (te::findClipForID (*e, id));
    }

    // Copies a MIDI clip (notes + name + length) onto a track at a new start time
    inline te::MidiClip* copyMidiClip (te::MidiClip& src, te::AudioTrack& dest, te::TimePosition start)
    {
        auto& e = src.edit;
        const double startBeat = beatOf (e, start);
        const double lenBeats  = beatOf (e, src.getPosition().getEnd()) - beatOf (e, src.getPosition().getStart());

        auto nc = dest.insertMIDIClip (te::TimeRange (start, timeOf (e, startBeat + lenBeats)), nullptr);
        if (nc == nullptr) return nullptr;

        nc->setName (src.getName());
        nc->getSequence().copyFrom (src.getSequence(), &e.getUndoManager());
        return nc.get();
    }
}
