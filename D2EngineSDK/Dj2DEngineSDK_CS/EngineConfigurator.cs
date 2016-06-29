//////////////////////////////////////////////////////////////////////////
/// \defgroup EXAMPLES Examples for C++-like Style
///  Copyright, (c) Shanghai United Imaging healthcare Inc., 2011
///  All rights reserved.
///
///  \author  Miao JiaHong  JiaHong.miao@united-imaging.com
///
///  \file    CLRConfigurator.cs
///  \brief   implement of CLRConfigurator
///
///  \version 1.0
///  \date    Sep. 01, 2011
///  \{
//////////////////////////////////////////////////////////////////////////
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;
using UIH.Mcsf.Log;
using UIH.Mcsf.Core.ContainerBase;

namespace UIH.Mcsf.Core
{

    static class Constants
    {
        public const string TAGNAME_SEND_CHANNEL_ID = "Container/SendEventChannel";
        public const string TAGNAME_LISTEN_PATH_PORT = "Container/ListenAddress/Port";
        public const string TAGNAME_LISTEN_PATH_HOST = "Container/ListenAddress/Host";
        public const string TAGNAME_REMOTE_PATH_PORT = "Container/SysDispatcherAddress/Port";
        public const string TAGNAME_REMOTE_PATH_HOST = "Container/SysDispatcherAddress/Host";
   
        public const string TAGNAME_EVENT_LISTEN_CHANNEL_PATH = "Container/ListenEventChannel/Channel";
        public const string TAGNAME_COMMUNICATION_PROXY_NAME = "Container/CommunicationProxyName";
        public const string TAGNAME_2DENGINE_PROXY_NAME = "Container/Engine2DProxyName";
   
        public const string TAGNAME_LOG_CONFIG_FILE = "Container/LogConfigFile";
        public const string TAGNAME_CONTAINEE_CONFIG_FILE = "CustomizedConfigFile";

    }

    public class DJ2EngineConfigurator
    {

        XmlDocument m_XmlConfig;//=new XmlDocument();
        public DJ2EngineConfigurator(string sFileName)
        {
            m_XmlConfig = new XmlDocument();
            try
            {
                m_XmlConfig.Load(sFileName);
            }
            catch(XmlException e)
            {
              
               // CLRLogger.GetInstance().LogDevError(ClrCtnLog.logsrc, e.Message);
            }
            catch(Exception e)
            {
               
               // CLRLogger.GetInstance().LogDevError(ClrCtnLog.logsrc, e.Message);
            }
        }

    
        ///
        ///  <key> \n
        ///  PRA:No \n
        ///  Traced from: \n
        ///  Description: get event send channel.\n
        ///  Short Description:GetEventSendChannel  \n
        ///  Component:CLRConfigurator \n
        ///  </key> \n
        public int GetEventSendChannel( )
        {
            int iEvtChannelId = 0;
            XmlNode evtChannelId = m_XmlConfig.SelectSingleNode(Constants.TAGNAME_SEND_CHANNEL_ID);
            if (null != evtChannelId)
                iEvtChannelId = Convert.ToInt32(evtChannelId.InnerText);
            return iEvtChannelId;
        }

        public int GetEventListenChannel()
        {
            int iEvtChannelId = 0;
            XmlNode evtChannelId = m_XmlConfig.SelectSingleNode(Constants.TAGNAME_EVENT_LISTEN_CHANNEL_PATH);
            if (null != evtChannelId)
                iEvtChannelId = Convert.ToInt32(evtChannelId.InnerText);
            return iEvtChannelId;
        }

        /// 
        ///  <key> \n
        ///  PRA:No \n
        ///  Traced from: \n
        ///  Description: get listen address.\n
        ///  Short Description:GetListenAddress  \n
        ///  Component:CLRConfigurator \n
        ///  </key> \n
        public string  GetListenPort( )
        {
            XmlNode listenPort = m_XmlConfig.SelectSingleNode(Constants.TAGNAME_LISTEN_PATH_PORT);
            if (null != listenPort)
                return listenPort.InnerText;

            return null;
        }


        public string GetListenHost()
        {
            XmlNode listenPort = m_XmlConfig.SelectSingleNode(Constants.TAGNAME_LISTEN_PATH_HOST);
            if (null != listenPort)
                return listenPort.InnerText;

            return null;
        }

        ///
        ///  <key> \n
        ///  PRA:No \n
        ///  Traced from: \n
        ///  Description:get log config file. \n
        ///  Short Description:GetLogConfigFile  \n
        ///  Component:CLRConfigurator \n
        ///  </key> \n
        public string GetLogConfigFile()
        {
            XmlNode logFile = m_XmlConfig.SelectSingleNode(Constants.TAGNAME_LOG_CONFIG_FILE);
            if (null != logFile)
            {
                return logFile.InnerText;
            }
            return null;
        }

        public string GetProxyName()
        {
            XmlNode logFile = m_XmlConfig.SelectSingleNode(Constants.TAGNAME_COMMUNICATION_PROXY_NAME);
            if (null != logFile)
            {
                return logFile.InnerText;
            }
            return null;
        }

        public string GetDispathServerHost()
        {
            XmlNode logFile = m_XmlConfig.SelectSingleNode(Constants.TAGNAME_REMOTE_PATH_HOST);
            if (null != logFile)
            {
                return logFile.InnerText;
            }
            return null;
        }

        public string GetDispatchServerPort()
        {
            XmlNode logFile = m_XmlConfig.SelectSingleNode(Constants.TAGNAME_REMOTE_PATH_PORT);
            if (null != logFile)
            {
                return logFile.InnerText;
            }
            return null;
        }

        public string Get2DEngineProxyName()
        {
            XmlNode logFile = m_XmlConfig.SelectSingleNode(Constants.TAGNAME_2DENGINE_PROXY_NAME);
            if (null != logFile)
            {
                return logFile.InnerText;
            }
            return null;
        }
    }

}


