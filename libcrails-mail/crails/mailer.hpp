#ifndef  CRAILS_MAILER_HPP
# define CRAILS_MAILER_HPP

# include <crails/controller.hpp>
# include <memory>
# include "mail_servers.hpp"

namespace Crails
{
  class Mailer : public std::enable_shared_from_this<Mailer>
  {
  public:
    Mailer(Controller& controller, const std::string& configuration);
    Mailer(const std::string& configuration);

    void render(const std::string& view);
    void send(std::function<void()>);
    void set_recipients(const std::vector<Smtp::Mail::Recipient>& recipients) { mail.set_recipients(recipients); }

  protected:
    std::shared_ptr<Smtp::Server> smtp_server;
    Smtp::Mail   mail;
    SharedVars   vars;
    DataTree     params, response;

    virtual void on_error_occured(const std::exception&);

  private:
    void create_server();

    std::shared_ptr<Controller> controller;
    std::string configuration;
    bool        is_connected;
  };
}

#endif
