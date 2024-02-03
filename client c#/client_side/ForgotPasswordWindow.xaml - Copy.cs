using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
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
    /// Interaction logic for ForgotPasswordWindow.xaml
    /// </summary>
    public partial class ForgotPasswordWindow : Window
    {
        private Communicator communicator;

        public ForgotPasswordWindow(Communicator communicator)
        {
            this.communicator = communicator;
            InitializeComponent();
            Style = (Style)FindResource(typeof(Window));
        }

        private void btnSend_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (String.IsNullOrEmpty(txtUserName.Text))
                {
                    lblErrMsg.Content = "User name can't be empty";
                    return;
                }

                string userName = txtUserName.Text;
                string code = ((int)MessageCodes.MC_FORGOT_PASSW_REQUEST).ToString();

                // Send the request to the server
                communicator.SendData($"{code}{userName.Length:D5}{userName}");

                // Receive response from the server
                string response = communicator.ReceiveData();

                // Check the response from the server
                if (response.StartsWith(((int)MessageCodes.MC_FORGOT_PASSW_RESP).ToString()))
                {
                    // The server sent a response, handle it as needed
                    // For example, you can display a message to the user
                    lblErrMsg.Content = "Password reset email sent. Check your email for further instructions.";
                }
                else
                {
                    // Handle other responses or errors from the server
                    lblErrMsg.Content = "Error: Failed to initiate password reset.";
                }
            }
            catch (Exception ex)
            {
                lblErrMsg.Content = ex.Message;
            }
        }

        private void hprlnkBackToLogin(object sender, RoutedEventArgs e)
        {
            LoginWindow loginWindow = new LoginWindow(communicator);
            loginWindow.Show();
            Close();
        }
    }
}
