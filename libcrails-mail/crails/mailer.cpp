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
  create_server();
}

Mailer::Mailer(const std::string& configuration) : configuration(configuration), is_connected(false)
{
  create_server();
}

void Mailer::create_server()
{
  service = MailServers::singleton::require().create(configuration);
  service->set_error_callback(std::bind(&Mailer::on_error_occured, this, std::placeholders::_1));
}

void Mailer::render(const std::string& view, SharedVars vars)
{
  vars = merge(vars, this->vars);
  if (controller)
    vars = merge(vars, controller->vars);
  if (!params["headers"]["Accept"].exists())
    params["headers"]["Accept"] = "text/html text/plain";
  Renderer::render(view, params.as_data(), mail, vars);
}

void Mailer::send(std::function<void()> callback)
{
  service->connect([this, callback]()
  {
    service->send(mail, [this, callback]() { callback(); });
  });
}

void Mailer::on_error_occured(const std::exception& error)
{
}
