#pragma once

#include <JuceHeader.h>
#include "AstralLookAndFeel.h"

/** A draggable modulation-source chip — drag it onto a ModTargetSlider to
    create a routing in the mod matrix (Serum-style). */
class ModSourceChip : public juce::Component
{
public:
    ModSourceChip (int sourceIndexIn, juce::String nameIn)
        : sourceIndex (sourceIndexIn), name (std::move (nameIn))
    {
        setRepaintsOnMouseActivity (true);
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat().reduced (2.0f);
        const bool over = isMouseOverOrDragging();

        g.setColour (AstralLookAndFeel::cyan.withAlpha (over ? 0.35f : 0.12f));
        g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour (AstralLookAndFeel::cyan.withAlpha (over ? 1.0f : 0.6f));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

        g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
        g.drawText (name, bounds, juce::Justification::centred);
    }

    void mouseDrag (const juce::MouseEvent&) override
    {
        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this))
            if (! container->isDragAndDropActive())
                container->startDragging (juce::var (sourceIndex), this);
    }

private:
    int sourceIndex;
    juce::String name;
};

/** A rotary slider that accepts dropped ModSourceChips and reports the
    requested source -> target routing back to the editor. */
class ModTargetSlider : public juce::Slider,
                        public juce::DragAndDropTarget
{
public:
    void setModTarget (int targetIndex) noexcept { modTarget = targetIndex; }

    std::function<void (int sourceIndex, int targetIndex)> onModDrop;

    bool isInterestedInDragSource (const SourceDetails& details) override
    {
        return modTarget > 0 && details.description.isInt();
    }

    void itemDragEnter (const SourceDetails&) override { dragOver = true;  repaint(); }
    void itemDragExit  (const SourceDetails&) override { dragOver = false; repaint(); }

    void itemDropped (const SourceDetails& details) override
    {
        dragOver = false;
        repaint();

        if (onModDrop != nullptr)
            onModDrop ((int) details.description, modTarget);
    }

    void paint (juce::Graphics& g) override
    {
        juce::Slider::paint (g);

        if (dragOver)
        {
            g.setColour (AstralLookAndFeel::cyan);
            g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 6.0f, 2.0f);
        }
    }

private:
    int modTarget = 0;
    bool dragOver = false;
};
