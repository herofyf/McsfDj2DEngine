/////////////////////////////////////////////////////////////////////////
/// Copyright, (c) Shanghai United Imaging Healthcare Inc., 2012 
/// All rights reserved. 
/// 
/// Author: Li,Yongwei  yongwei.li@united-imaging.com
///
/// File: mcsf_finish_job_event_handler.cs
///
/// Summary: the excution body of finishing job
/// 
/// 
/// Date  2012-5-17 
//////////////////////////////////////////////////////////////////////////

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;

namespace UIH.Mcsf.Core.ContainerBase
{
    public class FinishJobEventHanlder : IEventHandler
    {
        private IContainee _containee;

        public FinishJobEventHanlder(IContainee pContainee)
        {
            _containee = pContainee;
        }

        public override int HandleEvent(string sender, 
            int channelId, 
            int eventId,
            string serialzedObject)
        {
            Thread t = new Thread(new ThreadStart(_containee.FinishJob));
            t.Start();
            return 0;
        }
    }
}
