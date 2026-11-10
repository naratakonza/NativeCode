#pragma once

#include <string_view>

#include <rapidjson/document.h>

namespace morgan::fileio::storage {

inline bool SettingsDepthValid(const rapidjson::Value& value, unsigned depth = 0)
{
    if (depth > 24) return false;
    if (value.IsObject())
    {
        for (const auto& member : value.GetObject())
        {
            if (!SettingsDepthValid(member.value, depth + 1)) return false;
        }
    }
    else if (value.IsArray())
    {
        for (const auto& item : value.GetArray())
        {
            if (!SettingsDepthValid(item, depth + 1)) return false;
        }
    }
    return true;
}

inline bool ParseSampSettings(std::string_view bytes, rapidjson::Document& document)
{
    constexpr unsigned flags = rapidjson::kParseValidateEncodingFlag | rapidjson::kParseIterativeFlag;
    return !bytes.empty() && bytes.find('\0') == std::string_view::npos && !document.Parse<flags>(bytes.data(), bytes.size()).HasParseError() && document.IsObject() && SettingsDepthValid(document);
}

} // namespace morgan::fileio::storage
