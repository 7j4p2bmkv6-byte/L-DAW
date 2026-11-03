#pragma once
// L-DAW 0.4
#include "Theme.h"

//==============================================================================
// Built-in effects (Tracktion Engine) + helpers for inserts, sends and channel strip
//==============================================================================
namespace ldaw::fx
{
    enum Type
    {
        reverb = 1, delay, chorus, phaser, compressor, maximizer, equaliser, filter, numTypes
    };

    juce::String typeName (int type);
    te::Plugin::Ptr create (te::Edit&, int type);
    juce::String displayName (te::Plugin&);

    // User-visible insert effects (not instrument / volume / meter / send / return / channel strip)
    bool isInsert (te::Plugin&);
    int  insertIndex (te::PluginList&);

    te::Plugin::Ptr addInsert (te::Edit&, te::PluginList&, int type);
    te::Plugin::Ptr addInsert (te::Edit&, te::PluginList&, const juce::PluginDescription&);
    juce::Array<te::Plugin*> getInserts (te::PluginList&);

    void setParamNorm (te::Plugin&, const juce::String& nameContains, float normalised);
    te::AutomatableParameter* paramByName (te::Plugin*, const juce::StringArray& nameContains);
    void makeFullyWet (te::Plugin&);

    // Channel-strip dynamics ("dyn") and EQ ("eq"), created on first use
    te::Plugin* findChannelPlugin (te::PluginList&, const juce::String& which);
    te::Plugin* ensureChannelPlugin (te::Edit&, te::PluginList&, const juce::String& which);
    te::AutomatableParameter* eqParam (te::Plugin*, int band /*0 LF, 1 LMF, 2 HMF, 3 HF*/, bool freq);

    // Sends
    te::AuxSendPlugin* findSend (te::AudioTrack&, int bus);
    float getSendDb (te::AudioTrack&, int bus);
    void  setSendDb (te::Edit&, te::AudioTrack&, int bus, float db);

    void ensureReturnTracks (te::Edit&);
    te::LevelMeterPlugin* ensureMasterMeter (te::Edit&);

    void openWindow (te::Plugin&);
    void showAddEffectMenu (te::Edit&, te::PluginList&, juce::Component* target, std::function<void()> onDone);
}
