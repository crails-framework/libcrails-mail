#ifndef  BOOTS_SMTP_HPP
# define BOOTS_SMTP_HPP

# include <utility>
# include <boost/asio.hpp>
# include <boost/asio/ssl.hpp>
# include "mail.hpp"
# include <map>

namespace Smtp
{
  class Server : public std::enable_shared_from_this<Server>
  {
  public:
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SslSocket;

    enum AuthenticationProtocol
    {
      PLAIN,
      LOGIN,
      DIGEST_MD5,
      MD5,
      CRAM_MD5
    };

    Server();

    void start_tls();
    void connect(const std::string& hostname, unsigned short port, std::function<void()> callback);
    void connect(const std::string& hostname, unsigned short port, const std::string& username, const std::string& password, AuthenticationProtocol auth, std::function<void()> callback);
    void disconnect();
    void send(const Mail& mail, std::function<void()> callback);
    const_attr_getter(std::string&, server_id)

  private:
    // SMTP Protocol Implementation
    void smtp_read_answer(std::function<void(std::string)> callback) { smtp_read_answer(250, callback); }
    void smtp_read_answer(unsigned short expected_return, std::function<void(std::string)> callback);
    void smtp_write_message(std::function<void()> callback);
    void smtp_write_and_read(std::function<void(std::string)> callback) { smtp_write_and_read(250, callback); }
    void smtp_write_and_read(unsigned short expected_return, std::function<void(std::string)> callback);
    void smtp_body(const Mail& mail, std::function<void()>);
    void smtp_recipients(const Mail& mail, std::function<void()>);
    void smtp_new_mail(const Mail& mail, std::function<void()>);
    void smtp_handshake(std::function<void()>);
    void smtp_handshake_end(std::function<void()> callback);
    void smtp_quit(std::function<void()>);

    void smtp_data_address(const std::string& field, const std::string& address, const std::string& name);
    void smtp_data_addresses(const std::string& field, const Mail& mail, int flag);

    std::string                   server_id;
    std::stringstream             server_message;
    boost::asio::ssl::context     ssl_context;
    SslSocket                     ssl_sock;
    boost::asio::ip::tcp::socket& sock;
    bool                          tls_enabled;
    std::string                   send_buffer;

    // Authentication Extension as defined in RFC 2554
    typedef void (Server::*SmtpAuthMethod)(const std::string& user, const std::string& password, std::function<void()>);
    void        smtp_auth_plain     (const std::string&, const std::string&, std::function<void()>);
    void        smtp_auth_login     (const std::string&, const std::string&, std::function<void()>);
    void        smtp_auth_digest_md5(const std::string&, const std::string&, std::function<void()>);
    void        smtp_auth_md5       (const std::string&, const std::string&, std::function<void()>);
    void        smtp_auth_cram_md5  (const std::string&, const std::string&, std::function<void()>);

    static std::map<AuthenticationProtocol,SmtpAuthMethod> auth_methods;
  };
}

#endif
