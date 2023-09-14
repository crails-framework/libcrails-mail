#include "smtp_mail_service.hpp"

using namespace Crails;
    
SmtpMailService::SmtpMailService(const MailServers::Conf& settings) :
  settings(settings)
{
}

SmtpMailService::~SmtpMailService()
{
}

void SmtpMailService::connect(std::function<void()> callback)
{
  if (!connected)
  {
    auto on_connected = [this, callback]()
    {
      connected = true;
      callback();
    };

    if (settings.use_tls)
      server.start_tls();
    if (settings.use_authentication)
      server.connect(settings.hostname, settings.port, settings.username, settings.password, settings.authentication_protocol, on_connected);
    else
      server.connect(settings.hostname, settings.port, on_connected);
  }
  else
    callback();
}

void SmtpMailService::send(const Mail& mail, std::function<void()> callback)
{
  server.send(mail, callback);
}

void SmtpMailService::send_batch(std::vector<Mail> mails, std::function<void()> callback)
{
  if (mails.size())
  {
    Mail mail = mails[0];

    mails.erase(mails.begin());
    send(mail, std::bind(&SmtpMailService::send_batch, this, mails, callback));
  }
  else
    callback();
}

void SmtpMailService::set_error_callback(std::function<void(const std::exception&)> callback)
{
  server.set_error_callback(callback);
}
