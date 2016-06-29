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
using System.Reflection;
using System.Threading;
using Mcsf_Internal;
using System.Runtime.InteropServices;
using UIH.Mcsf.Log;
using UIH.Mcsf.Core.ContainerBase;
using System.IO;
using System.Net.NetworkInformation;
using System.Diagnostics;

namespace UIH.Mcsf.Core
{
    public static class TypeEx
    {
        public static bool IsTypeof(this Type type, string name)
        {
            if (type.FullName == "System.Object" || type.BaseType == null)
                return false;

            if (type.FullName == name)
            {
                return true;
            }
            else
            {
                return IsTypeof(type.BaseType, name);
            }
        }
    }

    /// <summary>
    /// Load Containee and let it to work
    /// </summary>
    public class DJ2EngineCLRContainerBase : IContainer
    {
        private const int kINNER_CMD_DEFAULT_TIMEOUT = 1000;

        public ICommunicationProxy m_pComProxy = null;
        public string m_sProxyName = string.Empty;
        public string m_sListenIp = string.Empty;
        public int m_iEventSendChannel = 0;
        public string m_sCustFile = string.Empty;
        public string m_sDisName = string.Empty;
        public string m_sDispatchIp = string.Empty;
        public IContainee m_pContainee = null;
        private Thread _threadRunContainee = null;
        public static Mutex mutex = null;
        public static CondSync _cond = new CondSync();

    
        /// <summary>
        /// initialize logger and CommunicationProxy , and startup a containee
        /// </summary>
        /// <param name="sConfigFile">configuration file name</param>
        /// <returns>0 if success, else -1</returns>
        ///  <key> \n
        ///  PRA:No \n
        ///  Traced from: \n
        ///  Description: initialize logger and CommunicationProxy , and startup a containee.\n
        ///  Short Description:Init  \n
        ///  Component:CLRContainerBase \n
        ///  </key> \n
        public int Init(string strListenIp, string strProxyName, int iEventSendChannel, string strDispatchIp, IContainee iContaineeBase)
        {
            try
            {
                m_sProxyName = strProxyName;

                m_sListenIp = strListenIp;
                
                m_iEventSendChannel = iEventSendChannel;

                m_sDispatchIp = strDispatchIp;
                
                m_sDisName = strProxyName;
                
                m_pComProxy = CreateCommunitionProxy(m_sProxyName, m_sListenIp, m_iEventSendChannel);
                    
                if (null == m_pComProxy)
                {
                    CLRLogger.GetInstance().LogDevError(ClrCtnLog.logsrc, "Create Communication Proxy failed!");
                    return -1;
                }

                MemoryManagement._pCommProxy = m_pComProxy;

                m_pContainee = iContaineeBase;
                if (null == m_pContainee)
                {
                    CLRLogger.GetInstance().LogDevError(ClrCtnLog.logsrc, "Create Containee failed!");
                    return -1;
                }

                m_pContainee.SetCommunicationProxy(m_pComProxy);
                //m_pContainee.SetCustomConfigFile();
                                    
                m_pContainee.Startup();

                RegisterHandlers(m_pComProxy, m_pContainee);

                _threadRunContainee = new Thread(new ThreadStart(ContaineeDoWork));
                _threadRunContainee.Start();

                CLRLogger.GetInstance().LogDevInfo(ClrCtnLog.logsrc,"CLRContainerBase::Init leave");
                return 0;
            }
            catch (System.Exception ex)
            {
                Console.WriteLine("Found a exception {0}", ex.Message);
                CLRLogger.GetInstance().LogDevError(ClrCtnLog.logsrc, ex.Message);
                return -1;
            }
        }

      
        /// <summary>
        /// Create Communication proxy instance and initialization.
        /// </summary>
        /// <param name="name"></param>
        /// <param name="addr"></param>
        /// <param name="ichannel"></param>
        /// <returns></returns>
        private CLRCommunicationProxy CreateCommunitionProxy(string name, string addr, int ichannel)
        {
            try
            {
                CLRCommunicationProxy proxy = new CLRCommunicationProxy();
                proxy.SetName(name);
                proxy.SetListenAddress(addr);
                proxy.SetSenderChannelId(ichannel);
                if (0 != proxy.CheckCastToSystemDispatcher(GetDiptAddr()))
                {
                    CLRLogger.GetInstance().LogDevError(ClrCtnLog.logsrc, "Can't connect dispatcher!" + GetDiptAddr());
                    return null;
                }

                if (0 != proxy.StartListener(addr))
                {
                    CLRLogger.GetInstance().LogDevError(ClrCtnLog.logsrc, "Start listen failed!");
                    return null;
                }

                return proxy;
            }
            catch(System.Exception ex)
            {
                CLRLogger.GetInstance().LogDevError(ClrCtnLog.logsrc, ex.Message);
                return null;
            }
        }

        void ContaineeDoWork()
        {
            try
            {
                m_pContainee.DoWork();
            }
            catch (System.Exception ex)
            {
                CLRLogger.GetInstance().LogDevError(ClrCtnLog.logsrc, ex.Message);
                _cond.notify_one();
            }
        }

        ///  <key> \n
        ///  PRA:No \n
        ///  Traced from: \n
        ///  Description: Do nothing but sleep.\n
        ///  Short Description:DoWork  \n
        ///  Component:CLRContainerBase \n
        ///  </key> \n
        public void DoWork()
        {
            _cond.wait();
        }

        /// <summary>
        /// Get the address of dispatcher
        /// </summary>
        /// <returns></returns>
        private string GetDiptAddr()
        {
            return m_sDispatchIp;
        }

        /// <summary>
        /// the interface of Icontainer ,nothing to do here
        /// </summary>
        /// <returns></returns>
        ///  <key> \n
        ///  PRA:No \n
        ///  Traced from: \n
        ///  Description:the interface of Icontainer ,nothing to do here. \n
        ///  Short Description:Fini  \n
        ///  Component:CLRContainerBase \n
        ///  </key> \n
        override public int Fini()
        {
            try
            {
                _cond.notify_one();
                _threadRunContainee.Abort();
                CLRLogger.GetInstance().LogDevInfo(ClrCtnLog.logsrc,"thread of DoWork in containee is over...");
                if (null != m_pComProxy)
                {
                    m_pComProxy.UnRegisterAllHandlers();
                }

                CLRLogger.GetInstance().LogDevInfo(ClrCtnLog.logsrc,"To destroy log...");
                CLRLogger.GetInstance().DestoryLogger();
                //Console.WriteLine("Destory log spends {0} ms.", st.ElapsedMilliseconds);
                return 0;
            }
            catch (SystemException ex)
            {
                //let user could see
                Thread.Sleep(1000);
                Console.WriteLine(ex.Message);
                return -1;
            }
        }

     

        /// <summary>
        /// Register command and event handlers, and pass the communication proxy to other class
        /// </summary>
        /// <param name="proxy"></param>
        /// <param name="ctee"></param>
        private void RegisterHandlers(ICommunicationProxy proxy, IContainee ctee )
        {
            proxy.RegisterCommandHandler((int)SystemCommandId.SYSTEM_COMMAND_ID_SYS_MANAGER_GET_PROCESS,
                new ProcessInfoReporterCmdHandler());
            proxy.RegisterEventHandler(0, (int)CLRContaineeEventId.SYSTEM_COMMAND_EVENT_ID_FINISH_JOB,
                new FinishJobEventHanlder(ctee));
            proxy.RegisterCommandHandler((int)SystemCommandId.SYSTEM_COMMAND_ID_SYS_MANAGER_SHUT_DOWN,
                new ShutDownCommandHandler(this));
            proxy.RegisterCommandHandler((int)CLRContaineeCommandId.SYSTEM_COMMAND_ID_SYS_MANAGER_QUERY_JOB_FINISHED_TIME,
                new EstimatedTimeAcquireCmdHanlder(ctee));

            UIH.Mcsf.Log.McsfLogChangeLevelBE.GetInstance().Initial(proxy);
            SysInfoCollector._Instance.SetProxy(proxy);
            SysInfoCollector._Instance.SetDisplayName(m_sDisName);
        }

    }

    /// <summary>
    /// Shut down command handler
    /// </summary>
    public class ShutDownCommandHandler : ICLRCommandHandler
    {
        DJ2EngineCLRContainerBase m_pContainer;

        public ShutDownCommandHandler(DJ2EngineCLRContainerBase pContainer)
        {
            m_pContainer = pContainer;
        }
        /// <summary>
        /// handle the shut down command
        /// </summary>
        /// <param name="pContext">sending message</param>
        /// <param name="pSyncResult">result wanted to set</param>
        /// <returns></returns>
        override public int HandleCommand(CommandContext pContext, ISyncResult pSyncResult)
        {
            try
            {
                CLRLogger.GetInstance().LogDevInfo(ClrCtnLog.logsrc,"Call containee's shutdown.");
                m_pContainer.m_pContainee.Shutdown(Convert.ToBoolean(Convert.ToInt16(pContext.sStringObject)));
                ReplyMessage.Builder biler = new ReplyMessage.Builder();
                biler.SetSContext(m_pContainer.m_pComProxy.GetName());
                biler.SetSStatus(statustype.SUCCESS);
                pSyncResult.SetSerializedObject(biler.Build().ToByteArray());
                //Release in time
                CLRLogger.GetInstance().LogDevInfo(ClrCtnLog.logsrc,"To release singleton mutex.");
                if (null != DJ2EngineCLRContainerBase.mutex)
                {
                    DJ2EngineCLRContainerBase.mutex.Close();
                    DJ2EngineCLRContainerBase.mutex.Dispose();
                }

                CLRLogger.GetInstance().LogDevInfo(ClrCtnLog.logsrc,"To notify main thread.");
                DJ2EngineCLRContainerBase._cond.notify_one();
                return 0;
            }
            catch (System.Exception ex)
            {
                CLRLogger.GetInstance().LogDevError(ClrCtnLog.logsrc, "ShutDownCommandHandler::HandleCommand=>" + ex.Message);
                return -1;
            }

        }
    }

}
