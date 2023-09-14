#ifndef  CRAILS_SMTP_MAIL_SERVICE_HPP
# define CRAILS_SMTP_MAIL_SERVICE_HPP

# include "mail_servers.hpp"
# include "smtp.hpp"

namespace Crails
{
  struct SmtpMailService : public MailServiceInterface
  {
  public:
    SmtpMailService(const MailServers::Conf& settings);
    ~SmtpMailService();

    void connect(std::function<void()> callback) override;
    void send(const Mail&, std::function<void()>) override;
    void send_batch(std::vector<Mail>, std::function<void()>) override;
    void set_error_callback(std::function<void(const std::exception&)>) override;

  private:
    const MailServers::Conf& settings;
    Smtp::Server             server;
    bool                     connected = false;
  };
}

#endif
