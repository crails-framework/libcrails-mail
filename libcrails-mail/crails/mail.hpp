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
    struct Identity
    {
      std::string address;
      std::string name;

      bool operator==(const std::string& address) const { return (this->address == address); }
      std::string to_string() const;
    };

    struct Recipient : public Identity
    {
      Recipient() : type(0) {}
      Recipient(const std::string& address, unsigned char type = 0) : type(type) { this->address = address; }
      Recipient(const std::string& address, const std::string& name, unsigned char type = 0) : type(type) { this->address = address; this->name = name; }

      unsigned char type;
    };

    typedef Identity Sender;
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
    void set_body(const char* str, size_t size) override;
    const_attr_accessor(std::string&, id)
    const_attr_accessor(Sender&,      sender)
    const_attr_accessor(std::string&, subject)
    const_attr_accessor(std::string&, reply_to)
    const_attr_accessor(std::string&, content_type)
    const_attr_accessor(std::string&, charset)
    const_attr_accessor(Recipients&,  recipients)

    const std::string& get_html() const { return html; }
    const std::string& get_text() const { return text; }

  private:
    std::string id;
    Recipients  recipients;
    Sender      sender;
    std::string subject;
    std::string html, text;
    std::string reply_to;
    std::string content_type;
    std::string charset = "utf-8";
  };
}

#endif
