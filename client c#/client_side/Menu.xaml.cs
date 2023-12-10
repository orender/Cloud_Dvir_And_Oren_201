using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
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
    /// Interaction logic for Menu.xaml
    /// </summary>
    public partial class Menu : Window
    {
        private Communicator communicator;

        public Menu(Communicator communicator)
        {
            InitializeComponent();
            Style = (Style)FindResource(typeof(Window));

            this.communicator = communicator;
            txtFileName.Text = "Welcome user: " + communicator.UserId.ToString();
        }
        private void CreateNewFileButton_Click(object sender, RoutedEventArgs e)
        {
            CreateNewFIle craeteFile = new CreateNewFIle(communicator);
            craeteFile.Show();
            Close();
        }

        private void ShowExistingFilesButton_Click(object sender, RoutedEventArgs e)
        {
            Files showFiles = new Files(communicator);
            showFiles.Show();
            Close();
        }
    }
}
