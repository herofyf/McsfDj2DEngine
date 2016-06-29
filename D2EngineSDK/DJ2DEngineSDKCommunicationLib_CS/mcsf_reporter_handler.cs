using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UIH.Mcsf.Log;
using UIH.Mcsf.Core.ContainerBase;

namespace UIH.Mcsf.Core
{
    public class ProcessInfoReporterCmdHandler : ICLRCommandHandler
    {
        public override int HandleCommand(CommandContext context, ISyncResult result)
        {
            try
            {
                if(context.sReceiver == context.sSender)
                {
                    result.SetSerializedObject(SysInfoCollector._Instance.FillBuilder().Build().ToByteArray());
                }
                else
                {
                    result.SetSerializedString("");
                }
                return 0;
            }
            catch (System.Exception ex)
            {
                CLRLogger.GetInstance().LogDevError(ClrCtnLog.logsrc, "#########");
                CLRLogger.GetInstance().LogDevError(ClrCtnLog.logsrc, ex.Message);
                CLRLogger.GetInstance().LogDevError(ClrCtnLog.logsrc, "#########");
                return -1;
            }
        }
    }
}
