#pragma once
// L-DAW 0.4
#include "Theme.h"

//==============================================================================
// Audio tracks: import, tempo-follow (time-stretch), copying, inputs
//==============================================================================
namespace ldaw::audio
{
    juce::String fileWildcard();
    bool isAudioFile (const juce::File&);

    // Reads "120bpm" / "120 BPM" / "_120_" style tempos from a file name
    double guessBpm (const juce::File&, double fallback);

    // Inserts a file on an audio track; returns the clip (or nullptr)
    te::WaveAudioClip* importFile (te::AudioTrack&, const juce::File&, te::TimePosition start,
                                   double sourceBpm, bool followTempo);

    // Turns tempo-follow (time-stretch) on/off for a clip
    void setTempoFollow (te::AudioClipBase&, double sourceBpm, bool follow);
    double getSourceBpm (te::Clip&);

    // Copies a MIDI or audio clip to a track at a new start
    te::Clip* copyClipTo (te::Clip&, te::AudioTrack& dest, te::TimePosition start);

    juce::StringArray getWaveInputNames (te::Engine&);
}
