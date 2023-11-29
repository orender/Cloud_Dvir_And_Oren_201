using System;
using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Windows;

namespace client_side
{
    public partial class MainWindow : Window
    {
        private string filePath = "path_to_your_file.txt"; // Replace with the actual file path
        private Communicator communicator;

        public MainWindow()
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
                ReceiveInitialContent();  // Receive initial content from the server

                // Start the thread to receive updates from the server
                new Thread(ReceiveServerUpdates) { IsBackground = true }.Start();

                // Start the thread to get user input
                new Thread(GetUserInput) { IsBackground = true }.Start();
            }
            catch (Exception)
            {
                MessageBox.Show("An error occurred while connecting to the server.");
                Application.Current.Shutdown();
                Environment.Exit(0);
            }
            LoadFileContent();
        }
        private void LoadFileContent()
        {
            try
            {
                if (File.Exists(filePath))
                {
                    string fileContent = File.ReadAllText(filePath);
                    txtFileContent.Text = fileContent;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error loading file content: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void SaveFileContent()
        {
            try
            {
                File.WriteAllText(filePath, txtFileContent.Text);
                MessageBox.Show("File saved successfully.", "Success", MessageBoxButton.OK, MessageBoxImage.Information);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error saving file content: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void SaveButton_Click(object sender, RoutedEventArgs e)
        {
            SaveFileContent();
        }

        private void GetUserInput()
        {
            try
            {
                while (true)
                {
                    char input = Console.ReadKey(intercept: true).KeyChar;

                    // Check if the input is a valid character
                    if (char.IsLetterOrDigit(input))
                    {
                        // Send the server a message with the input and its index
                        int index = txtFileContent.CaretIndex;
                        string message = $"{input},{index}";
                        communicator.SendData(message);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error getting user input: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void ReceiveServerUpdates()
        {
            try
            {
                while (true)
                {
                    // Receive update from the server
                    string update = communicator.ReceiveData();

                    // Parse the update message (e.g., "h,0" -> character 'h' at index 0)
                    char updatedChar = update[0];
                    int index = int.Parse(update.Substring(2));

                    // Update the UI with the new character at the specified index
                    Dispatcher.Invoke(() =>
                    {
                        // Ensure the index is within the bounds of the text
                        if (index >= 0 && index < txtFileContent.Text.Length)
                        {
                            txtFileContent.Text = txtFileContent.Text.Remove(index, 1).Insert(index, updatedChar.ToString());
                        }
                    });
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error receiving server updates: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void ReceiveInitialContent()
        {
            try
            {
                // Receive the initial content from the server
                string initialContent = communicator.ReceiveData();

                // Update the UI with the initial content
                Dispatcher.Invoke(() => txtFileContent.Text = initialContent);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error receiving initial content: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }
    }
}
