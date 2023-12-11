using Microsoft.VisualBasic;
using System;
using System.Collections.Generic;
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

namespace client_side
{
    /// <summary>
    /// Interaction logic for Files.xaml
    /// </summary>
    public partial class Files : Window
    {
        Communicator communicator;
        private readonly BackgroundWorker backgroundWorker;
        private readonly CancellationTokenSource cancellationTokenSource = new CancellationTokenSource();

        public Files(Communicator communicator)
        {
            InitializeComponent();
            Style = (Style)FindResource(typeof(Window));
            this.communicator = communicator;

            // Ensure the previous background worker is canceled
            if (backgroundWorker != null && backgroundWorker.IsBusy)
            {
                backgroundWorker.CancelAsync();
                cancellationTokenSource.Cancel();
            }

            backgroundWorker = new BackgroundWorker();
            backgroundWorker.WorkerReportsProgress = true;
            backgroundWorker.WorkerSupportsCancellation = true;
            backgroundWorker.DoWork += BackgroundWorker_DoWork;
            backgroundWorker.ProgressChanged += BackgroundWorker_ProgressChanged;
            backgroundWorker.RunWorkerAsync();

            lstFiles.KeyDown += LstFiles_KeyDown;
            btnJoin.IsEnabled = false;
        }

        private void LstFiles_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                e.Handled = true; // Prevent further processing of the Enter key
                // Enter key pressed, simulate the "Join" button click
                btnJoin_Click(sender, e);
            }
        }

        private void btnJoin_Click(object sender, RoutedEventArgs e)
        {
            string selectedFile = lstFiles.SelectedItem as string;
            if (selectedFile != null)
            {
                string FileName = selectedFile.Split("                             ")[0]; // Extract the room name

                backgroundWorker.CancelAsync();
                cancellationTokenSource.Cancel();
                string code = ((int)MessageCodes.MC_JOIN_FILE_REQUEST).ToString();
                communicator.SendData($"{code}{FileName.Length:D5}{FileName}{communicator.UserId}");
                TextEditor TextEditorWindow = new TextEditor(communicator, FileName);
                TextEditorWindow.Show();
                
                Close();
            }
        }

        private void lstFiles_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            btnJoin.IsEnabled = (lstFiles.SelectedItem != null);
        }

        private void BackgroundWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            BackgroundWorker worker = sender as BackgroundWorker;

            while (!worker.CancellationPending && !cancellationTokenSource.Token.IsCancellationRequested)
            {
                string code = ((int)MessageCodes.MC_GET_FILES_REQUEST).ToString();
                communicator.SendData($"{code}");

                string rep = communicator.ReceiveData();
                string repCode = rep.Substring(0, 3);
                if (repCode == ((int)MessageCodes.MC_GET_FILES_RESP).ToString() && rep.Length > 3)
                {
                    string[] receivedFileNames = rep.Substring(3).Split(';');

                    worker.ReportProgress(0, receivedFileNames);
                }
                
                Thread.Sleep(50000);
            }

        }
        private void BackgroundWorker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            if (e.UserState == null)
            {
                MessageBox.Show("something went wrong.");
                return;
            }
            string[] files = e.UserState as string[];
            List<string> items = new List<string>();
            foreach (var file in files)
            {
                items.Add(file);
            }
            lstFiles.ItemsSource = items;
        }
        private void btnReturnToMenu_Click(object sender, RoutedEventArgs e)
        {
            backgroundWorker.CancelAsync();
            cancellationTokenSource.Cancel();
            Menu mainWindow = new Menu(communicator);
            mainWindow.Show();
            Close();
        }

        private void CreateNewFileButton_Click(object sender, RoutedEventArgs e)
        {
            backgroundWorker.CancelAsync();
            cancellationTokenSource.Cancel();
            CreateNewFIle craeteFile = new CreateNewFIle(communicator);
            craeteFile.Show();
            Close();
        }
    }
}
