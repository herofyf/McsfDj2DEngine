using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Web2DEngineSdk
{
    public interface ISiteCommentConfiguration
    {
        void ChangeImageComments(byte[] bytes);
    }

    public class SiteCommentConfiguration : ISiteCommentConfiguration
    {
        SiteConfiguration _parentObj;

        internal SiteCommentConfiguration(SiteConfiguration parentObj)
        {
            _parentObj = parentObj;
        }

        public void ChangeImageComments(byte[] bytes)
        {
            if (_parentObj != null)
            {
                _parentObj.UpdateSiteCommentTags(bytes);
            }
        }
      
    }
}
