#ifndef  BOOTS_SMTP_HPP
# define BOOTS_SMTP_HPP

# include <boost/asio.hpp>
# include <boost/asio/ssl.hpp>
# include <crails/utils/helpers.hpp>
# include <crails/render_target.hpp>
# include <map>
# include <sstream>

namespace Smtp
{
  class Server;

  class Mail : public Crails::RenderTarget
  {
    friend class Server;
  public:
    struct Recipient
    {
      Recipient() : type(0) {}

      bool          operator==(const std::string& address) const { return (this->address == address); }
      std::string   address;
      std::string   name;
      unsigned char type;
    };

    struct Sender
    {
      std::string address;
      std::string name;
    };

    typedef std::vector<Recipient> Recipients;

    enum RecipientType
    {
      CarbonCopy = 1,
      Blind      = 2
    };

    void        set_sender(const std::string& address, const std::string& name = "");
    void        add_recipient(const std::string& address, const std::string& name = "", unsigned char flags = 0);
    void        del_recipient(const std::string& address);
    void        set_header(const std::string& header, const std::string& value) override;
    void        set_body(const char* str, size_t size) override { body = std::string(str, size); }
    const_attr_accessor(Sender&,      sender)
    const_attr_accessor(std::string&, subject)
    const_attr_accessor(std::string&, reply_to)
    const_attr_accessor(std::string&, content_type)

    const std::string& get_body() const { return body; }

  private:
    Recipients  recipients;
    Sender      sender;
    std::string subject;
    std::string body;
    std::string reply_to;
    std::string content_type;
  };

  class Server
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
    void connect(const std::string& hostname, unsigned short port);
    void connect(const std::string& hostname, unsigned short port, const std::string& username, const std::string& password, AuthenticationProtocol auth = PLAIN);
    void disconnect();
    void send(const Mail& mail);
    const_attr_getter(std::string&, server_id)

  private:
    // SMTP Protocol Implementation
    std::string smtp_read_answer(unsigned short expected_return = 250);
    void        smtp_write_message();
    void        smtp_body(const Mail& mail);
    void        smtp_recipients(const Mail& mail);
    void        smtp_new_mail(const Mail& mail);
    void        smtp_handshake();
    void        smtp_quit();

    void        smtp_data_address(const std::string& field, const std::string& address, const std::string& name);
    void        smtp_data_addresses(const std::string& field, const Mail& mail, int flag);

    std::string                   server_id;
    std::stringstream             server_message;
    boost::asio::ssl::context     ssl_context;
    SslSocket                     ssl_sock;
    boost::asio::ip::tcp::socket& sock;
    bool                          tls_enabled;

    // Authentication Extension as defined in RFC 2554
    typedef void (Server::*SmtpAuthMethod)(const std::string& user, const std::string& password);
    void        smtp_auth_plain     (const std::string&, const std::string&);
    void        smtp_auth_login     (const std::string&, const std::string&);
    void        smtp_auth_digest_md5(const std::string&, const std::string&);
    void        smtp_auth_md5       (const std::string&, const std::string&);
    void        smtp_auth_cram_md5  (const std::string&, const std::string&);

    static std::map<AuthenticationProtocol,SmtpAuthMethod> auth_methods;
  };
}

#endif
