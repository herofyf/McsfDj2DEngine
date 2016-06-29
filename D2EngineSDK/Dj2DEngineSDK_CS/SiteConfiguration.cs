using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UIH.Mcsf.Core;
using McsfCommunication;
using Google.ProtocolBuffers;

namespace Web2DEngineSdk
{
    public interface ISiteConfiguration
    {
        ISiteCommentConfiguration GetCommentConfigObj();
    }

    public class SiteConfiguration : ISiteConfiguration
    {
        string _siteId;
        ISiteCommentConfiguration _siteCommentConfigObj;

        internal SiteConfiguration(string siteId)
        {
            _siteId = siteId;
            _siteCommentConfigObj = new SiteCommentConfiguration(this);
        }
        public ISiteCommentConfiguration GetCommentConfigObj()
        {
            return _siteCommentConfigObj;
        }

        internal bool UpdateSiteCommentTags(byte[] bytes)
        {
            CLRContaineeBase _containee = Engine2DSdk.Instance().GetContaineeBase();
            CommandContext context = new CommandContext();
            context.sReceiver = Engine2DSdk.Instance().Get2DEngineProxyName();
            context.iCommandId = (int)MessageCommandType.SiteSettings;

            ImageRequest.Builder builder = new ImageRequest.Builder();

            ImagePosId.Builder imageBuilder = builder.ImagePosId.CreateBuilderForType();
            imageBuilder.SiteId = _siteId;
            imageBuilder.PageId = "";
            imageBuilder.SeriesId = "";

            imageBuilder.SeriesPagePos = 0;

            imageBuilder.ImageCellPos = 0;

            builder.ImagePosId = imageBuilder.Build();

            SiteSettingInformation.Builder siteSettingInfBuilder = builder.SiteSettingInformation.CreateBuilderForType();
            if (siteSettingInfBuilder == null) return false;

            siteSettingInfBuilder.SiteSettingType = McsfCommunication.SiteSettingType.SiteCommentSettingsType;

            SiteSettingContent.Builder siteSettingContentBuilder = siteSettingInfBuilder.SiteSettingContent.CreateBuilderForType();
            if (siteSettingInfBuilder == null) return false;

            ByteString bs = ByteString.CopyFrom(bytes);
            siteSettingContentBuilder.SetMessageIndex(-1);
            siteSettingContentBuilder.SetMessageSize(bytes.Length);
            siteSettingContentBuilder.SetMessageContent(bs);

            siteSettingInfBuilder.SiteSettingContent = siteSettingContentBuilder.Build();


            builder.SiteSettingInformation = siteSettingInfBuilder.Build();

            context.sSerializeObject = builder.Build().ToByteArray();

            context.pCommandCallback = new AsynCmdCallBackArchitecture();
            int res = _containee.AsyncSendCommand(context);
            if (0 != res)
            {
                return false;
            }
 
            return true;
        }

    }
}
