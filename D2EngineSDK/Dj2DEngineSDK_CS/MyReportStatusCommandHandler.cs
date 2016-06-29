using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UIH.Mcsf.Core;
using McsfCommunication;
using Mcsf_Internal;

namespace Web2DEngineSdk
{
   public class MyReportStatusCommandHandler : UIH.Mcsf.Core.ICLRCommandHandler
    {

       public override int HandleCommand(UIH.Mcsf.Core.CommandContext context, UIH.Mcsf.Core.ISyncResult result)
       {
           if (context == null || context.iCommandId != (int)McsfCommunication.MessageCommandType.ReportStatus)
               return -1;


           McsfCommunication.ReportStatusInformation reportStatusInf = McsfCommunication.ReportStatusInformation.ParseFrom(context.sSerializeObject);
           if (reportStatusInf == null)
               return -1;

           if (reportStatusInf.ReportType == McsfCommunication.ReportStatusType.ReportMouseStatus)
           {
              
               IImageStreamHandler handler = Engine2DSdk.Instance().GetImageSteamHandler();
               if (handler != null)
               {
                   if (reportStatusInf.HasImagePosId && reportStatusInf.HasMouseCursor)
                   {
                       ImagePosId imagePosId = reportStatusInf.ImagePosId;
                       ReportMouseCursorArgs mouseCursor = reportStatusInf.MouseCursor;

                       if (imagePosId.HasSiteId && imagePosId.HasPageId && imagePosId.HasSeriesPagePos &&
                           imagePosId.HasSeriesId && imagePosId.HasImageCellPos && mouseCursor.HasMouseType)
                       {
                           handler.OnImageChangeMouseCursor(imagePosId.SiteId, imagePosId.PageId, imagePosId.SeriesId,
                               imagePosId.SeriesPagePos, imagePosId.ImageCellPos, mouseCursor.MouseType);
                       }
                   }

               }
           }
           else if (reportStatusInf.ReportType == McsfCommunication.ReportStatusType.ReportSeriesStatus)
           {
                // to find the series and 
               if (reportStatusInf.HasImagePosId && reportStatusInf.HasSeriesStatus)
               {
                   ImagePosId imagePosId = reportStatusInf.ImagePosId;
                    Engine2DSdk.Instance().UpdateSeriesStatus(
                       imagePosId.SiteId, imagePosId.PageId, imagePosId.SeriesPagePos,
                       reportStatusInf.SeriesStatus); 
               }
               
           }
           else if (reportStatusInf.ReportType == McsfCommunication.ReportStatusType.ReportImageNoteStatus)
           {
               IImageStreamHandler handler = Engine2DSdk.Instance().GetImageSteamHandler();
               if (handler == null)
                   return 0;

               if (reportStatusInf.HasImagePosId && reportStatusInf.HasNoteStatus)
               {
                   ImagePosId imagePosId = reportStatusInf.ImagePosId;
                   McsfCommunication.ReportNoteStatusArgs noteStatus = reportStatusInf.NoteStatus;

                   NoteStatusInformation noteStatusInf = new NoteStatusInformation();
                   bool b = noteStatusInf.SetValues(noteStatus);
                   if (b == false)
                       return 0;
                   handler.OnImageNoteStatusChanged(imagePosId.SiteId, imagePosId.PageId, imagePosId.SeriesId,
                               imagePosId.SeriesPagePos, imagePosId.ImageCellPos, noteStatusInf);
               }
           }
          
           return 0;
       }
    }
}
