#ifndef  BOOTS_MAIL_HPP
# define BOOTS_MAIL_HPP

# include <crails/utils/helpers.hpp>
# include <crails/render_target.hpp>
# include <sstream>
# include <vector>

namespace Smtp
{
  class Server;
}

namespace Crails
{
  class Mail : public Crails::RenderTarget
  {
    friend class Server;
  public:
    struct Recipient
    {
      Recipient() : type(0) {}
      Recipient(const std::string& address, unsigned char type = 0) : address(address), type(type) {}
      Recipient(const std::string& address, const std::string& name, unsigned char type = 0) : address(address), name(name), type(type) {}

      bool          operator==(const std::string& address) const { return (this->address == address); }
      std::string   address;
      std::string   name;
      unsigned char type;
    };

    struct Sender
    {
      std::string address;
      std::string name;
    };

    typedef std::vector<Recipient> Recipients;

    enum RecipientType
    {
      CarbonCopy = 1,
      Blind      = 2
    };

    void set_sender(const std::string& address, const std::string& name = "");
    void add_recipient(const std::string& address, const std::string& name = "", unsigned char flags = 0);
    void del_recipient(const std::string& address);
    void set_header(const std::string& header, const std::string& value) override;
    void set_body(const char* str, size_t size) override { body = std::string(str, size); }
    const_attr_accessor(std::string&, id)
    const_attr_accessor(Sender&,      sender)
    const_attr_accessor(std::string&, subject)
    const_attr_accessor(std::string&, reply_to)
    const_attr_accessor(std::string&, content_type)
    const_attr_accessor(Recipients&,  recipients);

    const std::string& get_body() const { return body; }

  private:
    std::string id;
    Recipients  recipients;
    Sender      sender;
    std::string subject;
    std::string body;
    std::string reply_to;
    std::string content_type;
  };
}

#endif
