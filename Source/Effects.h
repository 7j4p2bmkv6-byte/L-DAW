#pragma once
#include "Theme.h"

//==============================================================================
// Built-in effects (all from Tracktion Engine) + helpers for inserts and sends
//==============================================================================
namespace ldaw::fx
{
    enum Type
    {
        reverb = 1, delay, chorus, phaser, compressor, maximizer, equaliser, filter, numTypes
    };

    juce::String typeName (int type);

    // Creates a configured effect (does not insert it anywhere)
    te::Plugin::Ptr create (te::Edit&, int type);

    // Name shown in the mixer (e.g. "Maximizer" for the compressor preset)
    juce::String displayName (te::Plugin&);

    // True for user-visible insert effects (not instrument / volume / meter / send / return)
    bool isInsert (te::Plugin&);

    // Where a new insert should go (before the fader/meter of the chain)
    int insertIndex (te::PluginList&);

    // Insert an effect into a chain; returns the plug-in
    te::Plugin::Ptr addInsert (te::Edit&, te::PluginList&, int type);
    te::Plugin::Ptr addInsert (te::Edit&, te::PluginList&, const juce::PluginDescription&);

    juce::Array<te::Plugin*> getInserts (te::PluginList&);

    // Sets the first parameter whose name contains `nameContains` (0..1 normalised)
    void setParamNorm (te::Plugin&, const juce::String& nameContains, float normalised);

    // Makes an effect 100 % wet (used on send-FX returns)
    void makeFullyWet (te::Plugin&);

    // Sends
    te::AuxSendPlugin* findSend (te::AudioTrack&, int bus);
    float getSendDb (te::AudioTrack&, int bus);          // -60 = off
    void  setSendDb (te::Edit&, te::AudioTrack&, int bus, float db);

    // Creates the four send-FX return tracks if they don't exist yet
    void ensureReturnTracks (te::Edit&);

    // Ensures there's a level meter at the end of the master chain and returns it
    te::LevelMeterPlugin* ensureMasterMeter (te::Edit&);

    void openWindow (te::Plugin&);
    void showAddEffectMenu (te::Edit&, te::PluginList&, juce::Component* target, std::function<void()> onDone);
}
