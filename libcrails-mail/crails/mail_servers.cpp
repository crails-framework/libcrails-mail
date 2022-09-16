#include "mail_servers.hpp"
#include <algorithm>
#include <map>

using namespace std;
using namespace Crails;

MailServers::MailServers(void)
{
}

void MailServers::configure_mail_server(const string& conf_name, Smtp::Server& server) const
{
  auto iterator = servers.find(conf_name);

  if (iterator != servers.end())
    iterator->second.connect_server(server);
}

MailServers::Conf::Conf()
{
  hostname = "0.0.0.0";
  use_tls = false;
  use_authentication = false;
  port = 465;
}

void MailServers::Conf::connect_server(Smtp::Server& server) const
{
  if (use_tls)
    server.start_tls();
  if (use_authentication)
    server.connect(hostname, port, username, password, authentication_protocol);
  else
    server.connect(hostname, port);
}
