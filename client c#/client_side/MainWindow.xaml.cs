﻿using System;
using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Windows.Input;
using System.Windows;
using System.Windows.Documents;
using System.Reflection;
using Microsoft.VisualBasic;

namespace client_side
{
    public partial class MainWindow : Window
    {
        private string filePath = "file.txt"; // Replace with the actual file path
        private Communicator communicator;
        private bool isCapsLockPressed = false;
        private bool isBackspaceHandled = false;

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
                //new Thread(GetUserInput) { IsBackground = true }.Start();
            }
            catch (Exception)
            {
                MessageBox.Show("An error occurred while connecting to the server.");
                Application.Current.Shutdown();
                Environment.Exit(0);
            }
        }

        private void SaveFileContent()
        {
            try
            {
                // Use the Text property to get the content of the TextBox
                string fileContent = txtFileContent.Text;
                File.WriteAllText(filePath, fileContent);
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

        private void TextBoxInput_KeyDown(object sender, KeyEventArgs e)
        {
            try
            {
                // Check if Caps Lock key is pressed
                if (e.Key == Key.CapsLock)
                {
                    isCapsLockPressed = !isCapsLockPressed;
                    return; // Do not send a message for Caps Lock key press
                }

                
                if (Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl))
                {
                    // Ctrl key is pressed
                    if (e.Key == Key.C)
                    {
                        // Ctrl+C (copy) is pressed
                        CopySelectedText();
                    }
                    else if (e.Key == Key.V)
                    {
                        // Ctrl+V (paste) is pressed
                        PasteClipboardContent();
                    }
                    else if (e.Key == Key.X)
                    {
                        // Ctrl+X (cut) is pressed
                        cutSelectedText();
                    }
                }
                
                else if (e.Key == Key.Back)
                {
                    e.Handled = true;
                    HandleBackspace();
                }
                else if (e.Key != Key.LeftShift && e.Key != Key.RightShift && e.Key != Key.LeftCtrl && e.Key != Key.RightCtrl && e.Key != Key.LeftAlt && e.Key != Key.RightAlt)
                {
                    int index = txtFileContent.SelectionStart;
                    int selectionLength = txtFileContent.SelectionLength;

                    string code;

                    if (selectionLength > 0)
                    {
                        // Replace the selected text
                        string selectedText = txtFileContent.Text.Substring(index, selectionLength);
                        char replacementText = GetInputChar(e.Key);
                        txtFileContent.Text = txtFileContent.Text.Remove(index, selectionLength);

                        code = ((int)MessageCodes.MC_REPLACE_REQUEST).ToString();
                        LogAction($"{code}{selectionLength:D5}{replacementText.ToString().Length:D5}{replacementText}{index}");
                        communicator.SendData($"{code}{selectionLength:D5}{replacementText.ToString().Length:D5}{replacementText}{index}"); // Replace action
                        txtFileContent.CaretIndex = index;
                    }
                    else
                    {
                        char inputChar = GetInputChar(e.Key);

                        if (inputChar != '\0')
                        {
                            string inputString = inputChar.ToString();

                            code = ((int)MessageCodes.MC_INSERT_REQUEST).ToString();
                            LogAction($"{code}{inputString.Length:D5}{inputString}{index}");
                            communicator.SendData($"{code}{inputString.Length:D5}{inputString}{index}"); // Insert action
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error handling key down: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void CopySelectedText()
        {
            // Copy the selected text to the clipboard
            string selectedText = txtFileContent.SelectedText;
            Clipboard.SetText(selectedText);
        }
        private void cutSelectedText()
        {
            // Copy the selected text to the clipboard
            string selectedText = txtFileContent.SelectedText;
            Clipboard.SetText(selectedText);

            int index = txtFileContent.SelectionStart;
            string clipboardContent = Clipboard.GetText();
            //txtFileContent.Text = txtFileContent.Text.Remove(index, clipboardContent);
            txtFileContent.CaretIndex = index - clipboardContent.Length;

            string code = ((int)MessageCodes.MC_DELETE_REQUEST).ToString();
            LogAction($"{code}{clipboardContent.Length:D5}{index}");
            communicator.SendData($"{code}{clipboardContent.Length:D5}{index}");
        }

        private void PasteClipboardContent()
        {
            // Paste the clipboard content at the current caret position
            int index = txtFileContent.SelectionStart;
            string clipboardContent = Clipboard.GetText();
            //txtFileContent.Text = txtFileContent.Text.Insert(index, clipboardContent);
            txtFileContent.CaretIndex = index + clipboardContent.Length;

            // Send the paste action to the server as an insert
            string code = ((int)MessageCodes.MC_INSERT_REQUEST).ToString();
            LogAction($"{code}{clipboardContent.Length:D5}{clipboardContent}{index}");
            communicator.SendData($"{code}{clipboardContent.Length:D5}{clipboardContent}{index}"); // Insert action
        }

        private void HandleBackspace()
        {
            int index = txtFileContent.SelectionStart;
            int selectionLength = txtFileContent.SelectionLength;

            string code = ((int)MessageCodes.MC_DELETE_REQUEST).ToString();

            if (selectionLength > 0)
            {
                // Delete the selected text
                string deletedText = txtFileContent.Text.Substring(index, selectionLength);
                txtFileContent.Text = txtFileContent.Text.Remove(index, selectionLength);

                LogAction($"{code}{selectionLength:D5}{index}");
                communicator.SendData($"{code}{selectionLength:D5}{index}"); // Delete action

                // Maintain the cursor position
                txtFileContent.CaretIndex = index;
            }
            else if (index > 0)
            {
                // Delete a single character at the current index
                txtFileContent.Text = txtFileContent.Text.Remove(index - 1, 1);

                LogAction($"{code}00001{index - 1}");
                communicator.SendData($"{code}00001{index - 1}"); // Delete action with length 1
                // Maintain the cursor position
                txtFileContent.CaretIndex = index - 1;

            }
        }

        private char GetInputChar(Key key)
        {
            switch (key)
            {
                case Key.A:
                case Key.B:
                case Key.C:
                case Key.D:
                case Key.E:
                case Key.F:
                case Key.G:
                case Key.H:
                case Key.I:
                case Key.J:
                case Key.K:
                case Key.L:
                case Key.M:
                case Key.N:
                case Key.O:
                case Key.P:
                case Key.Q:
                case Key.R:
                case Key.S:
                case Key.T:
                case Key.U:
                case Key.V:
                case Key.W:
                case Key.X:
                case Key.Y:
                case Key.Z:
                    bool isShiftPressed = Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift);
                    bool isCapsLockPressed = Console.CapsLock;

                    char baseChar = char.ToLower((char)KeyInterop.VirtualKeyFromKey(key));
                    return (isShiftPressed ^ isCapsLockPressed) ? char.ToUpper(baseChar) : baseChar;

                case Key.D0:
                case Key.D1:
                case Key.D2:
                case Key.D3:
                case Key.D4:
                case Key.D5:
                case Key.D6:
                case Key.D7:
                case Key.D8:
                case Key.D9:
                case Key.Space:
                    return (char)KeyInterop.VirtualKeyFromKey(key);

                default:
                    return '\0'; // Invalid key
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

                    // Parse the update message and update the TextBox accordingly
                    LogAction($"{update}");

                    string code = update.Substring(0, 3); // Assuming the message code is always 3 characters

                    switch (code)
                    {
                        case "202": // MC_INSERT_RESP
                            HandleInsertResponse(update);
                            break;

                        case "203": // MC_DELETE_RESP
                            HandleDeleteResponse(update);
                            break;

                        case "204": // MC_REPLACE_RESP
                            HandleReplaceResponse(update);
                            break;
                        // Add more cases as needed for other message types

                        default:
                            throw new InvalidOperationException($"Unknown message code: {code}");
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error receiving server updates: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void HandleInsertResponse(string update)
        {
            int currentIndex = txtFileContent.CaretIndex;
            // Insert message format: "202{lenOfInput}{input}{index}"
            int lenIndex = 3; // Start index of lenOfInput
            int lenOfInput = int.Parse(update.Substring(lenIndex, 5));
            int inputIndex = lenIndex + 5;
            string input = update.Substring(inputIndex, lenOfInput);
            int indexIndex = inputIndex + lenOfInput;
            int index = int.Parse(update.Substring(indexIndex));

            // Update the TextBox with the new input at the specified index
            Dispatcher.Invoke(() =>
            {
                // Ensure the index is within the bounds of the text
                if (index >= 0 && index <= txtFileContent.Text.Length)
                {
                    StringBuilder sb = new StringBuilder(txtFileContent.Text);
                    sb.Insert(index, input);
                    txtFileContent.Text = sb.ToString();

                    // Check if the new index is after the current index
                    if (currentIndex >= index)
                    {
                        // Adjust the caret index based on the length of the inserted text
                        txtFileContent.CaretIndex = currentIndex + lenOfInput;
                    }
                    else
                    {
                        // Keep the caret index unchanged
                        txtFileContent.CaretIndex = currentIndex;
                    }
                }
            });
        }

        private void HandleDeleteResponse(string update)
        {
            int currentIndex = txtFileContent.CaretIndex;
            // Delete message format: "{code}{length}{index}"
            int lengthIndex = 3; // Start index of length
            int length = int.Parse(update.Substring(lengthIndex, 5));

            int indexIndex = lengthIndex + 5; // Start index of index
            int indexLength = update.Length - indexIndex; // Determine the length of the index part
            int index = int.Parse(update.Substring(indexIndex, indexLength));

            // Update the TextBox by removing the specified length of characters at the specified index
            Dispatcher.Invoke(() =>
            {
                // Ensure the index is within the bounds of the text
                if (index >= 0 && index < txtFileContent.Text.Length)
                {
                    StringBuilder sb = new StringBuilder(txtFileContent.Text);
                    sb.Remove(index, length);
                    txtFileContent.Text = sb.ToString();

                    // Check if the new index is after an inserted character
                    if (currentIndex > index)
                    {
                        // Adjust the caret index based on the length of the removed text
                        txtFileContent.CaretIndex = currentIndex - length;
                    }
                    else
                    {
                        // Keep the caret index unchanged
                        txtFileContent.CaretIndex = currentIndex;
                    }
                }
            });
        }

        private void HandleReplaceResponse(string update)
        {
            int currentIndex = txtFileContent.CaretIndex;
            // Replace message format: "{code}{lengthToRemove}{replacementTextLength}{replacementText}{index}"

            // Extract information from the response
            int lengthToRemoveIndex = 3; // Start index of lengthToRemove
            int lengthToRemove = int.Parse(update.Substring(lengthToRemoveIndex, 5));

            int replacementTextLengthIndex = lengthToRemoveIndex + 5; // Start index of replacementTextLength
            int replacementTextLength = int.Parse(update.Substring(replacementTextLengthIndex, 5));

            int replacementTextIndex = replacementTextLengthIndex + 5; // Start index of replacementText
            string replacementText = update.Substring(replacementTextIndex, replacementTextLength);

            int indexIndex = replacementTextIndex + replacementTextLength; // Start index of index
            int index = int.Parse(update.Substring(indexIndex, update.Length - indexIndex));

            // Update the TextBox by removing the specified length of characters and inserting the replacement text at the specified index
            Dispatcher.Invoke(() =>
            {
                // Ensure the index is within the bounds of the text
                if (index >= 0 && index < txtFileContent.Text.Length)
                {
                    StringBuilder sb = new StringBuilder(txtFileContent.Text);
                    sb.Remove(index, lengthToRemove);
                    sb.Insert(index, replacementText);
                    txtFileContent.Text = sb.ToString();

                    // Check if the new index is after the current index
                    if (currentIndex >= index)
                    {
                        // Adjust the caret index based on the difference in length
                        txtFileContent.CaretIndex = currentIndex + replacementText.Length - lengthToRemove;
                    }
                    else
                    {
                        // Keep the caret index unchanged
                        txtFileContent.CaretIndex = currentIndex;
                    }
                }
            });
        }

        private void ReceiveInitialContent()
        {
            try
            {
                string code = ((int)MessageCodes.MC_INITIAL_REQUEST).ToString();
                communicator.SendData($"{code}00000");
                string initialContent = communicator.ReceiveData();

                string codeString = initialContent.Substring(0, 3);
                if (codeString == ((int)MessageCodes.MC_INITIAL_RESP).ToString())
                {
                    initialContent = initialContent.Substring(3);

                    // Extract length and data
                    string lengthString = initialContent.Substring(0, 5);
                    int length = int.Parse(lengthString);
                    string data = initialContent.Substring(5);

                    // Ensure the length matches the actual data length
                    if (length == data.Length)
                    {
                        // Update the TextBox with the initial content
                        Dispatcher.Invoke(() =>
                        {
                            txtFileContent.Text = data;
                        });
                    }
                    else
                    {
                        // Handle the case where the length does not match the data length
                        MessageBox.Show("Error: Length mismatch in initial content.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error receiving initial content: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void LogAction(string action)
        {
            try
            {
                string logFilePath = "log.txt";

                // Append the action to the log file
                File.AppendAllText(logFilePath, $"{DateTime.Now}: {action}\n");
            }
            catch (Exception ex)
            {
                // Handle the exception or log it to another source
                MessageBox.Show($"Error logging action: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void txtFileContent_TextChanged(object sender, System.Windows.Controls.TextChangedEventArgs e)
        {
            if (isBackspaceHandled)
            {
                // Reset the flag after handling backspace
                isBackspaceHandled = false;

                // Ensure the caret index is correctly set after backspace
                txtFileContent.CaretIndex = Math.Max(0, txtFileContent.CaretIndex);
            }
        }
    }  
}
