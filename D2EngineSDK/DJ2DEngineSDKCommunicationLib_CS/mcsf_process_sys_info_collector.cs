using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.Threading;
using System.Runtime.InteropServices;
using System.IO;
using UIH.Mcsf.Log;
using FILETIME = System.Runtime.InteropServices.ComTypes.FILETIME;
using UIH.Mcsf.Core.ContainerBase;

namespace UIH.Mcsf.Core
{
    class SysInfoCollector
    {
        public static SysInfoCollector _Instance = new SysInfoCollector();

        private ICommunicationProxy _proxy;
        private string _strDisplay = string.Empty;
        [DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        public static extern bool GetProcessTimes(IntPtr handle, out long creation, out long exit, out long kernel, out long user);

        [DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        public static extern IntPtr GetCurrentProcess();


        [DllImport("kernel32")]
        public static extern void GetSystemTimeAsFileTime(ref FILETIME lpSystemTimeAsFileTime);

        public SysInfoCollector()
        {
        }

        public void SetProxy(ICommunicationProxy pxy)
        {
            _proxy = pxy;
        }

        public void SetDisplayName(string sDisp)
        {
            _strDisplay = sDisp;
        }

        public SingleProcessInfo.Builder FillBuilder()
        {
            try
            {
                SingleProcessInfo.Builder _infoBilder = new SingleProcessInfo.Builder();
                Process _pro = Process.GetCurrentProcess();
                _infoBilder.SetIPID(_pro.Id);
                _infoBilder.SetIPriority(_pro.PriorityClass.GetHashCode());
                _infoBilder.SetIMemUsage((int)_pro.WorkingSet64);
                _infoBilder.SetIArchitecture(64);
                _infoBilder.SetSDisplayName(_strDisplay);

                string sapp = string.Empty;
                string sindex = string.Empty;
                string stype = string.Empty;
                CommunicationNodeName.ParseCommunicationProxyName(_proxy.GetName(), ref sapp, ref sindex, ref stype);
                _infoBilder.SetSApplicationGroup(sapp);

                long tCreateTime;
                long tExitTimet;
                long tKernelTime;
                long tUserTime;
                IntPtr phandle = GetCurrentProcess();
                if (!GetProcessTimes(phandle, out tCreateTime, out tExitTimet, out tKernelTime, out tUserTime))
                {
                    Console.WriteLine("--------GetProcessTimes error.-----------");
                    CLRLogger.GetInstance().LogDevError(ClrCtnLog.logsrc, "--------GetProcessTimes error.-----------");
                }

                _infoBilder.SetISysCPUTime(tKernelTime);
                _infoBilder.SetIUserCPUTime(tUserTime);
                _infoBilder.SetIStartTime(_pro.StartTime.ToFileTime().GetHashCode());
                _infoBilder.SetIThreadCount(_pro.Threads.Count);
                _infoBilder.SetSImageFile(_pro.MainModule.FileName);
                _infoBilder.SetSUser(_pro.StartInfo.UserName);
                _infoBilder.SetILastUpdateTime(GetUpdateTime(_pro));
                _infoBilder.SetICPUUsage(GetCpuUsing());
                _infoBilder.SetSProcessName(_proxy.GetName());

                return _infoBilder;
            }
            catch (System.Exception ex)
            {
                Console.WriteLine("***********{0}************", ex.Message);
                CLRLogger.GetInstance().LogDevError(ClrCtnLog.logsrc, ex.Message);
            }
            return null;
        }

        private long GetUpdateTime(Process pro)
        {
            FileInfo f = new FileInfo(pro.MainModule.FileName);
            return long.Parse(f.LastWriteTime.ToFileTime().ToString());
        }

        long last_time_ = 0;
        long last_system_time_ = 0;
        private int GetCpuUsing()
        {
            FILETIME now = new FILETIME();
            long creation_time;
            long exit_time;
            long kernel_time;
            long user_time;
            long system_time;
            long time;
            long system_time_delta;
            long time_delta;
            int cpu = 0;

            int processor_count_ = Environment.ProcessorCount;
            processor_count_ = processor_count_ == 0 ? -1 : processor_count_;

            GetSystemTimeAsFileTime(ref now);

            IntPtr phandle = GetCurrentProcess();
            if (!GetProcessTimes(phandle, out creation_time, out exit_time, out kernel_time, out user_time))
            {
                return 0;
            }
            system_time = (kernel_time + user_time) / processor_count_;
            time = (((long)now.dwHighDateTime) << 32) + now.dwLowDateTime;

            if ((last_system_time_ == 0) || (last_time_ == 0))
            {
                last_system_time_ = system_time;
                last_time_ = time;
                return 0;
            }

            system_time_delta = system_time - last_system_time_;
            time_delta = time - last_time_;

            if (time_delta == 0)
                return 0;

            cpu = (int)((system_time_delta * 100 + time_delta / 2) / time_delta);
            last_system_time_ = system_time;
            last_time_ = time;

            return cpu;
        }
    }
}
