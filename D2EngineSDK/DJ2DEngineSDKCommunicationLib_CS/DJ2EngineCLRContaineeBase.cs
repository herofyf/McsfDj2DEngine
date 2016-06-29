/////////////////////////////////////////////////////////////////////////
/// Copyright, (c) Shanghai United Imaging Healthcare Inc., 2011 
/// All rights reserved. 
/// 
/// Author: Li,Yongwei  yongwei.li@united-imaging.com
///
/// File: CLRContaineeTestFE.cs
///
/// Summary: Sample code of Front End
/// 
/// 
/// Date  2011-11-15 
//////////////////////////////////////////////////////////////////////////

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Google.ProtocolBuffers;
using System.Runtime.Serialization;
using System.Threading;
using System.IO;
using System.Diagnostics;
using System.Collections;
using System.Reflection;
using System.Runtime.InteropServices;
namespace UIH.Mcsf.Core
{
    public class DJ2EngineCLRContaineeBase : CLRContaineeBase
    {
        DJ2EngineCLRContainerBase _containerBase;

        int _eventListenChannel = -1;
        public int EventListenChannel
        {
            get { return _eventListenChannel; }
        }

        string _2dengineProxyName = "";
        public string dj2engineProxyName
        {
            get { return _2dengineProxyName; }
        }
        /// <summary>
        /// If containee has initial work to do, do it here
        /// </summary>
        override public void Startup()
        {
            
        }

        bool _bInited = false;
        public bool Init(string configureFileName)
        {
            if (_containerBase == null && _bInited == false)
            {
                string repository_path = mcsf_clr_systemenvironment_config.GetApplicationPath();
                string repository_file = repository_path + configureFileName;
                if (File.Exists(repository_file) == false)
                {
                    repository_file = configureFileName;
                }

                DJ2EngineConfigurator configurator = new DJ2EngineConfigurator(repository_file);
                string serverPort = configurator.GetListenPort();
                string serverHost = configurator.GetListenHost();
                string server = (serverPort.Length > 0) ? serverHost + ":" + serverPort : serverHost;

                string dispatchPort = configurator.GetDispatchServerPort();
                string dispatchHost = configurator.GetDispathServerHost();
                string dispatch = (dispatchPort.Length > 0) ? dispatchHost + ":" + dispatchPort : dispatchHost;

                string proxyName = configurator.GetProxyName();

                int iEvtSendChannel = configurator.GetEventSendChannel();

                _2dengineProxyName = configurator.Get2DEngineProxyName();

                if (server.Length <= 0 || proxyName.Length <= 0 || dispatch.Length <= 0 || dj2engineProxyName.Length < 0)
                    return false;

                _containerBase = new DJ2EngineCLRContainerBase();
                _eventListenChannel = configurator.GetEventListenChannel();
                _containerBase.Init(server, proxyName, iEvtSendChannel, dispatch, this);

                _bInited = true;
                return true;
            }

            return _bInited;
        }

        public bool Uninit()
        {
            if (_containerBase != null)
            {
                _containerBase.Fini();
                _containerBase = null;
            }

            return true;
        }

        /// <summary>
        /// The containee will to do what
        /// </summary>
        override public void DoWork()
        {
            SendSystemEvent("", (int)CLRContaineeEventId.SYSTEM_COMMAND_EVENT_ID_COMPONENT_READY,
                GetName());
        }

        
        override public void FinishJob()
        {

        }

        public override bool Shutdown(bool bReboot)
        {
            return base.Shutdown(bReboot);
        }

        override public int GetEstimatedTimeToFinishJob(bool bReboot)
        {
            return 1;
        }
        /// <summary>
        /// send big data test
        /// </summary>
        private void SendBigDataArchitechture()
        {

        }


    }

    
}
