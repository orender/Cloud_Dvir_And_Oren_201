using System;
using System.Text;
using System.Net;
using System.Net.Sockets;

 Console.WriteLine("1");

IPAddress ipAdress = IPAddress.Parse("0.0.0.0"); 
// You can use local IP as far as you use the 
//same in client
 Console.WriteLine("2");
// Initializes the Listener
TcpListener myList = new TcpListener(ipAdress,42011);
 Console.WriteLine("3");
// Start Listeneting at the specified port
myList.Start();
 Console.WriteLine("4");
 Socket s = myList.AcceptSocket();
Console.WriteLine("5");
 // When accepted
 Console.WriteLine("Connection accepted from "
                         + s.RemoteEndPoint);
Console.WriteLine("6");
 byte[] b = new byte[100];
 int k = s.Receive(b);
 Console.WriteLine("Recieved...");
 
for (int i=0;i<k;i++)
{
    Console.Write(Convert.ToChar(b[i]));
}

ASCIIEncoding asen = new ASCIIEncoding();
s.Send(asen.GetBytes("hello"));
Console.WriteLine("\n Automatic message sent!");

s.Close();
myList.Stop();