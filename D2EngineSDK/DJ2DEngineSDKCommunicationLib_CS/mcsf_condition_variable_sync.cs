using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;

namespace UIH.Mcsf.Core.ContainerBase
{
    public class CondSync
    {
        private object _lock = new object();

        public void wait()
        {
            lock (_lock)
            {
                Monitor.Wait(_lock);
            }
        }

        public void notify_one()
        {
            lock (_lock)
            {
                Monitor.Pulse(_lock);
            }
        }

        public void notify_all()
        {
            lock (_lock)
            {
                Monitor.PulseAll(_lock);
            }
        }
    }
}
