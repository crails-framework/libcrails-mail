#include <crails/utils/backtrace.hpp>
#include <crails/utils/base64.hpp>
#include <crails/server.hpp>
#include <crails/logger.hpp>
#include <boost/array.hpp>
#include <thread>
#include "smtp.hpp"

using namespace std;
using namespace Crails;
using namespace boost::asio::ip;

Smtp::Server::Server() :
  ssl_context(boost::asio::ssl::context::tlsv12),
  ssl_sock(Crails::Server::get_io_context(), ssl_context),
  sock(ssl_sock.next_layer()),
  tls_enabled(false)
{
}

void Smtp::Server::connect(const std::string& hostname, unsigned short port, std::function<void()> callback)
{
  boost::system::error_code error = boost::asio::error::host_not_found;
  tcp::resolver             resolver(Crails::Server::get_io_context());
  tcp::resolver::query      query(hostname, std::to_string(port));
  tcp::resolver::iterator   endpoint_iterator = resolver.resolve(query);
  tcp::resolver::iterator   end;

  logger << Logger::Debug << "Crails::Smtp::Server::connect to " << hostname << ':' << port << Logger::endl;
  while (error && endpoint_iterator != end)
  {
    sock.close();
    sock.connect(*endpoint_iterator++, error);
    logger << "Crails::Smtp::Server::connect: connected to endpoint iterator" << Logger::endl;
  }
  if (error)
    throw boost_ext::runtime_error("Cannot connect to " + hostname);
  smtp_handshake(callback);
}

void Smtp::Server::connect(const std::string& hostname, unsigned short port, const std::string& username, const std::string& password, AuthenticationProtocol protocol, std::function<void()> callback)
{
  this->username = username;
  this->connect(hostname, port, [this, username, password, protocol, callback]()
  {
    (this->*auth_methods[protocol])(username, password, callback);
  });
}

void Smtp::Server::disconnect(void)
{
  auto self = shared_from_this();

  smtp_quit([this, self]()
  {
    sock.close();
  });
}

void Smtp::Server::send(const Smtp::Mail& mail, std::function<void()> callback)
{
  auto self = shared_from_this();

  smtp_new_mail(mail, [this, self, callback, &mail]()
  {
    smtp_recipients(mail, [this, self, callback, &mail]()
    {
      smtp_body(mail, [this, self, callback]()
      {
        callback();
      });
    });
  });
}

void Smtp::Server::report_error(const std::exception& error) const
{
  logger << Logger::Error << "Smtp::Server: error occured: " << error.what() << Logger::endl;
  if (on_error)
  {
    try
    {
      on_error(error);
    }
    catch (const std::exception& new_error)
    {
      logger << Logger::Error << "Smtp::Server: error occured in error handler: " << new_error.what() << Logger::endl;
    }
    catch (...)
    {
      logger << Logger::Error << "Smtp::Server: error occured in error handler: unknown" << Logger::endl;
    }
  }
}

Smtp::Server::AsyncHandler Smtp::Server::protect_handler(AsyncHandler handler) const
{
  return [this, handler](const boost::system::error_code& error, std::size_t bytes_received)
  {
    try
    {
      handler(error, bytes_received);
    }
    catch (const std::exception& error)
    {
      report_error(error);
    }
    catch (...)
    {
      report_error(std::runtime_error("unknown exception"));
    }
  };
}

/*
 * SMTP Protocol Implementation (RFC 5321)
 */
void Smtp::Server::smtp_read_answer(unsigned short expected_return, std::function<void(std::string)> callback)
{
  auto self = shared_from_this();
  auto buffer = std::make_shared<boost::array<char,256>>();
  auto handler = [this, self, expected_return, buffer, callback](const boost::system::error_code& error, std::size_t bytes_received)
  {
    std::string    answer;
    unsigned short return_value;

    if (bytes_received == 0 || error.value() != boost::system::errc::success)
    {
      logger << Logger::Error << "Crails::Smtp::Server::smtp_read_answer: error:" << error << ", bytes received: " << bytes_received << Logger::endl;
      throw boost_ext::runtime_error("SMTP server closed the connection");
    }
    std::copy(buffer->begin(), buffer->begin() + bytes_received, std::back_inserter(answer));
    logger << Logger::Debug << "Crails::Smtp::Server::smtp_read_answer: answer received:\n" << answer << Logger::endl;

    return_value = atoi(answer.substr(0, 3).c_str());
    if (return_value != expected_return)
    {
      std::stringstream error_message;

      error_message << "Expected answer status to be " << expected_return << ", received `" << answer << '`';
      throw boost_ext::runtime_error(error_message.str());
    }
    callback(answer);
  };

  logger << Logger::Debug << "Crails::Smtp::Server::smtp_read_answer: expecting " << expected_return << Logger::endl;
  if (tls_enabled)
    ssl_sock.async_read_some(boost::asio::buffer(*buffer), protect_handler(handler));
  else
    sock.async_receive(boost::asio::buffer(*buffer), protect_handler(handler));
}

void Smtp::Server::smtp_write_message(std::function<void()> callback)
{
  send_buffer = server_message.str();
  auto self    = shared_from_this();
  auto buffer  = boost::asio::buffer(send_buffer);
  auto handler = [this, self, callback](const boost::system::error_code& error, std::size_t)
  {
    logger << "Crails::Smtp::Server::smtp_write_message: message written: " << error << Logger::endl;
    server_message.str(std::string());
    callback();
  };

  logger << Logger::Debug << "Crails::Smtp::Server::smtp_write_message:\n" << send_buffer << Logger::endl;
  if (tls_enabled)
    ssl_sock.async_write_some(buffer, protect_handler(handler));
  else
    sock.async_write_some(buffer, protect_handler(handler));
}

void Smtp::Server::smtp_write_and_read(unsigned short expected_return, std::function<void(std::string)> callback)
{
  smtp_write_message([this, expected_return, callback]()
  {
    smtp_read_answer(expected_return, callback);
  });
}

void Smtp::Server::smtp_body(const Smtp::Mail& mail, std::function<void()> callback)
{
  // Notify that we're starting the DATA stream
  server_message << "DATA\r\n";
  smtp_write_and_read(354, [this, &mail, callback](std::string)
  {
    Mail::Sender sender = mail.get_sender();

    if (sender.address.length() == 0)
      sender.address = username;
    // Setting the headers
    smtp_data_addresses("To",       mail, 0);
    smtp_data_addresses("Cc",       mail, Smtp::Mail::CarbonCopy);
    smtp_data_addresses("Bcc",      mail, Smtp::Mail::CarbonCopy | Smtp::Mail::Blind);
    if (mail.get_reply_to() != "")
      smtp_data_address("Reply-To", mail.get_reply_to(), "");
    smtp_data_address  ("From",     sender.address, sender.name);
    // Send the content type if necessary
    if (mail.get_content_type() != "")
    {
      server_message << "MIME-Version: 1.0\r\n";
      server_message << "Content-Type: " << mail.get_content_type() << "\r\n";
    }
    // Send a message-id if possible
    if (mail.get_id() != "")
      server_message << "Message-Id: " << mail.get_id() << "\r\n";
    // Send the subject
    server_message << "Subject: " << mail.get_subject() << "\r\n";
    // Send the body and finish the DATA stream
    server_message << mail.body   << "\r\n.\r\n";
    smtp_write_and_read([this, callback](std::string)
    {
      callback();
    });
  });
}

void Smtp::Server::smtp_data_addresses(const std::string& field, const Smtp::Mail& mail, int flag)
{
  auto it  = mail.recipients.begin();
  auto end = mail.recipients.end();
  int  i   = 0;

  for (; it != end ; ++it)
  {
    if (it->type == flag)
    {
      if (i != 0)
        server_message << ", ";
      else
        server_message << field << ": ";
      if (it->name.size() > 0)
        server_message << it->name << " <" << it->address << ">";
      else
        server_message << it->address;
      server_message << "\r\n";
      ++i;
    }
  }
}

void Smtp::Server::smtp_data_address(const std::string& field, const std::string& address, const std::string& name)
{
  server_message << field << ": " << name << " <" << address << ">\r\n";
}

void Smtp::Server::smtp_recipients(const Smtp::Mail& mail, std::function<void()> callback)
{
  auto it  = mail.recipients.begin();
  auto end = mail.recipients.end();
  
  for (; it != end ; ++it)
  {
    server_message << "RCPT TO: <" << it->address << ">\r\n";
    smtp_write_and_read([this, callback](std::string)
    {
      callback();
    });
  }
}

void Smtp::Server::smtp_new_mail(const Smtp::Mail& mail, std::function<void()> callback)
{
  const Smtp::Mail::Sender& sender = mail.get_sender();

  server_message << "MAIL FROM: <" << sender.address << ">\r\n";
  smtp_write_and_read([this, callback](std::string)
  {
    callback();
  });
}

void Smtp::Server::smtp_handshake(std::function<void()> callback)
{
  logger << Logger::Debug << "Crails::Smtp::Server::smtp_handshake" << Logger::endl;
  // Receiving the server identification
  if (tls_enabled)
  {
    logger << "Crails::Smtp::Server::smtp_handshake with tls enabled" << Logger::endl;
    tls_enabled = false;
    smtp_read_answer(220, [this, callback](std::string message)
    {
      server_id = message;
      server_message << "HELO " << sock.remote_endpoint().address().to_string() << "\r\n";
      smtp_write_and_read(250, [this, callback](std::string)
      {
        server_message << "STARTTLS" << "\r\n";
        smtp_write_and_read(220, [this, callback](std::string)
        {
          boost::system::error_code error;
          ssl_sock.handshake(SslSocket::client, error);
          tls_enabled = true;
	  smtp_handshake_end(callback);
        });
      });
    });
  }
  else
  {
    smtp_read_answer(220, [this, callback](std::string message)
    {
      server_id = message;
      smtp_handshake_end(callback);
    });
  }
}

void Smtp::Server::smtp_handshake_end(std::function<void()> callback)
{
  server_message << "HELO " << sock.remote_endpoint().address().to_string() << "\r\n";
  smtp_write_and_read(250, [this, callback](std::string)
  {
    logger << Logger::Debug << "Crails::Smtp::Server::smtp_handshake over" << Logger::endl;
    callback();
  });
}

void Smtp::Server::smtp_quit(std::function<void()> callback)
{
  server_message << "QUIT\r\n";
  smtp_write_and_read(221, [this, callback](std::string)
  {
    callback();
  });
}

/* 
 * SMTP StartTLS (RFC 2487) (TLS protocol is defined in RFC2246)
 */
void Smtp::Server::start_tls(void)
{
  tls_enabled = true;
}

/*
 * SMTP Authentication Extension (RFC 2554)
 */
std::map<Smtp::Server::AuthenticationProtocol,Smtp::Server::SmtpAuthMethod> Smtp::Server::auth_methods = {
  { Smtp::Server::PLAIN,      &Smtp::Server::smtp_auth_plain      },
  { Smtp::Server::LOGIN,      &Smtp::Server::smtp_auth_login      },
  { Smtp::Server::MD5,        &Smtp::Server::smtp_auth_md5        },
  { Smtp::Server::DIGEST_MD5, &Smtp::Server::smtp_auth_digest_md5 },
  { Smtp::Server::CRAM_MD5,   &Smtp::Server::smtp_auth_cram_md5   }
};

void Smtp::Server::smtp_auth_plain(const std::string& user, const std::string& password, std::function<void()> callback)
{
  string auth_hash = base64_encode('\000' + user + '\000' + password);

  server_message << "AUTH PLAIN\r\n";
  smtp_write_and_read(334, [this, auth_hash, callback](std::string)
  {
    server_message << auth_hash << "\r\n";
    smtp_write_and_read(235, [this, callback](std::string)
    {
      callback();
    });
  });
}

void Smtp::Server::smtp_auth_login(const std::string& user, const std::string& password, std::function<void()> callback)
{
  string user_hash = base64_encode(user);
  string pswd_hash = base64_encode(password);

  server_message << "AUTH LOGIN\r\n";
  smtp_write_and_read(334, [this, user_hash, pswd_hash, callback](std::string)
  {
    server_message << user_hash;
    smtp_write_and_read(334, [this, pswd_hash, callback](std::string)
    {
      server_message << pswd_hash;
      smtp_write_and_read(235, [this, callback](std::string)
      {
        callback();
      });
    });
  });
}

void Smtp::Server::smtp_auth_md5(const std::string& user, const std::string& password, std::function<void()>)
{
  throw std::runtime_error("unsupported smtp_auth_md5");
}

void Smtp::Server::smtp_auth_digest_md5(const string& user, const string& password, std::function<void()>)
{
  throw std::runtime_error("unsupported smtp_auth_digest_md5");
}

void Smtp::Server::smtp_auth_cram_md5(const string& user, const string& password, std::function<void()>)
{
  throw std::runtime_error("unsupported smt_auth_cram_md5");
}

// END SMTP AUTHENTICATION EXTENSION
