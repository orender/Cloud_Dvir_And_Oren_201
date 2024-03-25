using System;
using System.Text;
using System.Net;
using System.Net.Sockets;
using System;
using System.Data;
using System.Data.SQLite;
using System.Security.Cryptography;

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
    // Constant key and IV
    private static readonly byte[] key = new byte[]
    {
        0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
        0xAB, 0xF7, 0x97, 0x22, 0x8A, 0x5E, 0x65, 0x17
    };
    private static readonly byte[] iv = new byte[]
    {
        0x3A, 0xD7, 0x95, 0x22, 0x4F, 0x60, 0x37, 0x8A,
        0xA1, 0x2B, 0x22, 0x2A, 0x33, 0x66, 0x7A, 0x79
    };

    public static byte[] Encrypt(string plainText)
    {
        using (Aes aesAlg = Aes.Create())
        {
            aesAlg.Key = key;
            aesAlg.IV = iv;

            ICryptoTransform encryptor = aesAlg.CreateEncryptor(aesAlg.Key, aesAlg.IV);

            using (MemoryStream msEncrypt = new MemoryStream())
            {
                using (CryptoStream csEncrypt = new CryptoStream(msEncrypt, encryptor, CryptoStreamMode.Write))
                {
                    using (StreamWriter swEncrypt = new StreamWriter(csEncrypt))
                    {
                        swEncrypt.Write(plainText);
                    }
                    return msEncrypt.ToArray();
                }
            }
        }
    }

    public static string Decrypt(byte[] cipherText)
    {
        using (Aes aesAlg = Aes.Create())
        {
            aesAlg.Key = key;
            aesAlg.IV = iv;

            ICryptoTransform decryptor = aesAlg.CreateDecryptor(aesAlg.Key, aesAlg.IV);

            using (MemoryStream msDecrypt = new MemoryStream(cipherText))
            {
                using (CryptoStream csDecrypt = new CryptoStream(msDecrypt, decryptor, CryptoStreamMode.Read))
                {
                    using (StreamReader srDecrypt = new StreamReader(csDecrypt))
                    {
                        return srDecrypt.ReadToEnd();
                    }
                }
            }
        }
    }

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
                Console.WriteLine("message code was: " + code + ", message was: " + msg);
                if(msg == "")
                {
                    return;
                }
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
                                new SQLiteParameter("@Value2", Convert.ToBase64String(Encrypt(data))));
                            msg = "data saved";
                            code = 420;
                            s.Send(BinaryProtocol.WriteMessage(code, msg));

                            Console.WriteLine("saved: " + Convert.ToBase64String(Encrypt(data)));
                        }
                        break;
                    case (int)Commands.get:
                        id = int.Parse(msg.Substring(0));
                        result = dbHelper.ExecuteQuery("SELECT * FROM BLOBS WHERE ID = " + id);
                        foreach (DataRow row in result.Rows)
                        {
                            Console.WriteLine($"Column1: {row["ID"]}, Column2: {row["DATA"]}");
                            msg = Decrypt(Convert.FromBase64String($"{row["DATA"]}"));
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