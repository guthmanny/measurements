#pragma once

#include <juce_atom_theme/juce_atom_theme.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

/**
    The audio-effect-specific Footer.

    Inherits the generic atom::FooterBar layout skeleton and registers this
    framework's own child controls (MIDI port button, QUALITY / VIEW labels and
    combo boxes) via addItem().  It owns the zoom callback that maps the VIEW
    combo selection to a scale factor.
*/
class EffectFooterComponent final : public atom::FooterBar
{
 public:
  EffectFooterComponent();

  atom::ShapeButton& getBtnMidiPort() { return btnMidiPort; }
  atom::ComboBox& getQualityComboBox() { return qualityComboBox; }
  atom::ComboBox& getViewComboBox() { return viewComboBox; }

  std::function<void(float)> onZoomChanged;

 private:
  void applyFonts();

  atom::ShapeButton btnMidiPort{"btnMidiPort", AtomIconLibrary::Icon::MidiPort};
  atom::Label qualityLabel{"qualityLabel", "QUALITY:"};
  atom::ComboBox qualityComboBox;
  atom::Label viewLabel{"viewLabel", "VIEW:"};
  atom::ComboBox viewComboBox;
};
