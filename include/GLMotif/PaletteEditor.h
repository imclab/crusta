/***********************************************************************
PaletteEditor - Class to represent a GLMotif popup window to edit
one-dimensional transfer functions with RGB color and opacity.
Copyright (c) 2005-2007 Oliver Kreylos

This file is part of the 3D Data Visualizer (Visualizer).

The 3D Data Visualizer is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The 3D Data Visualizer is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the 3D Data Visualizer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef PALETTEEDITOR_INCLUDED
#define PALETTEEDITOR_INCLUDED

#include <Misc/CallbackList.h>
#include <GLMotif/FileSelectionDialog.h>
#include <GLMotif/PopupWindow.h>

#include <GLMotif/ColorMap.h>
#include <GLMotif/ColorPicker.h>
#include <GLMotif/RangeWidget.h>

/* Forward declarations: */
namespace Misc {
class CallbackData;
}
class GLColorMap;
namespace GLMotif {
class Blind;
class TextField;
class Slider;
}

class PaletteEditor:public GLMotif::PopupWindow
    {
    /* Embedded classes: */
    public:
    typedef GLMotif::ColorMap::ValueRange ValueRange;
    typedef GLMotif::ColorMap::ColorMapCreationType ColorMapCreationType;
    typedef GLMotif::ColorMap::Storage Storage;

    class CallbackData:public Misc::CallbackData // Base class for callback data sent by palette editors
        {
        /* Elements: */
        public:
        PaletteEditor* paletteEditor; // Pointer to the palette editor that sent the callback

        /* Constructors and destructors: */
        CallbackData(PaletteEditor* sPaletteEditor)
            :paletteEditor(sPaletteEditor)
            {
            }
        };

    /* Elements: */
    private:
    GLMotif::ColorMap* colorMap; // The color map widget
    GLMotif::RangeWidget* rangeWidget;
    GLMotif::TextField* controlPointValue; // The data value of the currently selected control point
    GLMotif::Blind* colorPanel; // The control point color display widget
    GLMotif::ColorPicker* colorPicker; // The color picker widget

    /* Private methods: */
    void selectedControlPointChangedCallback(Misc::CallbackData* cbData);
    void colorMapChangedCallback(Misc::CallbackData* cbData);
    void rangeChangedCallback(GLMotif::RangeWidget::RangeChangedCallbackData* cbData);
    void colorPickerValueChangedCallback(
        GLMotif::ColorPicker::ColorChangedCallbackData* cbData);
    void removeControlPointCallback(Misc::CallbackData* cbData);
    void loadPaletteCallback(Misc::CallbackData* cbData);
    void savePaletteCallback(Misc::CallbackData* cbData);

    void loadFileOKCallback(
        GLMotif::FileSelectionDialog::OKCallbackData* cbData);
    void loadFileCancelCallback(
        GLMotif::FileSelectionDialog::CancelCallbackData* cbData);


    /* Constructors and destructors: */
    public:
    PaletteEditor(void);

    /* Methods: */
    const GLMotif::ColorMap* getColorMap(void) const // Returns pointer to the underlying color map widget
        {
        return colorMap;
        }
    GLMotif::ColorMap* getColorMap(void) // Ditto
        {
        return colorMap;
        }
    const GLMotif::RangeWidget* getRangeWidget(void) const // Returns pointer to the underlying color map widget
    {
        return rangeWidget;
    }
    GLMotif::RangeWidget* getRangeWidget(void) // Ditto
    {
        return rangeWidget;
    }
    Storage* getPalette(void) const; // Returns the current palette
    void setPalette(const Storage* newPalette); // Sets a new palette
    void createPalette(ColorMapCreationType colorMapType,const ValueRange& newValueRange); // Creates a standard palette
    void createPalette(const std::vector<GLMotif::ColorMap::ControlPoint>& controlPoints); // Creates a palette from the given color map control point vector
    void loadPalette(const char* paletteFileName); // Loads a palette from a palette file
    void savePalette(const char* paletteFileName) const; // Saves the current palette to a palette file
    Misc::CallbackList& getColorMapChangedCallbacks(void) // Returns list of color map change callbacks
        {
        return colorMap->getColorMapChangedCallbacks();
        }
    Misc::CallbackList& getRangeChangedCallbacks(void)
        {
        return rangeWidget->getRangeChangedCallbacks();
        }

    void exportColorMap(GLColorMap& glColorMap) const; // Exports color map to GLColorMap object
    };

#endif
