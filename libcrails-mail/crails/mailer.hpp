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

    Mail& get_mail() { return mail; }
    void  render(Controller::RenderType type, const std::string& value);
    void  render(const std::string& view, SharedVars = {});
    void  send(std::function<void()> = std::function<void()>());

  protected:
    std::shared_ptr<Controller> controller;
    std::shared_ptr<MailServiceInterface> service;
    Mail       mail;
    SharedVars vars;
    DataTree   params, response;

    virtual void on_sent();
    virtual void on_error_occured(const std::exception&);

  private:
    void create_server();
    void render_content_type(const std::string& view, const std::string& type, SharedVars);

    std::string configuration;
    bool        is_connected;
  };
}

#endif
