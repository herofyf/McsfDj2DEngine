using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using McsfCommunication;

namespace Web2DEngineSdk
{
    public enum MouseBehaviorType
    {
        MouseMove = 1,    // 
        MouseDown = 2,    // mouse down for start action
        MouseDoubleClick = 3, // for completion
    }

    public interface IImageInstance
    {
        bool OnHandMode();
        bool OnMouse(int x, int y, bool leftDown, bool rightDown, MouseBehaviorType mouseBehavior);
        bool OnKeyboard(string key, bool ctrlPressed = false, bool shiftPressed = false, bool altPressed = false, KeyboardFeatureKeyType featureKey = KeyboardFeatureKeyType.FK_None);
        bool OnScale(float scaleX, float scaleY);
        bool OnTranslate(float offsetx, float offsety);
        bool OnRotate(float angle);
        bool FlipX();
        bool FlipY();
        bool InvertColor();
        bool SetWinCenterWidth(int center = 0, int width = 0);
        bool UseLineTool();
        bool UseAngleTool();
        bool UseMagnifyGlass();
        bool UseCircleTool();
        bool UseFreeHand();
        bool ResetImage();

        bool UseNoteTool(NoteToolType toolType);
        bool SetActiveNoteProp(NoteObjectInformation noteProp);
    }

    public class ImageInstance : IImageInstance
    {
        int _cellPos;
        StudySeries _parent;

        internal ImageInstance(int cellPos, StudySeries parent)
        {
            _cellPos = cellPos;
            _parent = parent;
        }

        public bool OnHandMode()
        {
            _parent.Image_OnHandMode(_cellPos);

            return true;
        }
        public bool OnScale(float scaleX, float scaleY)
        {
            _parent.Image_onScale(_cellPos, scaleX, scaleY);

            return true;
        }

        public bool OnMouse(int x, int y, bool leftDown, bool rightDown, MouseBehaviorType mouseBehavior)
        {
            _parent.Image_onMouse(_cellPos, x, y, leftDown, rightDown, mouseBehavior);
            return true;
        }

        
        public bool OnKeyboard(string key, bool ctrlPressed = false, bool shiftPressed = false, bool altPressed = false, KeyboardFeatureKeyType featureKey = KeyboardFeatureKeyType.FK_None)
        {
            _parent.Image_onKeyboard(_cellPos, ctrlPressed, shiftPressed, altPressed, key, featureKey);
            return true;
        }

        public bool OnTranslate(float offsetx, float offsety)
        {
            return _parent.Image_OnTranslate(_cellPos, offsetx, offsety);

        }
        public bool OnRotate(float angle)
        {
            return _parent.Image_OnRotate(_cellPos, angle);
        }

        public bool SetWinCenterWidth(int center = 0, int width = 0)
        {
            return _parent.Image_SetWinCenterWidth(_cellPos, center, width);
        }

        public bool UseLineTool()
        {
            return _parent.Image_UseLineTool(_cellPos);
        }

        public bool UseAngleTool()
        {
            return _parent.Image_UseAngleTool(_cellPos);
        }

        public bool UseMagnifyGlass()
        {
            return _parent.Image_MagnifyGlassTool(_cellPos);
        }

        public bool UseCircleTool()
        {
            return _parent.Image_UseCircleTool(_cellPos);
        }

        public bool FlipX()
        {
            return _parent.Image_FlipX(_cellPos);
        }

        public bool FlipY()
        {
            return _parent.Image_FlipY(_cellPos);
        }

        public bool InvertColor()
        {
            return _parent.Image_InvertColor(_cellPos);
        }

        public bool UseFreeHand()
        {
            return _parent.Image_UseFreeHand(_cellPos);
        }

        public bool ResetImage()
        {
            return _parent.Image_ResetImage(_cellPos);
        }

        public bool UseNoteTool(NoteToolType toolType)
        {
            return _parent.Image_UseNoteTool(_cellPos, toolType);
        }

        public bool SetActiveNoteProp(NoteObjectInformation noteProp)
        {
            return _parent.Image_SetNoteProp(_cellPos, noteProp);
        }
    }
}
