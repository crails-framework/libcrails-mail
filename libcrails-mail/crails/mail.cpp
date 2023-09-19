#include "mail.hpp"
#include <algorithm>

using namespace std;
using namespace Crails;

string Mail::Identity::to_string() const
{
  if (name.length() > 0)
    return name + " <" + address + '>';
  return address;
}

void Mail::set_header(const string& header, const string& value)
{
  if (header == "Content-Type")
    content_type = value;
}

void Mail::set_body(const char* str, size_t size)
{
  if (content_type == "text/html")
    html = std::string(str, size);
  else
    text = std::string(str, size);
}

void Mail::set_sender(const std::string& address, const std::string& name)
{
  sender.address = address;
  sender.name    = name;
}

void Mail::add_recipient(const std::string& address, const std::string& name, unsigned char flags)
{
  Recipient recipient;

  recipient.address = address;
  recipient.name    = name;
  recipient.type    = flags;
  recipients.push_back(recipient);
}

void Mail::del_recipient(const std::string& address)
{
  auto it = std::find(recipients.begin(), recipients.end(), address);

  if (it != recipients.end())
    recipients.erase(it);
}
