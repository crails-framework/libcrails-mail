#include "mailer.hpp"
#include <crails/renderer.hpp>
#include <crails/logger.hpp>

using namespace std;
using namespace Crails;

Mailer::Mailer(Controller& controller, const std::string& configuration) :
  controller(
    controller.shared_from_this(),
    reinterpret_cast<Controller*>(controller.shared_from_this().get())
  ),
  configuration(configuration),
  is_connected(false)
{
  smtp_server = std::make_shared<Smtp::Server>();
}

Mailer::Mailer(const std::string& configuration) : configuration(configuration), is_connected(false)
{
  smtp_server = std::make_shared<Smtp::Server>();
}

void Mailer::render(const std::string& view)
{
  SharedVars& vars = controller ? controller->vars : this->vars;

  if (!params["headers"]["Accept"].exists())
    params["headers"]["Accept"] = "text/html text/plain";
  Renderer::render(view, params.as_data(), mail, vars);
}

void Mailer::send(std::function<void()> callback)
{
  auto self = shared_from_this();
  auto send = [this, self, callback]()
  {
    smtp_server->send(mail, [self, callback]()
    {
      logger << Logger::Debug << "Crails::Mailer::send: mail sent" << Logger::endl;
      callback();
    });
  };

  if (!is_connected)
  {
    auto servers = MailServers::singleton::get();

    servers->configure_mail_server(configuration, *smtp_server, [this, send]()
    {
      is_connected = true;
      send();
    });
  }
  else
    send();
}
