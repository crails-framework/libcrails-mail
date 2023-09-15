#include "mailgun_mail_service.hpp"
#include <crails/utils/base64.hpp>
#include <crails/logger.hpp>
#include <sstream>

using namespace std;
using namespace Crails;

static string authorization_header(const MailServers::Conf& settings)
{
  return "Basic " + base64_encode(settings.username + ':' + settings.password);
}

static string format_recipients(const vector<Mail::Recipient>& recipients)
{
  stringstream stream;

  for (auto it = recipients.begin() ; it != recipients.end() ; ++it)
  {
    if (it != recipients.begin())
      stream << ", ";
    stream << it->to_string();
  }
  return stream.str();
}

static string mail_to_body(const Mail& mail)
{
  stringstream body;

  body << "from="     << Url::encode(mail.get_sender().to_string())
       << "&to="      << Url::encode(format_recipients(mail.get_recipients()))
       << "&subject=" << Url::encode(mail.get_subject());
  if (mail.get_content_type().find("html") > 0)
    body << "&html=" << Url::encode(mail.get_body());
  else
    body << "&text=" << Url::encode(mail.get_body());
  if (mail.get_reply_to().length())
    body << "&h:Reply-To="   << Url::encode(mail.get_reply_to());
  if (mail.get_id().length())
    body << "&h:Message-Id=" << Url::encode(mail.get_id());
  return body.str();
}

MailgunService::MailgunService(const MailServers::Conf& settings, const string& domain) :
  settings(settings), domain(domain), http(this->domain)
{
}

MailgunService::~MailgunService()
{
  try { http.disconnect(); }
  catch (boost::system::system_error) { }
}

void MailgunService::connect(function<void()> callback)
{
  http.connect();
  callback();
}

void MailgunService::send(const Mail& mail, function<void()> callback)
{
  Client::Request request{
    HttpVerb::post,
    uri("messages"),
    11
  };

  request.body() = mail_to_body(mail);
  request.set(HttpHeader::authorization, authorization_header(settings)); 
  request.set(HttpHeader::host, domain);
  request.set(HttpHeader::content_type, "application/x-www-form-urlencoded");
  request.set(HttpHeader::content_length, to_string(request.body().length()));
  http.async_query(request, [this, callback](const Client::Response& response, boost::beast::error_code error)
  {
    if (error)
      error_occured(error);
    else if (response.result() != HttpStatus::ok)
      received_error_status(response.result());
    else
      callback();
  });
}

void MailgunService::error_occured(boost::beast::error_code error)
{
  logger << Logger::Error << "MailgunService: error occured: " << error << Logger::endl;
  if (error_callback)
    error_callback(std::runtime_error("failed to send an email"));
}

void MailgunService::received_error_status(HttpStatus status)
{
  logger << Logger::Error << "MailgunService: responsded with error status "
         << static_cast<int>(status) << Logger::endl;
  if (error_callback)
    error_callback(std::runtime_error("failed to send an email"));
}
