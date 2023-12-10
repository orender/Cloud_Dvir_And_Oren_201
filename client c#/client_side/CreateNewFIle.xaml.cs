using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace client_side
{
    /// <summary>
    /// Interaction logic for CreateNewFIle.xaml
    /// </summary>
    public partial class CreateNewFIle : Window
    {
        private Communicator communicator;
        public CreateNewFIle(Communicator communicator)
        {
            InitializeComponent();
            Style = (Style)FindResource(typeof(Window));
            this.communicator = communicator;
        }

        private void btnCreateFile_Click(object sender, RoutedEventArgs e)
        {
            if (txtFileName.Text.Length > 0)
            {
                string code = ((int)MessageCodes.MC_CREATE_FILE_REQUEST).ToString();
                communicator.LogAction($"{code}{txtFileName.Text}.txt");
                communicator.SendData($"{code}{txtFileName.Text}.txt");

                string update = communicator.ReceiveData();
                communicator.LogAction($"{update}");
                string rep = update.Substring(0, 3);

                if (rep == ((int)MessageCodes.MC_CREATE_FILE_RESP).ToString())
                {
                    TextEditor textEditorWindow = new TextEditor(communicator, txtFileName.Text + ".txt");
                    textEditorWindow.Show();
                    Close();
                }
                else if (rep == ((int)MessageCodes.MC_ERR_RESP).ToString())
                {
                    lblErr.Content = "file already exist";
                    return;
                }
            }
        }

        private void btnReturnToMenu_Click(object sender, RoutedEventArgs e)
        {
            Menu mainWindow = new Menu(communicator);
            mainWindow.Show();
            Close();
        }
    }
}
