#include "MixerView.h"

using namespace ldaw;

namespace
{
    constexpr int stripWidth = 112;
    constexpr int maxInsertSlots = 5;

    juce::String sendShortName (te::Edit& e, int bus)
    {
        for (auto* t : getReturnTracks (e))
            if (returnBusOf (*t) == bus)
            {
                auto inserts = fx::getInserts (t->pluginList);
                if (! inserts.isEmpty())
                    return fx::displayName (*inserts.getFirst()).substring (0, 6);
            }
        return juce::String::charToString ((juce::juce_wchar) ('A' + bus));
    }
}

//==============================================================================
// LevelMeter
//==============================================================================
void LevelMeter::attach (te::LevelMeterPlugin* p)
{
    if (p == meterPlugin.get()) return;
    detach();
    if (p == nullptr) return;

    meterPlugin = te::Plugin::Ptr (p);
    measurer = &p->measurer;
    measurer->addClient (client);
}

void LevelMeter::detach()
{
    if (measurer != nullptr)
        measurer->removeClient (client);
    measurer = nullptr;
    meterPlugin = nullptr;
}

void LevelMeter::tick()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        float db = -100.0f;
        if (measurer != nullptr)
            db = client.getAndClearAudioLevel (ch).dB;

        levels[ch] = db > levels[ch] ? db : juce::jmax (-100.0f, levels[ch] - 2.5f);   // fall-off

        if (levels[ch] >= peaks[ch]) { peaks[ch] = levels[ch]; peakHold[ch] = 30; }
        else if (--peakHold[ch] <= 0) peaks[ch] = juce::jmax (-100.0f, peaks[ch] - 1.5f);
    }
    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r, 2.0f);

    auto toY = [&r] (float db)
    {
        const float norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 66.0f);   // -60..+6 dB
        return r.getBottom() - norm * r.getHeight();
    };

    const float w = (r.getWidth() - 3.0f) / 2.0f;
    for (int ch = 0; ch < 2; ++ch)
    {
        const float x = r.getX() + 1.0f + ch * (w + 1.0f);
        const float y = toY (levels[ch]);

        juce::ColourGradient grad (juce::Colour (0xffd8383a), x, toY (3.0f),
                                   juce::Colour (0xff3fbf6a), x, r.getBottom(), false);
        grad.addColour (0.25, juce::Colour (0xfff2d16b));
        g.setGradientFill (grad);
        g.fillRect (x, y, w, r.getBottom() - y);

        g.setColour (peaks[ch] > 0.0f ? juce::Colour (0xffd8383a) : juce::Colours::white.withAlpha (0.8f));
        g.fillRect (x, toY (peaks[ch]), w, 1.5f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawHorizontalLine ((int) toY (0.0f), r.getX(), r.getRight());
}

//==============================================================================
// MixerStrip
//==============================================================================
MixerStrip::MixerStrip (te::Edit& e, te::AudioTrack* t, Kind k, int i)
    : track (t), kind (k), edit (e), index (i)
{
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    nameLabel.setColour (juce::Label::textColourId, kind == Kind::master ? colours::accent : colours::text);
    if (kind == Kind::music)
    {
        nameLabel.setEditable (false, true, false);
        nameLabel.onTextChange = [this] { if (track) track->setName (nameLabel.getText()); };
    }
    addAndMakeVisible (nameLabel);

    if (kind == Kind::music)
    {
        instrumentButton.setColour (juce::TextButton::buttonColourId, colours::clip.darker (0.4f));
        instrumentButton.setTooltip ("Open the instrument panel");
        instrumentButton.onClick = [this]
        {
            if (track == nullptr) return;
            for (auto p : track->pluginList.getPlugins())
                if (dynamic_cast<te::ExternalPlugin*> (&*p) != nullptr && p->isSynth())
                { fx::openWindow (*p); return; }
        };
        addAndMakeVisible (instrumentButton);
    }

    addFxButton.setColour (juce::TextButton::buttonColourId, colours::panelLight);
    addFxButton.setTooltip ("Add an insert effect to this channel");
    addFxButton.onClick = [this]
    {
        fx::showAddEffectMenu (edit, chain(), &addFxButton, [safe = juce::Component::SafePointer<MixerStrip> (this)]
        {
            if (safe != nullptr) safe->rebuildInserts();
        });
    };
    addAndMakeVisible (addFxButton);

    if (kind == Kind::music)
    {
        for (int bus = 0; bus < numSendBuses; ++bus)
        {
            auto* k = sendKnobs.add (new juce::Slider());
            k->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            k->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            k->setRange (-60.0, 6.0, 0.1);
            k->setSkewFactorFromMidPoint (-12.0);
            k->setDoubleClickReturnValue (true, -60.0);
            k->setPopupDisplayEnabled (true, true, nullptr);
            k->setTextValueSuffix (" dB");
            k->setColour (juce::Slider::rotarySliderFillColourId, colours::send);
            k->setValue (track != nullptr ? fx::getSendDb (*track, bus) : -60.0, juce::dontSendNotification);
            k->onValueChange = [this, k, bus]
            {
                if (track != nullptr)
                    fx::setSendDb (edit, *track, bus, (float) k->getValue());
            };
            addAndMakeVisible (k);

            auto* l = sendLabels.add (new juce::Label());
            l->setFont (juce::Font (juce::FontOptions (10.0f)));
            l->setJustificationType (juce::Justification::centred);
            l->setColour (juce::Label::textColourId, colours::send);
            l->setText (sendShortName (edit, bus), juce::dontSendNotification);
            l->setInterceptsMouseClicks (false, false);
            addAndMakeVisible (l);
        }
    }

    pan.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    pan.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    pan.setRange (-1.0, 1.0, 0.01);
    pan.setDoubleClickReturnValue (true, 0.0);
    pan.setPopupDisplayEnabled (true, true, nullptr);
    pan.setColour (juce::Slider::rotarySliderFillColourId, colours::accent);
    pan.onValueChange = [this] { if (auto* v = volume()) v->setPan ((float) pan.getValue()); };
    addAndMakeVisible (pan);

    fader.setSliderStyle (juce::Slider::LinearVertical);
    fader.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);
    fader.setRange (-60.0, 6.0, 0.1);
    fader.setSkewFactorFromMidPoint (-12.0);
    fader.setDoubleClickReturnValue (true, 0.0);
    fader.setTextValueSuffix (" dB");
    fader.setColour (juce::Slider::thumbColourId, kind == Kind::master ? colours::accent
                                                 : kind == Kind::fxReturn ? colours::send : colours::text);
    fader.onValueChange = [this] { if (auto* v = volume()) v->setVolumeDb ((float) fader.getValue()); };
    addAndMakeVisible (fader);

    if (kind != Kind::master)
    {
        for (auto* b : { &muteButton, &soloButton })
        {
            b->setClickingTogglesState (true);
            b->setColour (juce::TextButton::buttonColourId, colours::panelLight);
            addAndMakeVisible (b);
        }
        muteButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffd9a400));
        soloButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff2f9be0));
        muteButton.onClick = [this] { if (track) track->setMute (muteButton.getToggleState()); };
        soloButton.onClick = [this] { if (track) track->setSolo (soloButton.getToggleState()); };
    }

    addAndMakeVisible (meter);
    if (kind == Kind::master)      meter.attach (fx::ensureMasterMeter (edit));
    else if (track != nullptr)     meter.attach (track->getLevelMeterPlugin());

    rebuildInserts();
    tick();
}

te::PluginList& MixerStrip::chain()
{
    return track != nullptr ? track->pluginList : edit.getMasterPluginList();
}

te::VolumeAndPanPlugin* MixerStrip::volume()
{
    if (kind == Kind::master)
        return edit.getMasterVolumePlugin().get();
    return track != nullptr ? track->getVolumePlugin() : nullptr;
}

juce::String MixerStrip::makeSignature()
{
    juce::String s;
    for (auto* p : fx::getInserts (chain()))
        s << fx::displayName (*p) << (p->isEnabled() ? "1" : "0") << "|";
    return s;
}

void MixerStrip::rebuildInserts()
{
    insertButtons.clear();
    auto inserts = fx::getInserts (chain());

    for (int i = 0; i < juce::jmin (maxInsertSlots, inserts.size()); ++i)
    {
        auto* p = inserts[i];
        auto* b = insertButtons.add (new SlotButton (fx::displayName (*p)));
        b->setColour (juce::TextButton::buttonColourId, p->isEnabled() ? colours::panelLight.brighter (0.15f)
                                                                       : colours::background);
        b->setColour (juce::TextButton::textColourOffId, p->isEnabled() ? colours::text : colours::dimText);
        b->setTooltip ("Click = open, right-click = bypass / remove");

        te::Plugin::Ptr ptr (p);
        b->onClick      = [ptr] { fx::openWindow (*ptr); };
        b->onRightClick = [this, ptr, b] { showInsertMenu (ptr.get(), b); };
        addAndMakeVisible (b);
    }

    addFxButton.setEnabled (inserts.size() < maxInsertSlots);
    insertSignature = makeSignature();
    resized();
}

void MixerStrip::showInsertMenu (te::Plugin* p, juce::Component* target)
{
    if (p == nullptr) return;

    juce::PopupMenu m;
    m.addItem (1, "Open");
    m.addItem (2, p->isEnabled() ? "Bypass" : "Enable");
    m.addSeparator();
    m.addItem (3, "Remove");

    te::Plugin::Ptr ptr (p);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (target),
                     [safe = juce::Component::SafePointer<MixerStrip> (this), ptr] (int r)
    {
        if (safe == nullptr || ptr == nullptr) return;
        if (r == 1) fx::openWindow (*ptr);
        if (r == 2) ptr->setEnabled (! ptr->isEnabled());
        if (r == 3)
        {
            safe->edit.getUndoManager().beginNewTransaction();
            ptr->deleteFromParent();
        }
        safe->rebuildInserts();
    });
}

void MixerStrip::tick()
{
    if (kind == Kind::master)
        nameLabel.setText ("MASTER", juce::dontSendNotification);
    else if (track != nullptr && ! nameLabel.isBeingEdited())
        nameLabel.setText (track->getName(), juce::dontSendNotification);

    if (kind == Kind::music && track != nullptr)
    {
        juce::String inst = "- no instrument -";
        for (auto p : track->pluginList.getPlugins())
            if (dynamic_cast<te::ExternalPlugin*> (&*p) != nullptr && p->isSynth())
                inst = p->getName();
        instrumentButton.setButtonText (inst);

        for (int bus = 0; bus < sendKnobs.size(); ++bus)
            if (! sendKnobs[bus]->isMouseButtonDown())
                sendKnobs[bus]->setValue (fx::getSendDb (*track, bus), juce::dontSendNotification);
    }

    if (auto* v = volume())
    {
        if (! fader.isMouseButtonDown()) fader.setValue (v->getVolumeDb(), juce::dontSendNotification);
        if (! pan.isMouseButtonDown())   pan.setValue (v->getPan(), juce::dontSendNotification);
    }

    if (track != nullptr && kind != Kind::master)
    {
        muteButton.setToggleState (track->isMuted (false), juce::dontSendNotification);
        soloButton.setToggleState (track->isSolo (false), juce::dontSendNotification);
    }

    if (makeSignature() != insertSignature)
        rebuildInserts();

    meter.tick();
}

void MixerStrip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f, 0.0f);
    const auto base = kind == Kind::master   ? colours::panel.brighter (0.05f)
                    : kind == Kind::fxReturn ? colours::panel.interpolatedWith (colours::send, 0.08f)
                                             : colours::panel;
    g.setColour (base);
    g.fillRoundedRectangle (r, 4.0f);

    g.setColour (kind == Kind::master ? colours::accent : kind == Kind::fxReturn ? colours::send : colours::clip);
    g.fillRect (r.getX() + 4.0f, 2.0f, r.getWidth() - 8.0f, 3.0f);

    g.setColour (colours::dimText);
    g.setFont (juce::Font (juce::FontOptions (10.0f)));

    const int insertsTop = kind == Kind::music ? 54 : 30;
    g.drawText ("INSERTS", 4, insertsTop, getWidth() - 8, 12, juce::Justification::centred);

    if (kind == Kind::music)
        g.drawText ("SENDS", 4, insertsTop + 14 + (maxInsertSlots + 1) * 22 + 2, getWidth() - 8, 12, juce::Justification::centred);

    if (kind == Kind::music)
    {
        g.setColour (colours::dimText);
        g.drawText (juce::String (index + 1), 4, getHeight() - 18, 20, 14, juce::Justification::left);
    }
}

void MixerStrip::resized()
{
    auto r = getLocalBounds().reduced (6, 6);
    r.removeFromTop (2);

    nameLabel.setBounds (r.removeFromTop (20));

    if (kind == Kind::music)
        instrumentButton.setBounds (r.removeFromTop (24).reduced (0, 2));

    r.removeFromTop (14);   // "INSERTS"
    for (int i = 0; i < maxInsertSlots; ++i)
    {
        auto slot = r.removeFromTop (22).reduced (0, 1);
        if (i < insertButtons.size()) insertButtons[i]->setBounds (slot);
    }
    addFxButton.setBounds (r.removeFromTop (22).reduced (0, 1));

    if (kind == Kind::music)
    {
        r.removeFromTop (14);   // "SENDS"
        auto grid = r.removeFromTop (100);
        const int cw = grid.getWidth() / 2;
        for (int i = 0; i < sendKnobs.size(); ++i)
        {
            juce::Rectangle<int> cell (grid.getX() + (i % 2) * cw, grid.getY() + (i / 2) * 50, cw, 50);
            sendLabels[i]->setBounds (cell.removeFromBottom (12));
            sendKnobs[i]->setBounds (cell.reduced (4, 0));
        }
    }

    r.removeFromTop (4);
    pan.setBounds (r.removeFromTop (40).withSizeKeepingCentre (40, 40));

    if (kind != Kind::master)
    {
        auto ms = r.removeFromBottom (26);
        muteButton.setBounds (ms.removeFromLeft (ms.getWidth() / 2).reduced (2, 1));
        soloButton.setBounds (ms.reduced (2, 1));
    }
    r.removeFromBottom (14);

    meter.setBounds (r.removeFromRight (16).withTrimmedBottom (20));
    fader.setBounds (r);
}

//==============================================================================
// MixerView
//==============================================================================
MixerView::MixerView()
{
    viewport.setViewedComponent (&content, false);
    viewport.setScrollBarsShown (false, true);
    addAndMakeVisible (viewport);
}

void MixerView::setEdit (te::Edit* e)
{
    strips.clear();
    edit = e;
    rebuild();
}

juce::String MixerView::makeTrackSignature()
{
    juce::String s;
    if (edit != nullptr)
        for (auto* t : te::getAudioTracks (*edit))
            s << t->itemID.toString() << ",";
    return s;
}

void MixerView::rebuild()
{
    strips.clear();
    trackSignature = makeTrackSignature();
    if (edit == nullptr) return;

    int i = 0;
    for (auto* t : getMusicTracks (*edit))
        content.addAndMakeVisible (strips.add (new MixerStrip (*edit, t, MixerStrip::Kind::music, i++)));

    for (auto* t : getReturnTracks (*edit))
        content.addAndMakeVisible (strips.add (new MixerStrip (*edit, t, MixerStrip::Kind::fxReturn, i++)));

    content.addAndMakeVisible (strips.add (new MixerStrip (*edit, nullptr, MixerStrip::Kind::master, i++)));

    resized();
}

void MixerView::tick()
{
    if (! isShowing()) return;

    if (makeTrackSignature() != trackSignature)
        rebuild();

    for (auto* s : strips)
        s->tick();
}

void MixerView::paint (juce::Graphics& g)
{
    g.fillAll (colours::background);
}

void MixerView::resized()
{
    viewport.setBounds (getLocalBounds());
    const int h = juce::jmax (560, viewport.getHeight() - viewport.getScrollBarThickness());

    int x = 4;
    int numMusic = 0;
    for (auto* s : strips)
        if (s->kind == MixerStrip::Kind::music) ++numMusic;

    for (int i = 0; i < strips.size(); ++i)
    {
        if (i == numMusic) x += 14;                                  // gap before FX returns
        if (strips[i]->kind == MixerStrip::Kind::master) x += 14;    // gap before master
        strips[i]->setBounds (x, 4, stripWidth, h - 8);
        x += stripWidth;
    }

    content.setSize (x + 4, h);
}
