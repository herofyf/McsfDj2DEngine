using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using UIH.Mcsf.Core;
using McsfCommunication;
using System.IO;
using Dj2DEngineSdk_CS;

namespace Web2DEngineSdk
{
    public class Engine2DSdk
    {
        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool TerminateProcess(IntPtr hProcess, uint uExitCode);
        
        public static Engine2DSdk _sdkObj = null;
        public static Engine2DSdk Instance()
        {
            if (_sdkObj == null)
                _sdkObj = new Engine2DSdk();

            return _sdkObj;
        }

        CLRContaineeBase _containeeBase = null;
        bool _bInited = false;

        internal CLRContaineeBase GetContaineeBase()
        {
            return _containeeBase;
        }

        string _2dengineProxyName = string.Empty;

        internal string Get2DEngineProxyName()
        {
            return _2dengineProxyName;
        }

        IImageStreamHandler _imageStreamHandler = null;

        internal IImageStreamHandler GetImageSteamHandler()
        {
            return _imageStreamHandler;
        }

        public bool Init(CLRContaineeBase containeeBase, string proxyName, int listenEvtChannelId)
        {
            if (containeeBase == null || proxyName.Length <= 0 || _bInited)
                return _bInited;

            _containeeBase = containeeBase;
            _2dengineProxyName = proxyName;

            _containeeBase.RegisterDataHandler(new MyDataHandler());

            //if (listenEvtChannelId > 0)
            //{
            //    _containeeBase.RegisterEventHandler(listenEvtChannelId, (int)MessageEventType.ReportMouseStatus, new MyImageEventHandler());
            //    _containeeBase.RegisterEventHandler(listenEvtChannelId, (int)MessageEventType.ReportSeriesStatus, new MyImageEventHandler());
            //}

            ICLRCommandHandler commandReportStatusHandler = new MyReportStatusCommandHandler();
            int cmdId = (int)McsfCommunication.MessageCommandType.ReportStatus;
            _containeeBase.RegisterCommandHandler(cmdId, commandReportStatusHandler);

            _bInited = true;
            
            return _bInited;
        }

       
        Dictionary<SeriesLocationId, IStudySeries> _openedSeriesDict = new Dictionary<SeriesLocationId, IStudySeries>();
        private Object _openedSeriesDictLock = new Object();

        // "1.2.840.113704.1.111.7948.1214916604.21"
        public IStudySeries LoadSeries(string siteId, string pageId, string seriesId, int pagePosition)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            if (_containee == null) return null;

            StudySeries studySeries = new StudySeries(siteId, pageId, seriesId, pagePosition);

            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.LoadSeries;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = siteId;
            imageBuilder.PageId = pageId;
            imageBuilder.SeriesId = seriesId;
            imageBuilder.SeriesPagePos = pagePosition;
            imageBuilder.ImageCellPos = 0;

            builder.ImagePosId = imageBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return null;
            }

            RemoveSeries(siteId, pageId, pagePosition);

            KeepSeries(siteId, pageId, pagePosition, studySeries);
            return studySeries;
        }


        public IStudySeries GetSeries(string siteId, string pageId, int pagePosition)
        {
            SeriesLocationId seriesLocId = new SeriesLocationId();
            seriesLocId.SiteId = siteId;
            seriesLocId.PageId = pageId;
            seriesLocId.PagePosition = pagePosition;

            lock (_openedSeriesDictLock)
            {
                if (_openedSeriesDict.ContainsKey(seriesLocId))
                {
                    return _openedSeriesDict[seriesLocId];
                }
            }
            
            return null;
        }

        public bool onWebPageClosing(string siteId, string pageId)
        {
            Dictionary<SeriesLocationId, IStudySeries> deletedItems = new Dictionary<SeriesLocationId, IStudySeries>();

            SeriesLocationId seriesLocId;

            lock (_openedSeriesDictLock)
            {
                foreach (KeyValuePair<SeriesLocationId, IStudySeries> pair in _openedSeriesDict)
                {
                    seriesLocId = pair.Key;
                    if (seriesLocId.SiteId.Equals(siteId) && seriesLocId.PageId.Equals(pageId))
                    {
                        deletedItems[seriesLocId] = pair.Value;
                    }
                }

                foreach (KeyValuePair<SeriesLocationId, IStudySeries> pair in deletedItems)
                {
                    _openedSeriesDict.Remove(pair.Key);
                }
            }

            foreach (KeyValuePair<SeriesLocationId, IStudySeries> pair in deletedItems)
            {
                pair.Value.CloseSeries();
            }

            return true;
        }

        internal void RemoveSeries(string siteId, string pageId, int pagePosition)
        {
            SeriesLocationId seriesLocId = new SeriesLocationId();
            seriesLocId.SiteId = siteId;
            seriesLocId.PageId = pageId;
            seriesLocId.PagePosition = pagePosition;

            lock (_openedSeriesDictLock)
            {
                if (_openedSeriesDict.ContainsKey(seriesLocId))
                {
                    _openedSeriesDict.Remove(seriesLocId);
                }
            }
            
        }

        internal void UpdateSeriesStatus(string siteId, string pageId, int pagePosition, McsfCommunication.ReportSeriesStatusArgs status)
        {
             SeriesLocationId seriesLocId = new SeriesLocationId();
            seriesLocId.SiteId = siteId;
            seriesLocId.PageId = pageId;
            seriesLocId.PagePosition = pagePosition;

            lock (_openedSeriesDictLock)
            {
                if (_openedSeriesDict.ContainsKey(seriesLocId))
                {
                    IStudySeries iStudySeries = _openedSeriesDict[seriesLocId];
                    StudySeries studySeries = iStudySeries as StudySeries;
                    if (studySeries != null)
                    {
                        studySeries.SetSeriesStatus(status);
                    }
                    
                }
            }
        }
        internal void KeepSeries(string siteId, string pageId, int pagePosition, IStudySeries seriesObj)
        {
            SeriesLocationId seriesLocId = new SeriesLocationId();
            seriesLocId.SiteId = siteId;
            seriesLocId.PageId = pageId;
            seriesLocId.PagePosition = pagePosition;

            lock (_openedSeriesDictLock)
            {
                _openedSeriesDict[seriesLocId] = seriesObj;
            }
            
        }

        public ISiteConfiguration GetSiteConfigObj(string siteId)
        {
            return new SiteConfiguration(siteId);
        }

        public void RegisterImageStreamHandler(IImageStreamHandler handler)
        {
            _imageStreamHandler = handler;
        }
    }
}
