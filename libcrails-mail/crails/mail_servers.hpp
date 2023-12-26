#ifndef  CRAILS_MAIL_SERVERS_HPP
# define CRAILS_MAIL_SERVERS_HPP

# include <crails/utils/singleton.hpp>
# include <map>
# include "smtp.hpp"

namespace Crails
{
  class MailServer;

  struct MailServiceInterface
  {
    virtual ~MailServiceInterface() {}
    virtual void connect(std::function<void()> callback) = 0;
    virtual void send(const Mail&, std::function<void()> callback) = 0;
    virtual void send_batch(std::vector<Mail>, std::function<void()> callback) = 0;
    virtual void set_error_callback(std::function<void(const std::exception&)>) = 0;
  };

  class MailServers
  {
    SINGLETON(MailServers)
  public:
    enum Backend
    {
      SMTP,
      MailGun,
      MailGunEurope
    };

    class Conf
    {
    public:
      Conf();

      Backend                              backend;
      std::string                          hostname;
      unsigned short                       port;
      bool                                 use_authentication, use_tls;
      Smtp::Server::AuthenticationProtocol authentication_protocol;
      std::string                          username;
      std::string                          password;
    };
    typedef std::map<std::string, Conf> List;

    std::shared_ptr<MailServiceInterface> create(const std::string& conf_name) const;
    std::shared_ptr<MailServiceInterface> create(const Conf&) const;

  protected:
    MailServers(void);

    List servers;
  };

  class MailServer
  {
    MailServers::Conf conf;
  public:
    operator MailServers::Conf() const { return conf; }

    MailServer& backend(MailServers::Backend value) { conf.backend = value; return *this; }
    MailServer& hostname(const std::string& value) { conf.hostname = value; return *this;}
    MailServer& port(unsigned short value) { conf.port = value; return *this; }
    MailServer& use_authentication(bool value) { conf.use_authentication = value; return *this; }
    MailServer& use_tls(bool value) { conf.use_tls = value; return *this; }
    MailServer& authentication_protocol(Smtp::Server::AuthenticationProtocol value) { conf.authentication_protocol = value; return *this; }
    MailServer& username(const std::string& value) { conf.username = value; return *this; }
    MailServer& password(const std::string& value) { conf.password = value; return *this; }
  };
}

#endif
