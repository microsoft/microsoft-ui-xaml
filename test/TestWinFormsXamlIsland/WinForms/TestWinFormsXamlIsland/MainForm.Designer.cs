namespace TestWinFormsXamlIsland
{
    partial class MainForm
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
            this.windowsXamlHost1 = new Microsoft.Toolkit.Forms.UI.XamlHost.WindowsXamlHost();
            this.SuspendLayout();
            // 
            // windowsXamlHost1
            // 
            this.windowsXamlHost1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowOnly;
            this.windowsXamlHost1.InitialTypeName = null;
            this.windowsXamlHost1.Location = new System.Drawing.Point(0, 0);
            this.windowsXamlHost1.Name = "windowsXamlHost1";
            this.windowsXamlHost1.Size = new System.Drawing.Size(800, 800);
            this.windowsXamlHost1.TabIndex = 0;
            this.windowsXamlHost1.Text = "windowsXamlHost1";
            this.windowsXamlHost1.Dock = System.Windows.Forms.DockStyle.Fill;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(800, 600);
            this.Controls.Add(this.windowsXamlHost1);
            this.Name = "Form1";
            this.Text = "WinForms .Net Core 3 and Xaml Islands";
            this.ResumeLayout(false);

        }

        #endregion

        private Microsoft.Toolkit.Forms.UI.XamlHost.WindowsXamlHost windowsXamlHost1;
    }
}

