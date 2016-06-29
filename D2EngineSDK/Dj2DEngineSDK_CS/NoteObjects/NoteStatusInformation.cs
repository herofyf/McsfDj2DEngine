using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;

namespace Web2DEngineSdk
{
    public enum NoteStatusType
    {
        ArrowNoteLineComp = 0,
        NoteDoubleClick,
        NoteActive
    }

    public class NoteStatusInformation
    {
        public NoteStatusType StatusType;
        public NoteObjectInformation NoteInf;

        public bool SetValues(McsfCommunication.ReportNoteStatusArgs mcsStatusInf)
        {
            if (mcsStatusInf == null)
                return false;

            switch (mcsStatusInf.NoteStateType)
            {
                case McsfCommunication.NoteStatusType.NoteActive:
                    StatusType = NoteStatusType.NoteActive;
                    break;
                case McsfCommunication.NoteStatusType.ArrowNoteLineComp:
                    StatusType = NoteStatusType.ArrowNoteLineComp;
                    break;
                case McsfCommunication.NoteStatusType.NoteDoubleClick:
                    StatusType = NoteStatusType.NoteDoubleClick;
                    break;
                default:
                    return false;
            }

            if (mcsStatusInf.HasNoteObject == false)
                return false;

            NoteInf = new NoteObjectInformation();
            McsfCommunication.NoteObjectInformation noteObjInf = mcsStatusInf.NoteObject;
            switch (noteObjInf.NoteType)
            {
                case McsfCommunication.NoteToolType.NoteToolTextNote:
                    {
                        NoteInf.NoteType = Web2DEngineSdk.NoteToolType.TextNote;
                    }
                    break;
                case McsfCommunication.NoteToolType.NoteToolArrowNote:
                    {
                        NoteInf.NoteType = Web2DEngineSdk.NoteToolType.ArrowNote;
                    }
                    break;
                default:
                    return false;
            }

            if (noteObjInf.HasArrowNoteLineInf == true)
            {
                NoteInf.NoteArrowLineInf = new LineInformation();

                NoteInf.NoteArrowLineInf.LineWidth = noteObjInf.ArrowNoteLineInf.LineWidth;
                NoteInf.NoteArrowLineInf.LineColor = Color.FromArgb((int)noteObjInf.ArrowNoteLineInf.LineColor);
            }

            if (noteObjInf.HasNoteTextInf == true)
            {
                NoteInf.NoteTextInf = new TextInformation();

                NoteInf.NoteTextInf.Text = noteObjInf.NoteTextInf.Text;
                NoteInf.NoteTextInf.FontSize = noteObjInf.NoteTextInf.FontSize;
                NoteInf.NoteTextInf.FontName = noteObjInf.NoteTextInf.FontName;
                NoteInf.NoteTextInf.FontStyle = (TextFontStyle)noteObjInf.NoteTextInf.FontStyle;
                NoteInf.NoteTextInf.FontColor = Color.FromArgb((int)noteObjInf.NoteTextInf.FontColor);
            }
            return true;
        }
    }
}
