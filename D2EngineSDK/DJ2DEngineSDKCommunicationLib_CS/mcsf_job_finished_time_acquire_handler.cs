using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UIH.Mcsf.Log;

namespace UIH.Mcsf.Core.ContainerBase
{
    public class EstimatedTimeAcquireCmdHanlder : ICLRCommandHandler
    {
        private IContainee _containee;

        public EstimatedTimeAcquireCmdHanlder(IContainee pContainee)
        {
            _containee = pContainee;
        }

        public override int HandleCommand(CommandContext context, ISyncResult result)
        {
            try
            {
                QueryShutdownResponse.Builder rsBl = new QueryShutdownResponse.Builder();
                rsBl.SetILeftTime(_containee.GetEstimatedTimeToFinishJob(
                    Convert.ToBoolean(Convert.ToInt16(context.sStringObject))));
                result.SetSerializedObject(rsBl.Build().ToByteArray());
                return 0;
            }
            catch (System.Exception ex)
            {
                CLRLogger.GetInstance().LogDevError(ClrCtnLog.logsrc, ex.Message);
                return 0;
            }
        }
    }
}
