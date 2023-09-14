#include "mail_servers.hpp"
#include "smtp_mail_service.hpp"
#include <crails/logger.hpp>
#include <algorithm>
#include <map>

using namespace std;
using namespace Crails;

MailServers::MailServers(void)
{
}

shared_ptr<MailServiceInterface> MailServers::create(const Conf& settings) const
{
  shared_ptr<MailServiceInterface> service;

  switch (settings.backend)
  {
  case MailServers::SMTP:
    service = make_shared<SmtpMailService>(settings);
    break ;
  case MailServers::MailGun:
    break ;
  }
  return service;
}

shared_ptr<MailServiceInterface> MailServers::create(const string& conf_name) const
{
  auto iterator = servers.find(conf_name);

  if (iterator != servers.end())
    return create(iterator->second);
  else
    throw boost_ext::runtime_error("mail service '" + conf_name + "' not found.");
  return nullptr;
}

MailServers::Conf::Conf()
{
  backend = SMTP;
  hostname = "0.0.0.0";
  use_tls = false;
  use_authentication = false;
  port = 465;
}
