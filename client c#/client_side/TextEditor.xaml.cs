﻿using System;
using System.IO;
using System.Net.Sockets;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;

namespace client_side
{
    public partial class TextEditor : Window
    {
        private string filePath;
        private Communicator communicator;
        private CancellationTokenSource cancellationTokenSource;
        private Thread receiveServerUpdatesThread;
        private bool isCapsLockPressed = false;
        private bool isBackspaceHandled = false;
        public int UserId { get; set; }

        public TextEditor(Communicator communicator, string fileName)
        {
            InitializeComponent();
            Style = (Style)FindResource(typeof(Window));
            this.communicator = communicator;
            filePath = fileName;
            lblFileName.Text = fileName;
            try
            {
                ReceiveInitialContent(fileName);  // Receive initial content from the server

                // Start the thread to receive updates from the server
                cancellationTokenSource = new CancellationTokenSource();
                receiveServerUpdatesThread = new Thread(() => ReceiveServerUpdates())
                {
                    IsBackground = true
                };
                receiveServerUpdatesThread.Start();

                // Start the thread to get user input
                //new Thread(GetUserInput) { IsBackground = true }.Start();
            }
            catch (Exception)
            {
                MessageBox.Show("An error occurred while connecting to the server.");
                DisconnectFromServer();
                Application.Current.Shutdown();
                Environment.Exit(0);
            }
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
                        //CopySelectedText();
                        return;
                    }
                    else if (e.Key == Key.V)
                    {
                        // Ctrl+V (paste) is pressed
                        PasteClipboardContent(false);
                    }
                    else if (e.Key == Key.X)
                    {
                        // Ctrl+X (cut) is pressed
                        cutSelectedText();
                    }
                    else if (e.Key == Key.S) 
                    {
                        SaveFileContent();
                        return;
                    }
                    else if (e.Key == Key.W)
                    {
                        // Ctrl+W is pressed
                        // Close the window and send a disconnect message to the server
                        //DisconnectFromServer();
                        e.Handled = true;
                        CloseFile("menu");
                        return;
                    }
                }
                else if (e.Key == Key.Enter)
                {
                    // Enter key is pressed
                    HandleEnter();
                }
                else if (e.Key == Key.Tab)
                {
                    // Tab key is pressed
                    HandleTab();
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
                        communicator.LogAction($"{code}{selectionLength:D5}{replacementText.ToString().Length:D5}{replacementText}{index}");
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
                            communicator.LogAction($"{code}{inputString.Length:D5}{inputString}{index}");
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


        //           ******** Handlers ********* 
        private void HandleEnter()
        {
            try
            {
                // Get the current caret index
                int index = txtFileContent.CaretIndex;

                if (txtFileContent.SelectionLength > 0)
                {
                    // Replace the selected text with a new line
                    index = txtFileContent.SelectionStart;
                    int selectionLength = txtFileContent.SelectionLength;
                    txtFileContent.Text = txtFileContent.Text.Remove(index, selectionLength).Insert(index, Environment.NewLine);

                    // Send the replace action to the server
                    string code = ((int)MessageCodes.MC_REPLACE_REQUEST).ToString();
                    communicator.LogAction($"{code}{selectionLength:D5}{Environment.NewLine.Length:D5}{Environment.NewLine}{index}");
                    communicator.SendData($"{code}{selectionLength:D5}{Environment.NewLine.Length:D5}{Environment.NewLine}{index}");

                    // Set the caret index to the position after the inserted new line
                    txtFileContent.CaretIndex = index + Environment.NewLine.Length;
                }
                else
                {
                    // Insert a new line at the caret position
                    txtFileContent.Text = txtFileContent.Text.Insert(index, Environment.NewLine);

                    // Move the caret to the position after the inserted new line
                    txtFileContent.CaretIndex = index + Environment.NewLine.Length;

                    // Send the server a message about the Enter key press
                    string code = ((int)MessageCodes.MC_INSERT_REQUEST).ToString();
                    communicator.LogAction($"{code}{Environment.NewLine.Length:D5}{Environment.NewLine}{index}");
                    communicator.SendData($"{code}{Environment.NewLine.Length:D5}{Environment.NewLine}{index}");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error handling Enter key: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void HandleTab()
        {
            // Implement the logic to handle the Tab key press
            // For example, you can insert a tab character at the current caret position
            int index = txtFileContent.CaretIndex;
            string tabString = "    ";

            // Insert the tab character at the current caret position
            txtFileContent.Text = txtFileContent.Text.Insert(index, tabString);

            // Move the caret position after the inserted tab
            txtFileContent.CaretIndex = index + tabString.Length;

            // Send the Tab action to the server
            string code = ((int)MessageCodes.MC_INSERT_REQUEST).ToString();
            communicator.LogAction($"{code}{tabString.Length:D5}{tabString}{index}");
            communicator.SendData($"{code}{tabString.Length:D5}{tabString}{index}");
        }

        private void CopySelectedText()
        {
            // Copy the selected text to the clipboard
            string selectedText = txtFileContent.SelectedText;
            Clipboard.SetText(selectedText);
        }

        private void PasteClipboardContent(bool isButton)
        {
            // Paste the clipboard content at the current caret position
            int index = txtFileContent.CaretIndex;
            string clipboardContent = Clipboard.GetText();

            if (isButton) // if its comming from the button press it wont insert it automaticly
            {
                txtFileContent.Text = txtFileContent.Text.Insert(index, clipboardContent);
                txtFileContent.CaretIndex = index + clipboardContent.Length;
            }

            string code = ((int)MessageCodes.MC_INSERT_REQUEST).ToString();
            communicator.LogAction($"{code}{clipboardContent.Length:D5}{clipboardContent}{index}");
            communicator.SendData($"{code}{clipboardContent.Length:D5}{clipboardContent}{index}");
        }

        private void cutSelectedText()
        {
            // Copy the selected text to the clipboard
            CopySelectedText();

            int index = txtFileContent.SelectionStart;
            int selectionLength = txtFileContent.SelectionLength;

            string deletedText = txtFileContent.Text.Substring(index, selectionLength);
            txtFileContent.Text = txtFileContent.Text.Remove(index, selectionLength);

            string code = ((int)MessageCodes.MC_DELETE_REQUEST).ToString();
            communicator.LogAction($"{code}{selectionLength:D5}{index}");
            communicator.SendData($"{code}{selectionLength:D5}{index}"); // Delete action

            // Maintain the cursor position
            txtFileContent.CaretIndex = index;
        }

        private void DisconnectFromServer()
        {
            try
            {
                // Send a disconnect message to the server
                string disconnectCode = ((int)MessageCodes.MC_DISCONNECT).ToString();
                communicator.LogAction(disconnectCode);
                communicator.SendData(disconnectCode);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error disconnecting from the server: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void CloseFile(string req)
        {
            communicator.LogAction($"{((int)MessageCodes.MC_CLOSE_FILE_REQUEST).ToString()}" +
                            $"{filePath.Length:D5}{filePath}{communicator.UserId}");

            communicator.SendData($"{((int)MessageCodes.MC_CLOSE_FILE_REQUEST).ToString()}" +
                $"{filePath.Length:D5}{filePath}{communicator.UserId}");

            cancellationTokenSource?.Cancel();
            // Wait for the background thread to complete before closing the window
            //if (receiveServerUpdatesThread != null && receiveServerUpdatesThread.IsAlive)

            receiveServerUpdatesThread.Join();
            

            if (req == "menu")
            {
                Menu mainWindow = new Menu(communicator);
                mainWindow.Show();
                Close();
            }
            else if (req == "create") // add key stop listen
            {
                CreateNewFIle mainWindow = new CreateNewFIle(communicator);
                mainWindow.Show();
                Close();
            }
            else if (req == "show") // add key stop listen
            {
                Files mainWindow = new Files(communicator);
                mainWindow.Show();
                Close();
            }
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

                communicator.LogAction($"{code}{selectionLength:D5}{index}");
                communicator.SendData($"{code}{selectionLength:D5}{index}"); // Delete action

                // Maintain the cursor position
                txtFileContent.CaretIndex = index;
            }
            else if (index > 0)
            {
                // Delete a single character at the current index
                txtFileContent.Text = txtFileContent.Text.Remove(index - 1, 1);

                communicator.LogAction($"{code}00001{index - 1}");
                communicator.SendData($"{code}00001{index - 1}"); // Delete action with length 1
                // Maintain the cursor position
                txtFileContent.CaretIndex = index - 1;

            }
        }

        private char GetInputChar(Key key)
        {
            bool isShiftPressed = Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift);

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
                    bool isCapsLockPressed = Console.CapsLock;

                    char baseChar = char.ToLower((char)KeyInterop.VirtualKeyFromKey(key));
                    return (isShiftPressed ^ isCapsLockPressed) ? char.ToUpper(baseChar) : baseChar;

                case Key.D0:
                    return isShiftPressed ? ')' : '0';
                case Key.D1:
                    return isShiftPressed ? '!' : '1';
                case Key.D2:
                    return isShiftPressed ? '@' : '2';
                case Key.D3:
                    return isShiftPressed ? '#' : '3';
                case Key.D4:
                    return isShiftPressed ? '$' : '4';
                case Key.D5:
                    return isShiftPressed ? '%' : '5';
                case Key.D6:
                    return isShiftPressed ? '^' : '6';
                case Key.D7:
                    return isShiftPressed ? '&' : '7';
                case Key.D8:
                    return isShiftPressed ? '*' : '8';
                case Key.D9:
                    return isShiftPressed ? '(' : '9';

                case Key.OemComma:
                    return isShiftPressed ? '<' : ',';
                case Key.OemPeriod:
                    return isShiftPressed ? '>' : '.';
                case Key.OemQuestion:
                    return isShiftPressed ? '?' : '/';
                case Key.OemOpenBrackets:
                    return isShiftPressed ? '{' : '[';
                case Key.OemCloseBrackets:
                    return isShiftPressed ? '}' : ']';
                case Key.OemSemicolon:
                    return isShiftPressed ? ':' : ';';
                case Key.OemQuotes:
                    return isShiftPressed ? '"' : '\'';
                case Key.OemPipe:
                    return isShiftPressed ? '|' : '\\';
                case Key.OemTilde:
                    return isShiftPressed ? '~' : '`';
                case Key.OemMinus:
                    return isShiftPressed ? '_' : '-';
                case Key.OemPlus:
                    return isShiftPressed ? '+' : '=';

                case Key.Space:
                    return ' ';

                default:
                    return '\0'; // Invalid key
            }
        }

        private async void ReceiveServerUpdates()
        {
            try
            {
                while (true)
                {
                    // Receive update from the server
                    string update = communicator.ReceiveData();

                    // Parse the update message and update the TextBox accordingly
                    communicator.LogAction($"{update}");

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
                        case "300": // MC_DISCONNECT
                            break; // TODO inform the client about him leaving

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

        private void ReceiveInitialContent(string fileName)
        {
            try
            {
                string code = ((int)MessageCodes.MC_INITIAL_REQUEST).ToString();
                communicator.SendData($"{code}{fileName}");
                string initialContent = communicator.ReceiveData();
                communicator.LogAction(initialContent);
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

        private void SaveFileContent()
        {
            try
            {
                // Use the Text property to get the content of the TextBox
                string fileContent = txtFileContent.Text;
                File.WriteAllText(filePath, fileContent);
                //MessageBox.Show("File saved successfully.", "Success", MessageBoxButton.OK, MessageBoxImage.Information);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error saving file content: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
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

        //           ******** Buttons ********* 
        private void SaveButton_Click(object sender, RoutedEventArgs e)
        {
            SaveFileContent();
        }

        private void CopyButton_Click(object sender, RoutedEventArgs e)
        {
            CopySelectedText();
        }

        private void PasteButton_Click(object sender, RoutedEventArgs e)
        {
            PasteClipboardContent(true);
        }

        private void CutButton_Click(object sender, RoutedEventArgs e)
        {
            cutSelectedText();
        }

        private void ShowfilesButton_Click(object sender, RoutedEventArgs e)
        {
            CloseFile("show");
        }

        private void CreateFileButton_Click(object sender, RoutedEventArgs e)
        {
            CloseFile("create");
        }

    }  
}
