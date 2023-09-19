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

void Mailer::render(Controller::RenderType type, const string& value)
{
  switch (type)
  {
  case Controller::TEXT:
    mail.set_content_type("text/plain");
    break ;
  case Controller::HTML:
    mail.set_content_type("text/html");
    break ;
  default:
    throw boost_ext::invalid_argument("unsupported render type for mails");
  }
  mail.set_body(value.c_str(), value.length());
}

void Mailer::render(const string& view, SharedVars vars)
{
  Data accept_header = params["headers"]["Accept"];

  vars = merge(vars, this->vars);
  if (controller)
    vars = merge(vars, controller->vars);
  if (accept_header.exists())
    render_content_type(view, accept_header.as<string>(), vars);
  else
  {
    render_content_type(view, "text/html", vars);
    render_content_type(view, "text/plain", vars);
  }
}

void Mailer::render_content_type(const string& view, const string& type, SharedVars vars)
{
  params["headers"]["Accept"] = type;
  Renderer::render(view, params.as_data(), mail, vars);
}

void Mailer::send(std::function<void()> callback)
{
  auto self = shared_from_this();

  service->connect([this, self, callback]()
  {
    service->send(mail, [this, self, callback]()
    {
      if (callback)
        callback();
      on_sent();
    });
  });
}

void Mailer::on_sent()
{
}

void Mailer::on_error_occured(const std::exception& error)
{
}
