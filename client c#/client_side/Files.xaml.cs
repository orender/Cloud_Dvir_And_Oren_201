using Microsoft.VisualBasic;
using System;
using System.Collections.Generic;
using System.IO;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace client_side
{
    /// <summary>
    /// Interaction logic for Files.xaml
    /// </summary>
    public partial class Files : Window
    {
        Communicator communicator;
        bool disconnect = true; // if window closed by the user disconnect

        public Files(Communicator communicator)
        {
            InitializeComponent();
            Style = (Style)FindResource(typeof(Window));
            this.communicator = communicator;

            lstFiles.KeyDown += LstFiles_KeyDown;
            txtNewFileName.KeyDown += TxtNewFileName_KeyDown;

            string code = ((int)MessageCodes.MC_GET_FILES_REQUEST).ToString();
            communicator.SendData($"{code}");

            string rep = communicator.ReceiveData();
            string repCode = rep.Substring(0, 3);

            if (repCode == ((int)MessageCodes.MC_GET_FILES_RESP).ToString() && rep.Length > 3)
            {
                // Remove the response code from the received message
                string filesData = rep.Substring(3);
                List<FileModel> fileList = new List<FileModel>();

                int currentIndex = 0;
                // Loop through the file data array and create FileModel objects
                while (currentIndex < filesData.Length)
                {
                    // Extract data length for each message
                    int dataLength = int.Parse(filesData.Substring(currentIndex, 5));
                    currentIndex += 5;

                    // Extract data from the response
                    string data = filesData.Substring(currentIndex, dataLength);
                    currentIndex += dataLength;

                    fileList.Add(new FileModel { FileName = data });

                }

                // Set the ListBox's ItemsSource to the list of FileModel objects
                lstFiles.ItemsSource = fileList;
            }
            Closing += Files_CloseFile; // Hook up the closing event handler
        }

        private void LstFiles_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                e.Handled = true;
                Join(sender, e);
            }
        }

        private void lstFiles_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            e.Handled = true;
            Join(sender, e);
        }

        private void Join(object sender, RoutedEventArgs e)
        {
            FileModel selectedFile = lstFiles.SelectedItem as FileModel;
            if (selectedFile != null)
            {
                disconnect = false;
                string FileName = selectedFile.FileName;
                string code = ((int)MessageCodes.MC_JOIN_FILE_REQUEST).ToString();
                communicator.SendData($"{code}{FileName.Length:D5}{FileName}{communicator.UserId}");
                TextEditor TextEditorWindow = new TextEditor(communicator, FileName);
                TextEditorWindow.Show();
                Close();
            }

        }

        private void TxtNewFileName_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                if (txtNewFileName.Text.Length > 0)
                {
                    string code = ((int)MessageCodes.MC_CREATE_FILE_REQUEST).ToString();
                    communicator.SendData($"{code}{txtNewFileName.Text}");

                    string update = communicator.ReceiveData();
                    string rep = update.Substring(0, 3);

                    if (rep == ((int)MessageCodes.MC_CREATE_FILE_RESP).ToString())
                    {
                        e.Handled = true;
                        disconnect = false;
                        TextEditor textEditorWindow = new TextEditor(communicator, txtNewFileName.Text + ".txt");
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
        }

        private async void Files_CloseFile(object sender, EventArgs e)
        {
            if (disconnect)
            {
                try
                {
                    string chatMessageCode = ((int)MessageCodes.MC_DISCONNECT).ToString();

                    string fullMessage = $"{chatMessageCode}{communicator.UserId:D5}";

                    communicator.SendData(fullMessage);

                    // Close the window on the UI thread
                    await Dispatcher.InvokeAsync(() => Close());
                }
                catch (Exception ex)
                {
                    MessageBox.Show($"Error during closing: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
        }
    }

    public class FileModel
    {
        public string FileName { get; set; }
    }

}

