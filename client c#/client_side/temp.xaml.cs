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
    /// latter will be login 
    /// </summary>
    public partial class temp : Window
    {
        private Communicator communicator;

        public temp()
        {
            InitializeComponent();
            Style = (Style)FindResource(typeof(Window));

            StreamReader sr = new StreamReader(".\\config.txt");
            string? line = sr.ReadLine();
            sr.Close();

            if (line == null)
            {
                throw new Exception("Please create file `config.txt` in the directory with the binary and set its contents in the following format: `SERVER_IP,SERVER_PORT`");
            }

            string[] ip_port = line.Split(",");

            try
            {
                communicator = new Communicator(ip_port[0], int.Parse(ip_port[1]));

                string receivedData = communicator.ReceiveData();

                // Extract the message code from the received data
                int receivedMessageCode = int.Parse(receivedData.Substring(0, 3));

                if (receivedMessageCode == (int)MessageCodes.MC_CLIENT_ID)
                {
                    // Extract the assigned client ID
                    communicator.UserId = int.Parse(receivedData.Substring(3));
                }
                Files mainWindow = new Files(communicator);
                mainWindow.Show();
                Close();
            }
            catch (Exception)
            {
                MessageBox.Show("An error occurred while connecting to the server.");
                Application.Current.Shutdown();
                Environment.Exit(0);
            }
        }
    }
}
