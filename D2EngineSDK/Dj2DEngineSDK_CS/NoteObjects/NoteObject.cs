using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Web2DEngineSdk
{
    public enum NoteToolType
    {
        TextNote = 0,
        ArrowNote,
    }
    public class NoteObjectInformation
    {
        public NoteToolType NoteType;
        public LineInformation NoteArrowLineInf;
        public TextInformation NoteTextInf;
    }
}
