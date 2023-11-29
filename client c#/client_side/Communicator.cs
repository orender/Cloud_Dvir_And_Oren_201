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

namespace client_side
{
    public class Communicator
    {
        private Socket m_socket;
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


    }
}
