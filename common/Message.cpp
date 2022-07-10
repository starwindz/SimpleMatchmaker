#include "Message.h"
#include <iostream>
#include <sstream>

std::vector<std::string> split(const std::string& text, char delim) {
    std::string line;
    std::vector<std::string> vec;
    std::stringstream ss(text);
    while (std::getline(ss, line, delim)) {
        vec.push_back(line);
    }
    return vec;
}


const std::map< MessageType, std::string> Message::headers = std::map< MessageType, std::string>{
    {MessageType::Info,"INFO:" },
    {MessageType::Version, "VERSION:"},
    {MessageType::Login,"LOGIN:" },
    {MessageType::Create,"CREATE:" },
    {MessageType::Join,"JOIN:" },
    {MessageType::Leave,"LEAVE:" },
    {MessageType::Eject,"EJECT:" },
    {MessageType::Start,"START:" }
};

void Message::OnData(std::function<void(const std::string&)> callback) const
{
    callback(m_data);
}

Message Message::Make(MessageType type, std::string content)
{
    return Message(type, headers.at(type) + content);
}

 Message Message::Parse(const unsigned char* data, size_t len)
{
    std::string input(data, data + len);
    auto it = std::find_if(headers.begin(), headers.end(), [&](const auto& header) {
        auto s = header.second;
        return input.compare(0, s.size(), s) == 0;
        });
    
    if (it == headers.end())
        throw BadMessageException();

     return Message(it->first, input);
   
}
MessageType Message::Type() const {
    return m_type;
}
const char* Message::Content() const
{
    return m_data.c_str() + headers.at(m_type).length();
}

bool Message::TryParseIPAddress(uint32_t& addr, uint16_t& port) const
{
    if (m_type != MessageType::Start)
        return false;

    auto strings = split(Content(), ':');

    if (strings.size() != 2)
        return false;

    try
    {
        addr = std::stoul(strings[0]);
        port = std::stoi(strings[1]);
        return true;
    }
    catch(...)
    {
        return false;
    }

}

void Message::ToConsole() const
{
    std::cout << m_data << "\n";
}