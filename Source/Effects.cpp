#include "Effects.h"

namespace ldaw::fx
{
    static const juce::Identifier fxNameProp { "ldawFxName" };

    juce::String typeName (int type)
    {
        switch (type)
        {
            case reverb:     return "Reverb";
            case delay:      return "Delay";
            case chorus:     return "Chorus";
            case phaser:     return "Phaser";
            case compressor: return "Compressor";
            case maximizer:  return "Maximizer";
            case equaliser:  return "EQ";
            case filter:     return "Filter";
            default:         return {};
        }
    }

    static const char* xmlTypeFor (int type)
    {
        switch (type)
        {
            case reverb:     return te::ReverbPlugin::xmlTypeName;
            case delay:      return te::DelayPlugin::xmlTypeName;
            case chorus:     return te::ChorusPlugin::xmlTypeName;
            case phaser:     return te::PhaserPlugin::xmlTypeName;
            case compressor: return te::CompressorPlugin::xmlTypeName;
            case maximizer:  return te::CompressorPlugin::xmlTypeName;
            case equaliser:  return te::EqualiserPlugin::xmlTypeName;
            case filter:     return te::LowPassPlugin::xmlTypeName;
            default:         return nullptr;
        }
    }

    void setParamNorm (te::Plugin& p, const juce::String& nameContains, float normalised)
    {
        for (int i = 0; i < p.getNumAutomatableParameters(); ++i)
        {
            te::AutomatableParameter::Ptr param (p.getAutomatableParameter (i));
            if (param != nullptr && param->getParameterName().containsIgnoreCase (nameContains))
            {
                param->setNormalisedParameter (juce::jlimit (0.0f, 1.0f, normalised), juce::sendNotification);
                return;
            }
        }
    }

    te::Plugin::Ptr create (te::Edit& edit, int type)
    {
        auto* xml = xmlTypeFor (type);
        if (xml == nullptr) return {};

        auto p = edit.getPluginCache().createNewPlugin (xml, {});
        if (p == nullptr) return {};

        p->state.setProperty (fxNameProp, typeName (type), nullptr);

        if (type == delay)
        {
            if (auto* d = dynamic_cast<te::DelayPlugin*> (p.get()))
            {
                // 1/8 note at the current tempo
                double bpm = 120.0;
                if (auto t = edit.tempoSequence.getTempo (0)) bpm = t->getBpm();
                d->lengthMs = (int) std::round (30000.0 / bpm);
            }
            setParamNorm (*p, "feedback", 0.55f);
            setParamNorm (*p, "mix", 0.3f);
        }
        else if (type == maximizer)
        {
            // Fast, hard-knee "brickwall-style" compressor preset with make-up gain
            setParamNorm (*p, "thresh",  0.70f);
            setParamNorm (*p, "ratio",   1.00f);
            setParamNorm (*p, "attack",  0.00f);
            setParamNorm (*p, "release", 0.12f);
            setParamNorm (*p, "output",  0.62f);
        }
        return p;
    }

    juce::String displayName (te::Plugin& p)
    {
        auto n = p.state.getProperty (fxNameProp).toString();
        return n.isNotEmpty() ? n : p.getName();
    }

    bool isInsert (te::Plugin& p)
    {
        return dynamic_cast<te::VolumeAndPanPlugin*> (&p) == nullptr
            && dynamic_cast<te::LevelMeterPlugin*>   (&p) == nullptr
            && dynamic_cast<te::AuxSendPlugin*>      (&p) == nullptr
            && dynamic_cast<te::AuxReturnPlugin*>    (&p) == nullptr
            && ! (dynamic_cast<te::ExternalPlugin*>  (&p) != nullptr && p.isSynth());
    }

    int insertIndex (te::PluginList& list)
    {
        int i = 0;
        for (auto p : list.getPlugins())
        {
            if (dynamic_cast<te::VolumeAndPanPlugin*> (&*p) != nullptr
                || dynamic_cast<te::LevelMeterPlugin*> (&*p) != nullptr
                || dynamic_cast<te::AuxSendPlugin*> (&*p) != nullptr)
                return i;
            ++i;
        }
        return i;
    }

    juce::Array<te::Plugin*> getInserts (te::PluginList& list)
    {
        juce::Array<te::Plugin*> out;
        for (auto p : list.getPlugins())
            if (isInsert (*p))
                out.add (&*p);
        return out;
    }

    te::Plugin::Ptr addInsert (te::Edit& edit, te::PluginList& list, int type)
    {
        auto p = create (edit, type);
        if (p != nullptr)
        {
            edit.getUndoManager().beginNewTransaction();
            list.insertPlugin (p, insertIndex (list), nullptr);
        }
        return p;
    }

    te::Plugin::Ptr addInsert (te::Edit& edit, te::PluginList& list, const juce::PluginDescription& desc)
    {
        auto p = edit.getPluginCache().createNewPlugin (te::ExternalPlugin::xmlTypeName, desc);
        if (p != nullptr)
        {
            edit.getUndoManager().beginNewTransaction();
            list.insertPlugin (p, insertIndex (list), nullptr);
        }
        return p;
    }

    void makeFullyWet (te::Plugin& p)
    {
        if (dynamic_cast<te::ReverbPlugin*> (&p) != nullptr)
        {
            setParamNorm (p, "dry", 0.0f);
            setParamNorm (p, "wet", 0.8f);
            setParamNorm (p, "room", 0.75f);
        }
        else
        {
            setParamNorm (p, "mix", 1.0f);
        }
    }

    //--------------------------------------------------------------------------
    te::AuxSendPlugin* findSend (te::AudioTrack& t, int bus)
    {
        for (auto p : t.pluginList.getPlugins())
            if (auto* s = dynamic_cast<te::AuxSendPlugin*> (&*p))
                if (s->busNumber.get() == bus)
                    return s;
        return nullptr;
    }

    float getSendDb (te::AudioTrack& t, int bus)
    {
        if (auto* s = findSend (t, bus))
            return s->getGainDb();
        return -60.0f;
    }

    void setSendDb (te::Edit& edit, te::AudioTrack& t, int bus, float db)
    {
        auto* s = findSend (t, bus);

        if (db <= -59.9f)
        {
            if (s != nullptr)
                s->deleteFromParent();
            return;
        }

        if (s == nullptr)
        {
            auto p = edit.getPluginCache().createNewPlugin (te::AuxSendPlugin::xmlTypeName, {});
            s = dynamic_cast<te::AuxSendPlugin*> (p.get());
            if (s == nullptr) return;
            s->busNumber = bus;
            t.pluginList.insertPlugin (p, t.pluginList.getPlugins().size(), nullptr);   // post-fader
        }

        s->setGainDb (db);
    }

    void ensureReturnTracks (te::Edit& edit)
    {
        static const int defaults[numSendBuses] = { reverb, delay, chorus, phaser };

        for (int bus = 0; bus < numSendBuses; ++bus)
        {
            bool exists = false;
            for (auto* t : te::getAudioTracks (edit))
                if (isReturnTrack (*t) && returnBusOf (*t) == bus)
                    exists = true;
            if (exists) continue;

            const int count = te::getAudioTracks (edit).size();
            edit.ensureNumberOfAudioTracks (count + 1);
            auto* t = te::getAudioTracks (edit).getLast();
            if (t == nullptr) continue;

            t->state.setProperty (returnBusProp, bus, nullptr);
            t->setName ("FX " + juce::String::charToString ((juce::juce_wchar) ('A' + bus)) + " " + typeName (defaults[bus]));

            auto ret = edit.getPluginCache().createNewPlugin (te::AuxReturnPlugin::xmlTypeName, {});
            if (auto* r = dynamic_cast<te::AuxReturnPlugin*> (ret.get()))
            {
                r->busNumber = bus;
                t->pluginList.insertPlugin (ret, 0, nullptr);
            }

            if (auto effect = create (edit, defaults[bus]))
            {
                makeFullyWet (*effect);
                t->pluginList.insertPlugin (effect, insertIndex (t->pluginList), nullptr);
            }
        }
    }

    te::LevelMeterPlugin* ensureMasterMeter (te::Edit& edit)
    {
        auto& list = edit.getMasterPluginList();
        for (auto p : list.getPlugins())
            if (auto* m = dynamic_cast<te::LevelMeterPlugin*> (&*p))
                return m;

        auto p = edit.getPluginCache().createNewPlugin (te::LevelMeterPlugin::xmlTypeName, {});
        if (p == nullptr) return nullptr;
        list.insertPlugin (p, list.getPlugins().size(), nullptr);
        return dynamic_cast<te::LevelMeterPlugin*> (p.get());
    }

    void openWindow (te::Plugin& p)
    {
        if (p.windowState != nullptr)
            p.windowState->showWindowExplicitly();
    }

    void showAddEffectMenu (te::Edit& edit, te::PluginList& list, juce::Component* target, std::function<void()> onDone)
    {
        juce::PopupMenu m;
        for (int t = reverb; t < numTypes; ++t)
            m.addItem (t, typeName (t));

        juce::Array<juce::PluginDescription> vsts;
        for (auto& d : edit.engine.getPluginManager().knownPluginList.getTypes())
            if (! d.isInstrument)
                vsts.add (d);

        if (! vsts.isEmpty())
        {
            juce::PopupMenu sub;
            for (int i = 0; i < vsts.size(); ++i)
                sub.addItem (1000 + i, vsts[i].name + "  (" + vsts[i].manufacturerName + ")");
            m.addSeparator();
            m.addSubMenu ("VST3 effects", sub);
        }

        te::Edit* e = &edit;
        te::PluginList* l = &list;

        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
                         [e, l, vsts, onDone] (int r)
        {
            te::Plugin::Ptr p;
            if (r >= reverb && r < numTypes)               p = addInsert (*e, *l, r);
            else if (r >= 1000 && r - 1000 < vsts.size())  p = addInsert (*e, *l, vsts[r - 1000]);

            if (p != nullptr) openWindow (*p);
            if (onDone) onDone();
        });
    }
}
