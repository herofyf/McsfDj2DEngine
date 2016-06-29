namespace Test2DEngineSdk
{
    partial class NoteProp
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.btnSetProp = new System.Windows.Forms.Button();
            this.fontDialog1 = new System.Windows.Forms.FontDialog();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.colorDialog1 = new System.Windows.Forms.ColorDialog();
            this.btnFontName = new System.Windows.Forms.Button();
            this.btnFontColor = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // btnSetProp
            // 
            this.btnSetProp.Location = new System.Drawing.Point(98, 167);
            this.btnSetProp.Name = "btnSetProp";
            this.btnSetProp.Size = new System.Drawing.Size(75, 23);
            this.btnSetProp.TabIndex = 0;
            this.btnSetProp.Text = "SetProp";
            this.btnSetProp.UseVisualStyleBackColor = true;
            this.btnSetProp.Click += new System.EventHandler(this.btnSetProp_Click);
            // 
            // textBox1
            // 
            this.textBox1.Location = new System.Drawing.Point(66, 37);
            this.textBox1.Name = "textBox1";
            this.textBox1.Size = new System.Drawing.Size(144, 21);
            this.textBox1.TabIndex = 1;
            this.textBox1.Text = "2323ddddd";
            // 
            // btnFontName
            // 
            this.btnFontName.Location = new System.Drawing.Point(57, 94);
            this.btnFontName.Name = "btnFontName";
            this.btnFontName.Size = new System.Drawing.Size(65, 25);
            this.btnFontName.TabIndex = 2;
            this.btnFontName.Text = "fontName";
            this.btnFontName.UseVisualStyleBackColor = true;
            this.btnFontName.Click += new System.EventHandler(this.btnFontName_Click);
            // 
            // btnFontColor
            // 
            this.btnFontColor.Location = new System.Drawing.Point(175, 94);
            this.btnFontColor.Name = "btnFontColor";
            this.btnFontColor.Size = new System.Drawing.Size(77, 21);
            this.btnFontColor.TabIndex = 3;
            this.btnFontColor.Text = "FontColor";
            this.btnFontColor.UseVisualStyleBackColor = true;
            this.btnFontColor.Click += new System.EventHandler(this.btnFontColor_Click);
            // 
            // NoteProp
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(555, 353);
            this.Controls.Add(this.btnFontColor);
            this.Controls.Add(this.btnFontName);
            this.Controls.Add(this.textBox1);
            this.Controls.Add(this.btnSetProp);
            this.Name = "NoteProp";
            this.Text = "NoteProp";
            this.Load += new System.EventHandler(this.NoteProp_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnSetProp;
        private System.Windows.Forms.FontDialog fontDialog1;
        private System.Windows.Forms.TextBox textBox1;
        private System.Windows.Forms.ColorDialog colorDialog1;
        private System.Windows.Forms.Button btnFontName;
        private System.Windows.Forms.Button btnFontColor;
    }
}