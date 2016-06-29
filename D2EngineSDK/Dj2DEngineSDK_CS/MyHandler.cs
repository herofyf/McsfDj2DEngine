using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.IO;
using UIH.Mcsf.Core;


namespace Web2DEngineSdk
{
    public class MyDataHandler : IDataHandler
    {
        public MyDataHandler()
        {
            
        }
        public override int HandleDataTrans(byte[] buffer, int len)
        {
            int index = 0;
            string[] parts = new string[6];
            int partId = 0;

            if (buffer[0] == '?')
            {
                while (true)
                {
                    if (buffer[index ++] == '?')
                    {
                        partId++;
                    }

                    if (partId == 6)
                        break;

                    if (buffer[index] != '?')
                        parts[partId] += Convert.ToChar(buffer[index]);

                }
            }

            if (partId == 6)
            {
                string siteId = parts[1];
                string pageId = parts[2];
                int pagePos = int.Parse(parts[3]);
                string seriesId = parts[4];
                int cellPos = int.Parse(parts[5]);

                byte[] imageBuffer = new byte[len - index];
                System.Buffer.BlockCopy(buffer, index, imageBuffer, 0, len - index);

                IImageStreamHandler handler = Engine2DSdk.Instance().GetImageSteamHandler();
                if (handler != null)
                {
                    handler.OnImageString(siteId, pageId, seriesId, pagePos, cellPos, imageBuffer, len - index);
                }
            }
            
            return 0;
        }
    }

    public class MyCommandHandler : ICLRCommandHandler
    {
        override public int HandleCommand(CommandContext pContext, ISyncResult pSyncResult)
        {
            return 0;
        }
    }

     class AsynCmdCallBackArchitecture : ICommandCallbackHandler
    {
        /// <summary>
        /// handle the result of BE processed
        /// </summary>
        /// <param name="pAsyncResult">store the result of BE processed</param> 
        /// <returns>0 if success</returns>
        public override int HandleReply(UIH.Mcsf.Core.IAsyncResult pAsyncResult)
        {
            // here you can deal with the result of BE returned
            // pAsyncResult.GetStringObject() can be a string or a class ;
            // if a class, parsing it by protobuf you defined
           
            //Console.WriteLine("Call back function has been called ,pAsyncResult = {0}", pAsyncResult.GetStringObject());
            return 0;
        }
    };
   
}
