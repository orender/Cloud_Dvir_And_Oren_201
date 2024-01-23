using System;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System;
using System.Data;
using System.Data.SQLite;

public class BinaryProtocol
{
    public static byte[] WriteMessage(int code, string message)
    {
        using (MemoryStream stream = new MemoryStream())
        using (BinaryWriter writer = new BinaryWriter(stream))
        {
            ushort length = (ushort)message.Length;

            // Write header
            writer.Write(IPAddress.HostToNetworkOrder(code));
            writer.Write(IPAddress.HostToNetworkOrder((short)length));

            // Write payload
            writer.Write(System.Text.Encoding.UTF8.GetBytes(message));

            return stream.ToArray();
        }
    }

    // Function to read a message from a buffer
    public static bool ReadMessage(byte[] buffer, out int code, out string message)
    {
        code = 0;
        message = null;

        using (MemoryStream stream = new MemoryStream(buffer))
        using (BinaryReader reader = new BinaryReader(stream))
        {
            try
            {
                // Read header
                code = IPAddress.NetworkToHostOrder(reader.ReadInt32());
                ushort length = reader.ReadUInt16();
                // Read payload
                byte[] payload = reader.ReadBytes(length);
                message = System.Text.Encoding.UTF8.GetString(payload);

                return true;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error reading message: {ex.Message}");
                return false;
            }
        }
    }
}

public class simpleDbHelper
{
    private readonly string connectionString;

    public simpleDbHelper(string dbName)
    {
        connectionString = $"Data Source={dbName};Version=3;";
    }

    public void CreateDatabase()
    {
        using (var connection = new SQLiteConnection(connectionString))
        {
            connection.Open();
            
            // Create a table with three columns (Modify as needed)
            using (var command = new SQLiteCommand(
                "CREATE TABLE IF NOT EXISTS BLOBS (" +
                "   ID INTEGER NOT NULL," +
                "   DATA TEXT NOT NULL" +
                ");", connection))
            {
                command.ExecuteNonQuery();
            }
        }
    }

    public int ExecuteNonQuery(string query, params SQLiteParameter[] parameters)
    {
        using (var connection = new SQLiteConnection(connectionString))
        using (var command = new SQLiteCommand(query, connection))
        {
            connection.Open();
            if (parameters != null)
            {
                command.Parameters.AddRange(parameters);
            }
            return command.ExecuteNonQuery();
        }
    }

    public object ExecuteScalar(string query, params SQLiteParameter[] parameters)
    {
        using (var connection = new SQLiteConnection(connectionString))
        using (var command = new SQLiteCommand(query, connection))
        {
            connection.Open();
            if (parameters != null)
            {
                command.Parameters.AddRange(parameters);
            }
            return command.ExecuteScalar();
        }
    }

    public DataTable ExecuteQuery(string query, params SQLiteParameter[] parameters)
    {
        using (var connection = new SQLiteConnection(connectionString))
        using (var command = new SQLiteCommand(query, connection))
        using (var adapter = new SQLiteDataAdapter(command))
        {
            if (parameters != null)
            {
                command.Parameters.AddRange(parameters);
            }

            var dataTable = new DataTable();
            adapter.Fill(dataTable);
            return dataTable;
        }
    }
}

enum Commands
{
    save = 1,
    get = 2,
    delete = 3,
    getAll = 4
}

class Program
{
    static void Main()
    {
        var dbHelper = new simpleDbHelper("test.db");

        // Create the database and tables (if not already created)
        dbHelper.CreateDatabase();
        IPAddress ipAdress = IPAddress.Parse("0.0.0.0");
        TcpListener myList = new TcpListener(ipAdress,42011);
        myList.Start();
        Socket s = myList.AcceptSocket();
        Console.WriteLine("Connection accepted from " + s.RemoteEndPoint);
        byte[] b = new byte[1024];

        DataTable result;
        int code = 14;
        int k = 0;
        string msg = "";
        int id = 0;
        string data = "";

        while(code!=69)
        {
            k = s.Receive(b);
            msg = "";
            if(BinaryProtocol.ReadMessage(b, out code, out msg)){
                Console.WriteLine("message was: " + msg);
                switch(code) 
                {
                    case (int)Commands.save:
                        id = int.Parse(msg.Substring(0,6));
                        result = dbHelper.ExecuteQuery("SELECT * FROM BLOBS WHERE ID = " + id);
                        foreach (DataRow row in result.Rows)
                        {
                            Console.WriteLine($"Column1: {row["ID"]}, Column2: {row["DATA"]}");
                            msg = "id already exists";
                            code = 42;
                            s.Send(BinaryProtocol.WriteMessage(code, msg));
                        }
                        if(code != 42)
                        {
                            data = msg.Substring(6);
                            dbHelper.ExecuteNonQuery("INSERT INTO BLOBS (ID, DATA) VALUES (@Value1, @Value2)",
                                new SQLiteParameter("@Value1", id),
                                new SQLiteParameter("@Value2", data));
                            msg = "data saved";
                            code = 420;
                            s.Send(BinaryProtocol.WriteMessage(code, msg));
                        }
                        break;
                    case (int)Commands.get:
                        id = int.Parse(msg.Substring(0));
                        result = dbHelper.ExecuteQuery("SELECT * FROM BLOBS WHERE ID = " + id);
                        foreach (DataRow row in result.Rows)
                        {
                            Console.WriteLine($"Column1: {row["ID"]}, Column2: {row["DATA"]}");
                            msg = $"{row["DATA"]}";
                            code = 420;
                            s.Send(BinaryProtocol.WriteMessage(code, msg));
                        }
                        break;
                    case (int)Commands.delete:
                        id = int.Parse(msg.Substring(0));
                        result = dbHelper.ExecuteQuery("DELETE FROM BLOBS WHERE ID = " + id);
                        msg = "blob deleted";
                        code = 420;
                        s.Send(BinaryProtocol.WriteMessage(code, msg));
                        break;
                    default:
                        break;
                }
            }
            else{
                msg = "error reading message";
                code = 69;
                s.Send(BinaryProtocol.WriteMessage(code, msg));
            }
        }
    }
}