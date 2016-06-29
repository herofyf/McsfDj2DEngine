using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Dj2DEngineSdk_CS
{
    class SeriesLocationId
    {
        string _siteId;
        public string SiteId
        {
            get { return _siteId; }
            set { _siteId = value; }
        }

        string _pageId;
        public string PageId
        {
            get { return _pageId; }
            set { _pageId = value; }
        }
        int _pagePosition;
        public int PagePosition
        {
            get { return _pagePosition; }
            set { _pagePosition = value; }
        }

        public override bool Equals(object obj)
        {
            SeriesLocationId seriesLocId = obj as SeriesLocationId;
            if (obj == null)
                return false;

            return (_siteId == seriesLocId._siteId) && (_pageId == seriesLocId._pageId) && (_pagePosition == seriesLocId._pagePosition);
        }

        public override int GetHashCode()
        {
            return _siteId.GetHashCode() + _pageId.GetHashCode() + _pagePosition.GetHashCode();
        }
    }

}
