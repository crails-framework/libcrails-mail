#ifndef  CRAILS_MAIL_SERVERS_HPP
# define CRAILS_MAIL_SERVERS_HPP

# include <crails/utils/singleton.hpp>
# include <map>
# include "smtp.hpp"

namespace Crails
{
  class MailServer;

  class MailServers
  {
    SINGLETON(MailServers)
  public:
    class Conf
    {
      friend class MailServer;
    public:
      Conf();

      void connect_server(Smtp::Server& server, std::function<void()> callback) const;

    private:
      std::string                          hostname;
      unsigned short                       port;
      bool                                 use_authentication, use_tls;
      Smtp::Server::AuthenticationProtocol authentication_protocol;
      std::string                          username;
      std::string                          password;
    };
    typedef std::map<std::string, Conf> List;

    void configure_mail_server(const std::string& conf_name, Smtp::Server& server, std::function<void()> callback) const;

  private:
    MailServers(void);

    static const List servers;
  };

  class MailServer
  {
    MailServers::Conf conf;
  public:
    operator MailServers::Conf() const { return conf; }

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
