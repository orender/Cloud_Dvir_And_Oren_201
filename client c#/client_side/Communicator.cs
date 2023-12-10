using Microsoft.VisualBasic;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Security.Policy;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows;
using System.IO;
using System.Threading;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;

namespace client_side
{
    enum MessageCodes
    {
        MC_INITIAL_REQUEST = 101, //requests
        MC_INSERT_REQUEST = 102,
        MC_DELETE_REQUEST = 103,
        MC_REPLACE_REQUEST = 104,
        MC_CREATE_FILE_REQUEST = 105,
        MC_GET_FILES_REQUEST = 106,
        MC_CLOSE_FILE_REQUEST = 108,
        MC_ERR_RESP = 200, //responses
        MC_INITIAL_RESP = 201,
        MC_INSERT_RESP = 202,
        MC_DELETE_RESP = 203,
        MC_REPLACE_RESP = 204,
        MC_CREATE_FILE_RESP = 205,
        MC_GET_FILES_RESP = 206,
        MC_CLOSE_FILE_RESP = 208,
        MC_DISCONNECT = 300, //user
        MC_CLIENT_ID = 301

    };
    public class Communicator
    {
        private Socket m_socket;
        public int UserId { get; set; }
        public Communicator(string ip, int port)
        {
            m_socket = new Socket(SocketType.Stream, ProtocolType.Tcp);
            if (m_socket == null)
            {
                throw new ExternalException("Socket failed to initialize");
            }

            m_socket.Connect(ip, port);

        }

        ~Communicator()
        {
            m_socket.Close();
        }

        public void SendData(string message)
        {
            byte[] data = Encoding.UTF8.GetBytes(message);
            m_socket.Send(data);
        }

        public string ReceiveData()
        {
            byte[] buffer = new byte[1024];  // Adjust the buffer size as needed
            int bytesRead = m_socket.Receive(buffer);
            return Encoding.UTF8.GetString(buffer, 0, bytesRead);
        }

        public string ReceiveDataWithTimeout(TimeSpan timeout)
        {
            try
            {
                using (var cts = new CancellationTokenSource(timeout))
                {
                    var token = cts.Token;

                    Task<string> receiveTask = Task.Run(() =>
                    {
                        // Replace the following line with your actual ReceiveData logic
                        // Here, I'm assuming ReceiveData returns a string
                        return ReceiveData();
                    }, token);

                    // Wait for the task to complete within the specified timeout
                    if (receiveTask.Wait(timeout))
                    {
                        // Task completed within the timeout
                        return receiveTask.Result;
                    }
                    else
                    {
                        // Timeout occurred
                        return null;
                    }
                }
            }
            catch (Exception ex)
            {
                // Handle exceptions as needed
                Console.WriteLine($"Error in ReceiveDataWithTimeout: {ex.Message}");
                return null;
            }
        }

        public void LogAction(string action)
        {
            try
            {
                string logFilePath = "UserLog.txt";

                // Append the action to the log file
                File.AppendAllText(logFilePath, $"{DateTime.Now}: {action}\n");
                //File.AppendAllText(logFilePath, $"{action}\n"); // - just the msg without date
            }
            catch (Exception ex)
            {
                // Handle the exception or log it to another source
                MessageBox.Show($"Error logging action: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }
    }
}
