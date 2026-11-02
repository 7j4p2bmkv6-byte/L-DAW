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
        const juce::Colour send        { 0xff36b3a8 };   // teal for send FX
        const juce::Colour selection   { 0xfff2d16b };
    }

    //--------------------------------------------------------------------------
    // Return (send-FX) tracks are ordinary audio tracks marked with a property.
    // They are hidden from the arrange view and appear as FX channels in the mixer.
    //--------------------------------------------------------------------------
    inline const juce::Identifier returnBusProp { "ldawReturnBus" };
    constexpr int numSendBuses = 4;

    inline bool isReturnTrack (const te::Track& t)  { return t.state.hasProperty (returnBusProp); }
    inline int  returnBusOf (const te::Track& t)     { return (int) t.state.getProperty (returnBusProp, -1); }

    inline juce::Array<te::AudioTrack*> getMusicTracks (te::Edit& e)
    {
        juce::Array<te::AudioTrack*> out;
        for (auto* t : te::getAudioTracks (e))
            if (! isReturnTrack (*t))
                out.add (t);
        return out;
    }

    inline juce::Array<te::AudioTrack*> getReturnTracks (te::Edit& e)
    {
        juce::Array<te::AudioTrack*> out;
        for (int bus = 0; bus < numSendBuses; ++bus)
            for (auto* t : te::getAudioTracks (e))
                if (isReturnTrack (*t) && returnBusOf (*t) == bus)
                    out.add (t);
        return out;
    }

    //--------------------------------------------------------------------------
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

    //--------------------------------------------------------------------------
    // Shared note clipboard (piano roll <-> step editor)
    //--------------------------------------------------------------------------
    struct ClipNote { int pitch; double start, length; int velocity; };

    inline std::vector<ClipNote>& noteClipboard()  { static std::vector<ClipNote> v; return v; }
    inline double& noteClipboardSpan()            { static double s = 0.0; return s; }
}
