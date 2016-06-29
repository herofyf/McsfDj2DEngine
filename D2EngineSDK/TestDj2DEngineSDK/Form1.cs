using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Web2DEngineSdk;
using System.IO;
using System.Threading;
using System.Collections;
using System.Xml;
using UIH.Mcsf.Core;

namespace Test2DEngineSdk
{
    public partial class Form1 : Form
    {
        ImageStreamHandler _imageStreamHandler = null;
        public Form1()
        {
            InitializeComponent();

            _imageStreamHandler = new ImageStreamHandler(this);

            richTxtb.ScrollToCaret();
        }


        bool _init = false;
        DJ2EngineCLRContaineeBase _containeeBase = null;
        private void btn_Init(object sender, EventArgs e)
        {
            if (_init == true) return;

            _init = true;
            string s = tbConfigfile.Text;
            _containeeBase = new DJ2EngineCLRContaineeBase();
            _containeeBase.Init(s);

            Engine2DSdk.Instance().Init((CLRContaineeBase)_containeeBase, _containeeBase.dj2engineProxyName,
                _containeeBase.EventListenChannel);

            Engine2DSdk.Instance().RegisterImageStreamHandler(_imageStreamHandler);
        }

        string _siteId = "site1";
        IStudySeries _studySeries = null;
        public bool isTestThreads = false;

        private void btnLoadSeries_Click(object sender, EventArgs e)
        {
            btn_Init(this, null);

            isTestThreads = false;
            _studySeries = Engine2DSdk.Instance().LoadSeries(_siteId, "page1", txtBoxSeries.Text, int.Parse(tbPagePos.Text));
            if (_studySeries == null)
                return;
            IImageInstance image = _studySeries.GetImage(1);
        }

        private void openImage_Click(object sender, EventArgs e)
        {
            if (txtOffset.Text.Length > 0 && txtParam2.Text.Length > 0)
            {
                int width = int.Parse(txtParam2.Text);
                int height = width;
                pictureBox2.Width = pictureBox1.Width = width;
                pictureBox2.Height = pictureBox1.Height = height;
                _studySeries.OpenImage(int.Parse(txtOffset.Text), (chkboxAbsOffset.CheckState == CheckState.Checked), int.Parse(txtCells.Text), width, height);
            }
        }

        private void btnCloseSeries_Click(object sender, EventArgs e)
        {
            //StudySeries.closeSeries();
            _studySeries.CloseSeries();
        }

        public Image byteArrayToImage(byte[] byteArrayIn)
        {
            MemoryStream ms = new MemoryStream(byteArrayIn);
            Image returnImage = Image.FromStream(ms);
            return returnImage;
        }

        public void ChangeCellImageMouseCursor(int cellNum, McsfCommunication.MouseCursorType cursorType)
        {
            if (cursorType == McsfCommunication.MouseCursorType.MouseCursorDefault)
            {
                pictureBox1.Cursor = System.Windows.Forms.Cursors.Default;
            }
            else if (cursorType == McsfCommunication.MouseCursorType.MouseCursorWait)
            {
                pictureBox1.Cursor = System.Windows.Forms.Cursors.WaitCursor;
            }
        }

        public void ShowChangedNoteStatus(string siteId, string pageId, string seriesId, int pagePosition, int cellPos, NoteStatusInformation noteStatus)
        {
            NoteProp np = new NoteProp(noteStatus);
            np.ShowDialog();

            if (_studySeries == null) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.SetActiveNoteProp(noteStatus.NoteInf);
            }
        }

        public void DrawImage(Image obj, int cellNum)
        {
            if (cellNum == 0)
            {
                pictureBox1.Image = obj;
                pictureBox1.Invalidate();
            }
            else if (cellNum == 1)
            {
                pictureBox2.Image = obj;
                pictureBox2.Invalidate();
            }
            else if (cellNum == 2)
            {
                pictureBox3.Image = obj;
                pictureBox3.Invalidate();
            }
            else if (cellNum == 3)
            {
                pictureBox4.Image = obj;
                pictureBox4.Invalidate();
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (_studySeries == null) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.SetWinCenterWidth(int.Parse(txtParam1.Text), int.Parse(txtWinWidth.Text));
            }
        }

        private void btnLine_Click(object sender, EventArgs e)
        {
            if (_studySeries == null) return;

            if (txtBoxImageCell.Text.Length <= 0) return;
            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.UseLineTool();
            }
        }


        private void btnScale_Click(object sender, EventArgs e)
        {
            if (_studySeries == null) return;

            if (tbScalex.Text.Length <= 0 || tbscaley.Text.Length < 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.OnScale(float.Parse(tbScalex.Text), float.Parse(tbscaley.Text));
            }
        }

        private void Translate_Click(object sender, EventArgs e)
        {
            if (_studySeries == null) return;

            if (tbtranslatex.Text.Length <= 0 || tbtranslatey.Text.Length < 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.OnTranslate(int.Parse(tbtranslatex.Text), int.Parse(tbtranslatey.Text));
            }
        }

        private void Rotate_Click(object sender, EventArgs e)
        {
            if (_studySeries == null) return;

            if (tbrotate.Text.Length <= 0 ) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.OnRotate(int.Parse(tbrotate.Text));
            }
        }

        private void pictureBox1_MouseClick(object sender, MouseEventArgs e)
        {
            if (_studySeries == null) return;

            if (txtBoxImageCell.Text.Length <= 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
              //  image.OnMouse(e.X, e.Y, (e.Button == MouseButtons.Left), (e.Button == MouseButtons.Right), MouseBehaviorType.MouseClick);
            }

            this.label3.Text = string.Format("MouseClick X:{0:d}, y:{1:d}, left = {2}", e.X, e.Y, (e.Button == MouseButtons.Left));
            richTxtb.AppendText(this.label3.Text + System.Environment.NewLine);
        }

        private void btnDelete_Click(object sender, EventArgs e)
        {
            if (_studySeries == null) return;

            if (tbrotate.Text.Length <= 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.OnKeyboard(string.Empty, false, false, false, McsfCommunication.KeyboardFeatureKeyType.FK_DEL);
            }
        }

        List<Test2DEngineTask> _tasks = new List<Test2DEngineTask>();
        private void button2_Click(object sender, EventArgs e)
        {
            btn_Init(this, null);
            _tasks.Clear();

            isTestThreads = true;
            // to create threads 
            for (int i = 0; i < int.Parse(txtbThreads.Text); i++)
            {
                string siteId = String.Format("site{0:d}", i);
                Test2DEngineTask task = new Test2DEngineTask(siteId, txtBoxSeries.Text);
                _tasks.Add(task);
                Thread oThread = new Thread(new ThreadStart(task.Beta));
                oThread.Start();
            }

        }

        public delegate void refreshResult();
        public void showTasksResultOnRichText()
        {
            richTxtb.Clear();
           
            foreach (Test2DEngineTask ob in _tasks)
            {
                string str = String.Format("({0}) milisec: {1:d}, tries:{2:d}", ob._siteId, 
                    ob._elapseMills, ob._triesNum);
                str += System.Environment.NewLine;
                richTxtb.AppendText(str);
            }
        }

        public void StopTaskTick(string siteId, string seriesId)
        {
            lock (this)
            {
                foreach (Test2DEngineTask ob in _tasks)
                {
                    if (ob.Equals(siteId, seriesId))
                    {
                        ob.OnGotImageResult();
                        this.BeginInvoke(new refreshResult(showTasksResultOnRichText));
                        break;
                    }
                }
            }
        }

        private void btnAngle_Click(object sender, EventArgs e)
        {
            if (_studySeries == null) return;

            if (tbrotate.Text.Length <= 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.UseAngleTool();
            }
        }

        private void button3_Click(object sender, EventArgs e)
        {
            if (_studySeries == null) return;

            if (tbrotate.Text.Length <= 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.UseMagnifyGlass();
            }
        }

        private void button4_Click(object sender, EventArgs e)
        {
            pictureBox2.Visible = !(pictureBox2.Visible);
            pictureBox3.Visible = !(pictureBox3.Visible);
            pictureBox4.Visible = !(pictureBox4.Visible);
        }

        private void pictureBox1_MouseDown(object sender, MouseEventArgs e)
        {
            if (_studySeries == null) return;

            if (txtBoxImageCell.Text.Length <= 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.OnMouse(e.X, e.Y, (e.Button == MouseButtons.Left), (e.Button == MouseButtons.Right), MouseBehaviorType.MouseDown);
            }

            this.label3.Text = string.Format("MouseDown X:{0:d}, y:{1:d}, left = {2}", e.X, e.Y, (e.Button == MouseButtons.Left));
            richTxtb.AppendText(this.label3.Text + System.Environment.NewLine);
        }

        private void pictureBox1_MouseUp(object sender, MouseEventArgs e)
        {
            if (_studySeries == null) return;

            if (txtBoxImageCell.Text.Length <= 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
               // image.OnMouse(e.X, e.Y, (e.Button == MouseButtons.Left), (e.Button == MouseButtons.Right), false);
            }

            this.label3.Text = string.Format("MouseUp X:{0:d}, y:{1:d}, left = {2}", e.X, e.Y, (e.Button == MouseButtons.Left));
            richTxtb.AppendText(this.label3.Text + System.Environment.NewLine);
        }
        private void pictureBox1_MouseMove(object sender, MouseEventArgs e)
        {
            if (_studySeries == null) return;

           // if (e.Button == MouseButtons.None) return;

            if (txtBoxImageCell.Text.Length <= 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.OnMouse(e.X, e.Y, (e.Button == MouseButtons.Left), (e.Button == MouseButtons.Right), MouseBehaviorType.MouseMove);
            }

            this.label3.Text = string.Format("MouseMove X:{0:d}, y:{1:d}, left = {2}", e.X, e.Y, (e.Button == MouseButtons.Left));
            richTxtb.AppendText(this.label3.Text + System.Environment.NewLine);

        }
        private void btnCircle_Click(object sender, EventArgs e)
        {
            if (_studySeries == null) return;

            if (tbrotate.Text.Length <= 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.UseCircleTool();
            }
        }

        private void Form1_KeyDown(object sender, KeyEventArgs e)
        {
            if (_studySeries == null) return;

            if (txtBoxImageCell.Text.Length <= 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.OnKeyboard(e.KeyCode.ToString(), e.Control, e.Shift, e.Alt, McsfCommunication.KeyboardFeatureKeyType.FK_None);
            }
        }

        private void Form1_KeyUp(object sender, KeyEventArgs e)
        {
            Form1_KeyDown(sender, e);
        }

        private void btnFlipX_Click(object sender, EventArgs e)
        {
             if (_studySeries == null) return;

            if (txtBoxImageCell.Text.Length <= 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.FlipX();
            }
        }

        private void btnFlipY_Click(object sender, EventArgs e)
        {
            if (_studySeries == null) return;

            if (txtBoxImageCell.Text.Length <= 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.FlipY();
            }
        }

        private void btnColorInvert_Click(object sender, EventArgs e)
        {
             if (_studySeries == null) return;

            if (txtBoxImageCell.Text.Length <= 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.InvertColor();
            }
        }

        private void btnFreeHand_Click(object sender, EventArgs e)
        {
            if (_studySeries == null) return;

            if (txtBoxImageCell.Text.Length <= 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.UseFreeHand();
            }
        }

        bool enableTransSync = true;
        private void btnTransSync_Click(object sender, EventArgs e)
        {
            if (_studySeries == null) return;

            if (enableTransSync)
                _studySeries.EnableTransSync();
            else
                _studySeries.DisableTransSync();

            enableTransSync = !enableTransSync;
        }

        private void btnImageState_Click(object sender, EventArgs e)
        {
           
        }

        private void btnComments_Click(object sender, EventArgs e)
        {
            XmlDocument dom = new XmlDocument();
            dom.Load(@"D:\Work\MCSF\UIH\appdata\review\config\McsfMedViewer3dConfig\MedViewerImageText\mcsf_med_viewer_3d_image_text_ct_cpr.xml");
            byte[] bytes = Encoding.UTF8.GetBytes(dom.OuterXml);

            ISiteConfiguration siteConfig = Engine2DSdk.Instance().GetSiteConfigObj(_siteId);
            ISiteCommentConfiguration siteComm = siteConfig.GetCommentConfigObj();
            siteComm.ChangeImageComments(bytes);
        }

        private void btnRef_Click(object sender, EventArgs e)
        {
            if (_studySeries == null) return;

            _studySeries.SetLocalizerLinesResId(0, true, LocalizerLineOperation.Add);

        }
      
        private void btnRefed_Click(object sender, EventArgs e)
        {
            if (_studySeries == null) return;

            _studySeries.SetLocalizerLinesResId(0, false, LocalizerLineOperation.Add);
        }

        private void btnLLClear_Click(object sender, EventArgs e)
        {
            if (_studySeries == null) return;

            _studySeries.SetLocalizerLinesResId(0, false, LocalizerLineOperation.Clear);
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (_containeeBase != null)
            {
                _containeeBase.Uninit();
                _containeeBase = null;
            }
        }

        private void btnHandMode_Click(object sender, EventArgs e)
        {
            if (_studySeries == null) return;

            if (txtBoxImageCell.Text.Length <= 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.OnHandMode();
            }
        }

        private void btn_ClosePage_Click(object sender, EventArgs e)
        {
            Engine2DSdk.Instance().onWebPageClosing(_siteId, "page1");
        }

        private void btnResetImage_Click(object sender, EventArgs e)
        {
            if (txtBoxImageCell.Text.Length <= 0 || _studySeries == null)
                return;

            int cellIndex = int.Parse(txtBoxImageCell.Text);

            if (cellIndex < 0)
                _studySeries.ResetImages();
            else
            {
                IImageInstance image = _studySeries.GetImage(cellIndex);
                if (image != null)
                    image.ResetImage();
            }
        }

        private void btnArrowNote_Click(object sender, EventArgs e)
        {
            if (_studySeries == null) return;

            if (txtBoxImageCell.Text.Length <= 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.UseNoteTool(NoteToolType.ArrowNote);;
            }
        }

        private void button8_Click(object sender, EventArgs e)
        {
            richTxtb.Clear();
        }

        private void richTxtb_TextChanged(object sender, EventArgs e)
        {
            if (isTestThreads == false)
            {
                richTxtb.SelectionStart = richTxtb.Text.Length;
                // scroll it automatically
                richTxtb.ScrollToCaret();

            }
            
        }

        private void pictureBox1_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            if (_studySeries == null) return;

            // if (e.Button == MouseButtons.None) return;

            if (txtBoxImageCell.Text.Length <= 0) return;

            IImageInstance image = _studySeries.GetImage(int.Parse(txtBoxImageCell.Text));
            if (image != null)
            {
                image.OnMouse(e.X, e.Y, (e.Button == MouseButtons.Left), (e.Button == MouseButtons.Right), MouseBehaviorType.MouseDoubleClick);
            }

            this.label3.Text = string.Format("MouseDoubleClick X:{0:d}, y:{1:d}, left = {2}", e.X, e.Y, (e.Button == MouseButtons.Left));
            richTxtb.AppendText(this.label3.Text + System.Environment.NewLine);
        }

      

    }
}
