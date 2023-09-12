#include "mailer.hpp"
#include <crails/renderer.hpp>
#include <crails/logger.hpp>

using namespace std;
using namespace Crails;

Mailer::Mailer(Controller& controller, const std::string& configuration) : controller(&controller), configuration(configuration), is_connected(false)
{
}

Mailer::Mailer(const std::string& configuration) : controller(0), configuration(configuration), is_connected(false)
{
}

void Mailer::render(const std::string& view)
{
  SharedVars& vars = controller ? controller->vars : this->vars;

  if (!params["headers"]["Accept"].exists())
    params["headers"]["Accept"] = "text/html text/plain";
  Renderer::render(view, params.as_data(), mail, vars);
}

void Mailer::send(void)
{
  auto servers = MailServers::singleton::get();
  auto smtp_server = std::make_shared<Smtp::Server>();

  servers->configure_mail_server(configuration, *smtp_server);
  smtp_server->send(mail, []()
  {
    logger << Logger::Debug << "Crails::Mailer::send: mail sent" << Logger::endl;
  });
}
