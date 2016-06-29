using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Web2DEngineSdk;

namespace Web2DEngineSdk
{
    public interface IImageStreamHandler
    {
        bool OnImageString(string siteId, string pageId, string seriesId, int pagePosition, int cellPos, byte[] buffer, int len);
        bool OnImageChangeMouseCursor(string siteId, string pageId, string seriesId, int pagePosition, int cellPos, McsfCommunication.MouseCursorType cursorType);
        bool OnImageNoteStatusChanged(string siteId, string pageId, string seriesId, int pagePosition, int cellPos, NoteStatusInformation noteStatus);
    }
}
