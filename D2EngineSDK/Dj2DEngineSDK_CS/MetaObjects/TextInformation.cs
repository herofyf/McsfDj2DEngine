using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
namespace Web2DEngineSdk
{
    public enum TextFontStyle
    {
        FontStyleRegular    = 0,
        FontStyleBold       = 1,
        FontStyleItalic     = 2,
        FontStyleBoldItalic = 3,
        FontStyleUnderline  = 4,
        FontStyleStrikeout  = 8
    }

    public class TextInformation
    {
        public string Text;
        public string FontName;
        public int FontSize;
        public TextFontStyle FontStyle;
        public Color FontColor;
    }
}
