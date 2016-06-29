using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Web2DEngineSdk;

namespace Test2DEngineSdk
{
    public partial class NoteProp : Form
    {
        NoteObjectInformation _noteObj;

        public NoteProp(NoteStatusInformation noteInf)
        {
            InitializeComponent();
            _noteObj = noteInf.NoteInf;

            colorDialog1.Color = _noteObj.NoteArrowLineInf.LineColor;
            this.textBox1.Text = _noteObj.NoteTextInf.Text;
        }

        private void btnSetProp_Click(object sender, EventArgs e)
        {
            _noteObj.NoteTextInf.Text = textBox1.Text;
            this.Close();
        }

        private void btnFontName_Click(object sender, EventArgs e)
        {
            fontDialog1.ShowDialog();
            _noteObj.NoteTextInf.FontName = fontDialog1.Font.Name;
            _noteObj.NoteTextInf.FontSize = (int)fontDialog1.Font.Size;
            switch (fontDialog1.Font.Style)
            {
                case FontStyle.Regular:
                    _noteObj.NoteTextInf.FontStyle = TextFontStyle.FontStyleRegular;
                    break;
                case FontStyle.Bold:
                    _noteObj.NoteTextInf.FontStyle = TextFontStyle.FontStyleBold;
                    break;

            }
        }

        private void btnFontColor_Click(object sender, EventArgs e)
        {
            colorDialog1.ShowDialog();

            _noteObj.NoteArrowLineInf.LineColor = colorDialog1.Color;
            _noteObj.NoteTextInf.FontColor = colorDialog1.Color;
        }

        private void NoteProp_Load(object sender, EventArgs e)
        {

        }
    }
}
