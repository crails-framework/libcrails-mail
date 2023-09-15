#ifndef  MAILGUN_MAIL_SERVICE_HPP
# define MAILGUN_MAIL_SERVICE_HPP

# include "mail_servers.hpp"
# include <crails/client.hpp>
# include <crails/url.hpp>

namespace Crails
{
  class MailgunService : public MailServiceInterface
  {
  public:
    MailgunService(const MailServers::Conf& ssettings, const std::string& domain);
    ~MailgunService();

    void connect(std::function<void()> callback) override;
    void send(const Mail& mail, std::function<void()> callback) override;
    void send_batch(std::vector<Mail>, std::function<void()>) override {}

    void set_error_callback(std::function<void(const std::exception&)> value) override
    {
      error_callback = value;
    }

  private:
    template<typename... PARTS>
    std::string uri(PARTS... parts) const
    {
      return "/v3/" + settings.hostname + Crails::uri(parts...);
    }

    void error_occured(boost::beast::error_code error);
    void received_error_status(HttpStatus);

    const MailServers::Conf& settings;
    std::string domain;
    Ssl::Client http;
    std::function<void(const std::exception&)> error_callback;
  };
}

#endif
