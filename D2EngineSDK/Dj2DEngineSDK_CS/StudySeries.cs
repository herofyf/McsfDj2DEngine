using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using McsfCommunication;
using UIH.Mcsf.Core;
using Google.ProtocolBuffers;


namespace Web2DEngineSdk
{
    public enum LocalizerLineOperation
    {
        Add = 0,
        Clear = 1,
    }

    public interface IStudySeries
    {
        IImageInstance GetImage(int index);
        bool OpenImage(int imageOffset, bool isAbsOffset, int cellsNum, int width, int height);
        bool CloseSeries();
        void EnableTransSync();
        void DisableTransSync();
        void EnableSeriesPersist();
        void SetLocalizerLinesResId(int resId, bool isReferringSide, LocalizerLineOperation op);
        int GetFirstCellImageIndex();
        int GetCellsCount();
        int GetImagesCount();
        bool ResetImages();
    }

    public class StudySeries : IStudySeries
    {
        internal StudySeries(string siteId, string pageId, string seriesId, int pagePosition)
        {
            _siteId = siteId;
            _pageId = pageId;
            _seriesId = seriesId;
            _pagePosition = pagePosition;
            _lastCellsNum = 0;
            _cellsCount = 0;
            _imagesCount = 0;
            _firstCellImageIndex = 0;
        }

        string _siteId;
        string _pageId;
        string _seriesId;
        int _pagePosition;
        int _lastCellsNum;
        int _cellsCount;
        int _imagesCount;
        int _firstCellImageIndex;

        public IImageInstance GetImage(int index)
        {
            if (index < 0 || index >= _lastCellsNum) return null;
            
            return new ImageInstance(index, this);
        }

        public bool OpenImage(int imageOffset, bool isAbsOffset, int cellsNum, int width, int height)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.OpenImage;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;

            imageBuilder.SeriesPagePos = _pagePosition;

            imageBuilder.ImageCellPos = 0;
            builder.ImagePosId = imageBuilder.Build();

            OpenImageInformation.Builder openImageInformationBuilder = builder.OpenImageInformation.CreateBuilderForType();
            openImageInformationBuilder.Offset = imageOffset;
            openImageInformationBuilder.IsAbsOffset = isAbsOffset;
            openImageInformationBuilder.ImageHeight = height;
            openImageInformationBuilder.ImageWidth = width;
            openImageInformationBuilder.CellsNum = cellsNum;
            builder.OpenImageInformation = openImageInformationBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            _lastCellsNum = cellsNum;
            return true;
        }

        public bool ResetImages()
        {
            return Image_ResetImage(-1);
        }

        public bool CloseSeries()
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.CloseSeries;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId =_siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;

            imageBuilder.SeriesPagePos = _pagePosition;
            
            imageBuilder.ImageCellPos = 0;
            builder.ImagePosId = imageBuilder.Build();


            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }

            Engine2DSdk.Instance().RemoveSeries(_siteId, _pageId, _pagePosition);

            return true;
        }

        public void SetLocalizerLinesResId(int resId, bool isReferringSide, LocalizerLineOperation op)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.ChangeToolType;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = 0;
            builder.ImagePosId = imageBuilder.Build();


            ToolInformation.Builder toolInformationBuilder = builder.ToolInformation.CreateBuilderForType();
            // set tool member
            toolInformationBuilder.ToolType = ImageToolType.LocalizerLines;

            // for member object, need to create it's builder 
            LocalizerLinesResource.Builder localizerLinesBuilder = toolInformationBuilder.LocalizerLinesResource.CreateBuilderForType();
            localizerLinesBuilder.ResourceId = resId;
            localizerLinesBuilder.IsReferringSide = isReferringSide;
            localizerLinesBuilder.Operation = (int)op;
            toolInformationBuilder.LocalizerLinesResource = localizerLinesBuilder.Build();

            builder.ToolInformation = toolInformationBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
           
        }

        public bool Image_onMouse(int cellPos, int x, int y, bool leftDown, bool rightDown, MouseBehaviorType mouseBehavior)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.Mouse;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = cellPos;

            builder.ImagePosId = imageBuilder.Build();


            MouseInformation.Builder mouseBuilder = builder.MouseInformation.CreateBuilderForType();
            mouseBuilder.PointX = x;
            mouseBuilder.PointY = y;

            mouseBuilder.LeftDown = leftDown;
            mouseBuilder.RightDown = rightDown;
            mouseBuilder.MidRollerDown = false;
            mouseBuilder.MidRollerMove = false;
            mouseBuilder.BehaviorType = (int)mouseBehavior;
            builder.MouseInformation = mouseBuilder.Build();
            context.iCommandId = (int)MessageCommandType.Mouse;

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }

        public bool Image_OnHandMode(int cellPos)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.ChangeToolType;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = cellPos;
            builder.ImagePosId = imageBuilder.Build();


            ToolInformation.Builder toolInformationBuilder = builder.ToolInformation.CreateBuilderForType();
            // set tool member
            toolInformationBuilder.ToolType = ImageToolType.HandModeTool;

            // set 
            builder.ToolInformation = toolInformationBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }

        public bool Image_onScale(int cellPos, float scaleX, float scaleY)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.ChangeToolType;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = cellPos;
            builder.ImagePosId = imageBuilder.Build();


            ToolInformation.Builder toolInformationBuilder = builder.ToolInformation.CreateBuilderForType();
            // set tool member
            toolInformationBuilder.ToolType = ImageToolType.ScaleTool;

            // for member object, need to create it's builder 
            ScaleToolInformation.Builder scaleInformationBuilder = toolInformationBuilder.ScaleInformation.CreateBuilderForType();
            scaleInformationBuilder.ScaleXFactor = scaleX;
            scaleInformationBuilder.ScaleYFactor = scaleY;
            toolInformationBuilder.ScaleInformation = scaleInformationBuilder.Build();

            // set 
            builder.ToolInformation = toolInformationBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }

        public bool Image_onKeyboard(int cellPos, bool ctrlPressed, bool shiftPressed, bool altPressed, string key, KeyboardFeatureKeyType featureKey)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.Keyboard;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = cellPos;
            builder.ImagePosId = imageBuilder.Build();


            KeyboardInformation.Builder keyboardInformationBuilder = builder.KeyboardInformation.CreateBuilderForType();
            // set tool member
            keyboardInformationBuilder.KeyVal = key;
            keyboardInformationBuilder.CtrlPressed = ctrlPressed;
            keyboardInformationBuilder.ShiftPressed = shiftPressed;
            keyboardInformationBuilder.AltPressed = altPressed;
            if (featureKey > KeyboardFeatureKeyType.FK_None)
                keyboardInformationBuilder.FeatureKey = featureKey;

            builder.KeyboardInformation = keyboardInformationBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }

            return true;
        }

        public bool Image_OnTranslate(int cellPos, float translatex, float translatey)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.ChangeToolType;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = cellPos;
            builder.ImagePosId = imageBuilder.Build();


            ToolInformation.Builder toolInformationBuilder = builder.ToolInformation.CreateBuilderForType();
            // set tool member
            toolInformationBuilder.ToolType = ImageToolType.TranslateTool;

          
            // set 
            builder.ToolInformation = toolInformationBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }

        public bool Image_OnRotate(int cellPos, float angle)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.ChangeToolType;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = cellPos;
            builder.ImagePosId = imageBuilder.Build();


            ToolInformation.Builder toolInformationBuilder = builder.ToolInformation.CreateBuilderForType();
            // set tool member
            toolInformationBuilder.ToolType = ImageToolType.RotateTool;

            // for member object, need to create it's builder 
            RotateInformation.Builder rotateInformationBuilder = toolInformationBuilder.RotateInformation.CreateBuilderForType();
            rotateInformationBuilder.Angle = angle;

            toolInformationBuilder.RotateInformation = rotateInformationBuilder.Build();

            // set 
            builder.ToolInformation = toolInformationBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }

        public bool Image_SetWinCenterWidth(int cellPos, int nWinCenter, int nWinWidth)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.ChangeToolType;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = cellPos;
            builder.ImagePosId = imageBuilder.Build();


            ToolInformation.Builder toolInformationBuilder = builder.ToolInformation.CreateBuilderForType();
            // set tool member
            toolInformationBuilder.ToolType = ImageToolType.SetWinCenterWidth;

            if (nWinCenter > 0 && nWinWidth > 0)
            {
                WinWidthCenterInformation.Builder winWidthCenterBuilder = toolInformationBuilder.WinWidthCenterInformation.CreateBuilderForType();
                if (winWidthCenterBuilder != null)
                {
                    winWidthCenterBuilder.WinWidth = nWinWidth;
                    winWidthCenterBuilder.WinCenter = nWinCenter;
                    toolInformationBuilder.WinWidthCenterInformation = winWidthCenterBuilder.Build();
                }
            }
           
            // set 
            builder.ToolInformation = toolInformationBuilder.Build();
            
            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }

        public bool Image_UseLineTool(int cellPos)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.ChangeToolType;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = cellPos;
            builder.ImagePosId = imageBuilder.Build();


            ToolInformation.Builder toolInformationBuilder = builder.ToolInformation.CreateBuilderForType();
            // set tool member
            toolInformationBuilder.ToolType = ImageToolType.LineTool;

            // set 
            builder.ToolInformation = toolInformationBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }

        public bool Image_UseAngleTool(int cellPos)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.ChangeToolType;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = cellPos;
            builder.ImagePosId = imageBuilder.Build();


            ToolInformation.Builder toolInformationBuilder = builder.ToolInformation.CreateBuilderForType();
            // set tool member
            toolInformationBuilder.ToolType = ImageToolType.AngleTool;

            // set 
            builder.ToolInformation = toolInformationBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }

        public bool Image_MagnifyGlassTool(int cellPos)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.ChangeToolType;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = cellPos;
            builder.ImagePosId = imageBuilder.Build();


            ToolInformation.Builder toolInformationBuilder = builder.ToolInformation.CreateBuilderForType();
            // set tool member
            toolInformationBuilder.ToolType = ImageToolType.MagnifyGlassTool;

            // set 
            builder.ToolInformation = toolInformationBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }

        public bool Image_UseCircleTool(int _cellPos)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.ChangeToolType;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = _cellPos;
            builder.ImagePosId = imageBuilder.Build();


            ToolInformation.Builder toolInformationBuilder = builder.ToolInformation.CreateBuilderForType();
            // set tool member
            toolInformationBuilder.ToolType = ImageToolType.CircleTool;

            // set 
            builder.ToolInformation = toolInformationBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }

        internal bool Image_FlipX(int _cellPos)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.ChangeToolType;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = _cellPos;
            builder.ImagePosId = imageBuilder.Build();


            ToolInformation.Builder toolInformationBuilder = builder.ToolInformation.CreateBuilderForType();
            // set tool member
            toolInformationBuilder.ToolType = ImageToolType.FlipX;

            // set 
            builder.ToolInformation = toolInformationBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }

        internal bool Image_FlipY(int _cellPos)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.ChangeToolType;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = _cellPos;
            builder.ImagePosId = imageBuilder.Build();


            ToolInformation.Builder toolInformationBuilder = builder.ToolInformation.CreateBuilderForType();
            // set tool member
            toolInformationBuilder.ToolType = ImageToolType.FlipY;

            // set 
            builder.ToolInformation = toolInformationBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }

        internal bool Image_InvertColor(int _cellPos)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.ChangeToolType;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = _cellPos;
            builder.ImagePosId = imageBuilder.Build();


            ToolInformation.Builder toolInformationBuilder = builder.ToolInformation.CreateBuilderForType();
            // set tool member
            toolInformationBuilder.ToolType = ImageToolType.ColorInvert;

            // set 
            builder.ToolInformation = toolInformationBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }

        internal bool Image_UseFreeHand(int _cellPos)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.ChangeToolType;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = _cellPos;
            builder.ImagePosId = imageBuilder.Build();


            ToolInformation.Builder toolInformationBuilder = builder.ToolInformation.CreateBuilderForType();
            // set tool member
            toolInformationBuilder.ToolType = ImageToolType.FreeHand;

            // set 
            builder.ToolInformation = toolInformationBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }
       
        private bool Series_EnableTransSync(bool bEnable)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.SeriesSettings;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = 0;
            builder.ImagePosId = imageBuilder.Build();

            SeriesSettingInformation.Builder settingBuilder = builder.SeriesSettingInformation.CreateBuilderForType();
            settingBuilder.EnableTransfSync = bEnable;
            builder.SeriesSettingInformation = settingBuilder.Build();
            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }

        internal bool Image_ResetImage(int cellPos)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.ResetImage;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = cellPos;
            builder.ImagePosId = imageBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }

        internal bool Image_UseNoteTool(int cellPos, NoteToolType toolType)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.ChangeToolType;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = cellPos;
            builder.ImagePosId = imageBuilder.Build();


            ToolInformation.Builder toolInformationBuilder = builder.ToolInformation.CreateBuilderForType();
            // set tool member
            toolInformationBuilder.ToolType = ImageToolType.NoteTool;

            McsfCommunication.NoteObjectInformation.Builder noteInforBuilder = toolInformationBuilder.NoteObjectInformation.CreateBuilderForType();

            if (toolType == NoteToolType.TextNote)
                noteInforBuilder.NoteType = McsfCommunication.NoteToolType.NoteToolTextNote;
            else if (toolType == NoteToolType.ArrowNote)
                noteInforBuilder.NoteType = McsfCommunication.NoteToolType.NoteToolArrowNote;
            else
                return false;
            toolInformationBuilder.NoteObjectInformation = noteInforBuilder.Build();

            // set 
            builder.ToolInformation = toolInformationBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }

        internal bool Image_SetNoteProp(int cellPos, NoteObjectInformation noteObjectInformation)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.ChangeToolType;

            ImageRequest.Builder builder = new ImageRequest.Builder();
            if (builder == null) return false;

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            if (imageBuilder == null) return false;

            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = _pageId;
            imageBuilder.SeriesId = _seriesId;
            imageBuilder.SeriesPagePos = _pagePosition;
            imageBuilder.ImageCellPos = cellPos;
            builder.ImagePosId = imageBuilder.Build();

            ToolInformation.Builder toolInformationBuilder = builder.ToolInformation.CreateBuilderForType();
            
            // set tool member
            toolInformationBuilder.ToolType = ImageToolType.SetNoteProp;

            McsfCommunication.NoteObjectInformation.Builder noteInforBuilder = toolInformationBuilder.NoteObjectInformation.CreateBuilderForType();
            

            McsfCommunication.MetaTextInformation.Builder noteTextInfoBuilder = noteInforBuilder.NoteTextInf.CreateBuilderForType();
            noteTextInfoBuilder.Text = noteObjectInformation.NoteTextInf.Text;
            noteTextInfoBuilder.FontColor = noteObjectInformation.NoteTextInf.FontColor.ToArgb();
            noteTextInfoBuilder.FontStyle = (int)noteObjectInformation.NoteTextInf.FontStyle;

            //byte[] bytes = Encoding.Default.GetBytes();
            //noteTextInfoBuilder.FontName = Encoding.UTF8.GetString(bytes);
            noteTextInfoBuilder.FontName = noteObjectInformation.NoteTextInf.FontName;
            noteTextInfoBuilder.FontSize = noteObjectInformation.NoteTextInf.FontSize;
            noteInforBuilder.NoteTextInf = noteTextInfoBuilder.Build();

            McsfCommunication.MetaLineInformation.Builder noteLineInfoBuilder = noteInforBuilder.ArrowNoteLineInf.CreateBuilderForType();
            noteLineInfoBuilder.LineColor = noteObjectInformation.NoteArrowLineInf.LineColor.ToArgb();
            noteLineInfoBuilder.LineWidth = noteObjectInformation.NoteArrowLineInf.LineWidth;
            noteInforBuilder.ArrowNoteLineInf = noteLineInfoBuilder.Build();
           

            toolInformationBuilder.NoteObjectInformation = noteInforBuilder.Build();

            // set 
            builder.ToolInformation = toolInformationBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
            return true;
        }
        private bool Series_EnableSeriesPersist(bool bEnable)
        {
            return true;
        }
        
        public void EnableTransSync()
        {
            Series_EnableTransSync(true);
        }
        public void DisableTransSync()
        {
            Series_EnableTransSync(false);
        }
        public void EnableSeriesPersist()
        {
           
        }

        public int GetFirstCellImageIndex()
        {
            return _firstCellImageIndex;
        }

        public int GetCellsCount()
        {
            return _cellsCount;
        }

        public int GetImagesCount()
        {
            return _imagesCount;
        }


        internal void SetSeriesStatus(McsfCommunication.ReportSeriesStatusArgs seriesStatus)
        {
            _firstCellImageIndex = seriesStatus.FirstShownImageIndex;
            _cellsCount = seriesStatus.CellsNum;
            _imagesCount = seriesStatus.ImagesCount;
        }
    }
}
