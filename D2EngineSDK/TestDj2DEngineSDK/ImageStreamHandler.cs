using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Web2DEngineSdk;
using System.Drawing;

namespace Test2DEngineSdk
{
    public class ImageStreamHandler : IImageStreamHandler
    {
        public delegate void ShowImageDelegate(Image obj, int index);
        public delegate void ChangeImageMouseCursor(int index, McsfCommunication.MouseCursorType cursorType);
        public delegate void ShowChangedNoteStatus(string siteId, string pageId, string seriesId, int pagePosition, int cellPos, NoteStatusInformation noteStatus);

        Form1 _parent;
        public ImageStreamHandler(Form1 f)
        {
            _parent = f;

        }
        public bool OnImageString(string siteId, string pageId, string seriesId, int pagePosition, int cellPos, byte[] buffer, int len)
        {
            if (_parent.isTestThreads)
            {
                _parent.StopTaskTick(siteId, seriesId);
            }
            else
            {
                Image obj = _parent.byteArrayToImage(buffer);
                _parent.Invoke(new ShowImageDelegate(_parent.DrawImage), new object[] { obj, cellPos });
            
            }    
            return true;
        }

        public bool OnImageChangeMouseCursor(string siteId, string pageId, string seriesId, int pagePosition, int cellPos, McsfCommunication.MouseCursorType cursorType)
        {
            _parent.Invoke(new ChangeImageMouseCursor(_parent.ChangeCellImageMouseCursor), new object[] { cellPos, cursorType });

            return true;
        }

        public bool OnImageNoteStatusChanged(string siteId, string pageId, string seriesId, int pagePosition, int cellPos, NoteStatusInformation noteStatus)
        {
            _parent.Invoke(new ShowChangedNoteStatus(_parent.ShowChangedNoteStatus), new object[] { siteId, 
                pageId,
                seriesId,
                pagePosition,
                cellPos,
                noteStatus
            });

            return true;
        }
    }
}
