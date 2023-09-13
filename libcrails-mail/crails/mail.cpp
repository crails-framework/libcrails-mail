#include "mail.hpp"
#include <algorithm>

using namespace std;
using namespace Crails;

void Smtp::Mail::set_header(const string& header, const string& value)
{
  if (header == "Content-Type")
    content_type = value;
}

void Smtp::Mail::set_sender(const std::string& address, const std::string& name)
{
  sender.address = address;
  sender.name    = name;
}

void Smtp::Mail::add_recipient(const std::string& address, const std::string& name, unsigned char flags)
{
  Recipient recipient;

  recipient.address = address;
  recipient.name    = name;
  recipient.type    = flags;
  recipients.push_back(recipient);
}

void Smtp::Mail::del_recipient(const std::string& address)
{
  auto it = std::find(recipients.begin(), recipients.end(), address);

  if (it != recipients.end())
    recipients.erase(it);
}
