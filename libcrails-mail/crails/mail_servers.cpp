#include "mail_servers.hpp"
#include <crails/logger.hpp>
#include <algorithm>
#include <map>

using namespace std;
using namespace Crails;

MailServers::MailServers(void)
{
}

void MailServers::configure_mail_server(const string& conf_name, Smtp::Server& server, std::function<void()> callback) const
{
  auto iterator = servers.find(conf_name);

  if (iterator != servers.end())
    iterator->second.connect_server(server, callback);
  else
    logger << Logger::Error << "Crails::MailServers: configuration '" << conf_name << "' not found." << Logger::endl;
}

MailServers::Conf::Conf()
{
  hostname = "0.0.0.0";
  use_tls = false;
  use_authentication = false;
  port = 465;
}

void MailServers::Conf::connect_server(Smtp::Server& server, std::function<void()> callback) const
{
  if (use_tls)
    server.start_tls();
  if (use_authentication)
    server.connect(hostname, port, username, password, authentication_protocol, callback);
  else
    server.connect(hostname, port, callback);
}
