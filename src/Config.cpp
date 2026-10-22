#include "Config.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "beatsaber-hook/shared/rapidjson/include/rapidjson/document.h"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/prettywriter.h"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/stringbuffer.h"
#include "logger.hpp"

namespace {
    constexpr const char* ConfigPath = "/sdcard/ModData/com.beatgames.beatsaber/Configs/HidePacks.json";
    std::unordered_set<std::string> hidden;
}

namespace Config {
    void Load() {
        hidden.clear();
        try {
            std::ifstream in(ConfigPath);
            if (!in.good()) {
                Logger.info("No config file yet");
                return;
            }
            std::stringstream buffer;
            buffer << in.rdbuf();
            rapidjson::Document doc;
            doc.Parse(buffer.str().c_str());
            if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("hidden") || !doc["hidden"].IsArray()) {
                Logger.warn("Config file is not valid, ignoring it");
                return;
            }
            for (auto& value : doc["hidden"].GetArray()) {
                if (value.IsString()) {
                    hidden.insert(value.GetString());
                }
            }
            Logger.info("Loaded {} hidden packs", hidden.size());
        } catch (std::exception const& e) {
            Logger.warn("Config load failed: {}", e.what());
            hidden.clear();
        }
    }

    void Save() {
        try {
            rapidjson::Document doc;
            doc.SetObject();
            rapidjson::Value list(rapidjson::kArrayType);
            for (auto const& id : hidden) {
                list.PushBack(rapidjson::Value(id.c_str(), doc.GetAllocator()), doc.GetAllocator());
            }
            doc.AddMember("hidden", list, doc.GetAllocator());
            rapidjson::StringBuffer sb;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
            doc.Accept(writer);
            std::filesystem::path target(ConfigPath);
            std::filesystem::create_directories(target.parent_path());
            std::filesystem::path temp = target;
            temp += ".tmp";
            {
                std::ofstream out(temp, std::ios::trunc);
                out << sb.GetString();
                out.flush();
                if (!out.good()) {
                    Logger.warn("Config save failed, keeping the previous file");
                    std::filesystem::remove(temp);
                    return;
                }
            }
            std::filesystem::rename(temp, target);
            Logger.info("Saved {} hidden packs", hidden.size());
        } catch (std::exception const& e) {
            Logger.warn("Config save failed: {}", e.what());
        }
    }

    bool IsHidden(std::string_view packId) {
        return hidden.contains(std::string(packId));
    }

    const std::unordered_set<std::string>& Hidden() {
        return hidden;
    }

    void SetHidden(std::unordered_set<std::string> value) {
        hidden = std::move(value);
    }
}
