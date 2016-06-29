using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using Web2DEngineSdk;

namespace Test2DEngineSdk
{
    public class MyClassSpecialComparer : IEqualityComparer<Test2DEngineTask>
    {
        public bool Equals(Test2DEngineTask x, Test2DEngineTask y)
        {
            return x._seriesId == y._seriesId && x._siteId == y._siteId;
        }

        public int GetHashCode(Test2DEngineTask x)
        {
            return x._siteId.GetHashCode() + x._seriesId.GetHashCode();
        }


    }


    public class Test2DEngineTask
    {
        public string _siteId;
        public string _seriesId;

        Stopwatch _watch = new Stopwatch();
        public long _elapseMills;

        public long _triesNum = 0;
        public Test2DEngineTask(string siteId, string seriesId)
        {
            _siteId = siteId;
            _seriesId = seriesId;
        }

        public void Beta()
        {

            IStudySeries _studySeries = null;

            _studySeries = Engine2DSdk.Instance().LoadSeries(_siteId, "page1", _seriesId, 0);
            if (_studySeries != null)
            {
                _watch.Restart();
                _triesNum = 0;
                _studySeries.OpenImage(0, false, 2, 400, 400);

                IImageInstance image = _studySeries.GetImage(1);
                if (image != null)
                {
                    image.OnTranslate(10, 10);
                    image.OnMouse(10, 20, true, false, MouseBehaviorType.MouseMove);
                    image.OnMouse(200, 300, true, false, MouseBehaviorType.MouseMove);
                    image.OnMouse(60, 30, true, false, MouseBehaviorType.MouseMove);
                    image.OnMouse(100, 100, true, false, MouseBehaviorType.MouseMove);
                    image.OnMouse(200, 300, true, false, MouseBehaviorType.MouseMove);
                    image.OnMouse(40, 40, true, false, MouseBehaviorType.MouseMove);
                    image.OnScale(0.1f, 0.1f);
                    image.OnRotate(45);
                    image.OnRotate(45);
                }

                _studySeries.OpenImage(1, false, 2, 400, 400);
                _studySeries.OpenImage(1, false, 2, 400, 400);
                _studySeries.OpenImage(1, false, 2, 400, 400);
                _studySeries.OpenImage(1, false, 2, 400, 400);

            }

            _studySeries.CloseSeries();
            _watch.Stop();
        }

        public void OnGotImageResult()
        {
            _watch.Stop();
            _triesNum++;
            long curMillSeconds =  _watch.ElapsedMilliseconds;
            if (curMillSeconds > _elapseMills)
            {
                _elapseMills = curMillSeconds;
            }
            
            _watch.Restart();
        }

        public bool Equals(string siteId, string seriesId)
        {
            bool b1 = (this._siteId == siteId);
            bool b2 = (this._seriesId == seriesId);

            return (b1 && b2);
        }
    }
}
