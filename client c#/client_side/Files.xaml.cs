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
        private Thread receiveServerUpdatesThread;

        private bool isListeningToServer = true;
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

                // Sort the fileList alphabetically by file name
                fileList.Sort((a, b) => string.Compare(a.FileName, b.FileName, StringComparison.OrdinalIgnoreCase));

                // Set the ListBox's ItemsSource to the sorted list of FileModel objects
                lstFiles.ItemsSource = fileList;
            }

            receiveServerUpdatesThread = new Thread(() => ReceiveServerUpdates())
            {
                IsBackground = true
            };
            receiveServerUpdatesThread.Start();

            Closing += Files_CloseFile; // Hook up the closing event handler
        }

        private void LstFiles_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                e.Handled = true;
                Join(sender, e);
            }
            if (e.Key == Key.Back) 
            {
                remove(sender, e);
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
                isListeningToServer = false;
                string FileName = selectedFile.FileName;
                string code = ((int)MessageCodes.MC_JOIN_FILE_REQUEST).ToString();
                communicator.SendData($"{code}{FileName.Length:D5}{FileName}{communicator.UserId}");
                TextEditor TextEditorWindow = new TextEditor(communicator, FileName);
                TextEditorWindow.Show();
                Close();
            }

        }

        private void remove(object sender, RoutedEventArgs e)
        {
            FileModel selectedFile = lstFiles.SelectedItem as FileModel;
            if (selectedFile != null)
            {
                // remove the .txt at the end
                int newLength = selectedFile.FileName.Length - 4;
                string stringWithoutLast4Chars = selectedFile.FileName.Substring(0, newLength);
                string code = ((int)MessageCodes.MC_DELETE_FILE_REQUEST).ToString();
                communicator.SendData($"{code}{stringWithoutLast4Chars.Length:D5}{stringWithoutLast4Chars}");
            }
        }

        private void TxtNewFileName_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                if (txtNewFileName.Text.Length > 0 && IsValidMessage(txtNewFileName.Text))
                {
                    if (FileExists(txtNewFileName.Text + ".txt"))
                    {
                        lblErr.Content = "File already exists";
                        return;
                    }
                    e.Handled = true;
                    disconnect = false;
                    isListeningToServer = false;
                    string code = ((int)MessageCodes.MC_CREATE_FILE_REQUEST).ToString();
                    communicator.SendData($"{code}{txtNewFileName.Text}");

                    TextEditor textEditorWindow = new TextEditor(communicator, txtNewFileName.Text + ".txt");
                    textEditorWindow.Show();
                    Close();
                    return;
                }
            }
        }

        private bool IsValidMessage(string message)
        {
            // allow only letters, numbers, and specific special characters
            return System.Text.RegularExpressions.Regex.IsMatch(message, @"^[A-Za-z0-9,.""';:\[\]{}\-+=_!@#$%^&*()<>?/~` ]+$");
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

        private async void ReceiveServerUpdates()
        {
            try
            {
                while (isListeningToServer)
                {
                    // Receive update from the server
                    string update = communicator.ReceiveData();

                    string code = update.Substring(0, 3); // Assuming the message code is always 3 characters

                    switch (code)
                    {
                        case "207": // MC_ADD_FILE_RESP
                            HandleAddFile(update);
                            break;

                        case "214":
                            HandleDeleteFile(update);
                            break;

                        case "200": // MC_ERR_RESP
                            HandleError(update);
                            break;

                        case "302": // MC_APPROVE_RESP
                            break;

                        default:
                            throw new InvalidOperationException($"Unknown message code: {code}");
                    }
                }
                //receiveServerUpdatesThread.Abort();
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error receiving server updates: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void HandleAddFile(string update)
        {
            string msg = update.Substring(3);

            // Create a new FileModel object and add it to the ListBox
            FileModel newFile = new FileModel { FileName = msg };

            Dispatcher.Invoke(() =>
            {
                if (lstFiles.ItemsSource is List<FileModel> fileList)
                {
                    // Find the index to insert the new file
                    int insertIndex = 0;
                    while (insertIndex < fileList.Count && string.Compare(fileList[insertIndex].FileName, newFile.FileName, StringComparison.Ordinal) < 0)
                    {
                        insertIndex++;
                    }

                    // Insert the new file at the correct position
                    fileList.Insert(insertIndex, newFile);

                    // Set the ListBox's ItemsSource again to trigger the update
                    lstFiles.ItemsSource = null;
                    lstFiles.ItemsSource = fileList;
                }
            });
        }

        private void HandleDeleteFile(string update)
        {
            string deletedFileName = update.Substring(3);

            Dispatcher.Invoke(() =>
            {
                if (lstFiles.ItemsSource is List<FileModel> fileList)
                {
                    // Find the index of the file to delete
                    int deleteIndex = fileList.FindIndex(file => file.FileName == deletedFileName);

                    // If the file is found, remove it from the list
                    if (deleteIndex != -1)
                    {
                        fileList.RemoveAt(deleteIndex);

                        // Set the ListBox's ItemsSource again to trigger the update
                        lstFiles.ItemsSource = null;
                        lstFiles.ItemsSource = fileList;
                    }
                }
            });
        }

        private void HandleError(string update)
        {
            string msg = update.Substring (3);
            Dispatcher.Invoke(() =>
            {
                lblErr.Content = msg;
            });
        }

        private bool FileExists(string fileName)
        {
            foreach (var item in lstFiles.Items)
            {
                if (item is FileModel file && file.FileName == fileName)
                {
                    return true;
                }
            }
            return false;
        }

    }

    public class FileModel
    {
        public string FileName { get; set; }
    }

}

