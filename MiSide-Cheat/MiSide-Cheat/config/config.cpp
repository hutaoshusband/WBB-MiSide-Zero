#include "config.h"
#include <fstream>
#include <shlobj.h>
#include <filesystem>

// Simple JSON-like or strict binary saver/loader is safest without deps
// But for simplicity/readability requests usually imply text.
// We'll implement a custom simple text parser: "Section.Key=Value"

namespace config {

    std::string GetConfigDirectory() {
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path))) {
            std::string dir = std::string(path) + "\\wbb_mita";
            CreateDirectoryA(dir.c_str(), NULL);
            dir += "\\configs";
            CreateDirectoryA(dir.c_str(), NULL);
            return dir;
        }
        return "";
    }

    void Reset() {
        g_config = GlobalConfig(); // Revert to default constructor values (all false now)
    }

    // Helper for key-value parsing
    void ParseLine(const std::string& line, std::string& key, std::string& value) {
        size_t eq = line.find('=');
        if (eq != std::string::npos) {
            key = line.substr(0, eq);
            value = line.substr(eq + 1);
        }
    }

    bool Save(const char* name) {
        std::string dir = GetConfigDirectory();
        if (dir.empty()) return false;
        
        std::string path = dir + "\\" + name + ".cfg";
        std::ofstream file(path);
        if (!file.is_open()) return false;

        // Menu
        file << "[Menu]\n";
        file << "toggle_key=" << g_config.menu.toggle_key << "\n";
        file << "animation_speed=" << g_config.menu.animation_speed << "\n";
        file << "dpi_scale_index=" << g_config.menu.dpi_scale_index << "\n";
        file << "accent_color=" << g_config.menu.accent_color[0] << "," << g_config.menu.accent_color[1] << "," << g_config.menu.accent_color[2] << "," << g_config.menu.accent_color[3] << "\n";
        file << "show_watermark=" << g_config.menu.show_watermark << "\n";
        file << "show_module_list=" << g_config.menu.show_module_list << "\n";

        // Visuals
        file << "[Visuals]\n";
        file << "esp.enabled=" << g_config.visuals.esp.enabled << "\n";
        file << "esp.key=" << g_config.visuals.esp.key << "\n";
        file << "esp.mode=" << g_config.visuals.esp.mode << "\n";
        file << "esp_box=" << g_config.visuals.esp_box << "\n";
        file << "esp_name=" << g_config.visuals.esp_name << "\n";
        
        file << "chams.enabled=" << g_config.visuals.chams.enabled << "\n";
        file << "chams.key=" << g_config.visuals.chams.key << "\n";
        file << "chams.mode=" << g_config.visuals.chams.mode << "\n";
        
        // Misc
        file << "[Misc]\n";
        file << "speed_hack.enabled=" << g_config.misc.speed_hack.enabled << "\n";
        file << "speed_hack.key=" << g_config.misc.speed_hack.key << "\n";
        file << "speed_hack.mode=" << g_config.misc.speed_hack.mode << "\n";
        file << "speed_multiplier=" << g_config.misc.speed_multiplier << "\n";
        
        file << "noclip.enabled=" << g_config.misc.no_clip.enabled << "\n";
        file << "noclip.key=" << g_config.misc.no_clip.key << "\n";
        file << "noclip.mode=" << g_config.misc.no_clip.mode << "\n";

        // Add more fields as needed...
        
        return true;
    }

    bool Load(const char* name) {
        std::string dir = GetConfigDirectory();
        if (dir.empty()) return false;
        
        std::string path = dir + "\\" + name + ".cfg";
        std::ifstream file(path);
        if (!file.is_open()) return false;

        // Reset config first?
        // Reset(); 

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '[') continue;
            
            std::string key, val;
            ParseLine(line, key, val);
            if (key.empty()) continue;

            try {
                // Menu
                if (key == "toggle_key") g_config.menu.toggle_key = std::stoi(val);
                else if (key == "animation_speed") g_config.menu.animation_speed = std::stof(val);
                else if (key == "dpi_scale_index") g_config.menu.dpi_scale_index = std::stoi(val);
                else if (key == "show_watermark") g_config.menu.show_watermark = std::stoi(val);
                else if (key == "show_module_list") g_config.menu.show_module_list = std::stoi(val);
                else if (key == "accent_color") {
                    size_t pos = val.find(',');
                    if (pos != std::string::npos) {
                        g_config.menu.accent_color[0] = std::stof(val.substr(0, pos));
                        size_t pos2 = val.find(',', pos + 1);
                        if (pos2 != std::string::npos) {
                            g_config.menu.accent_color[1] = std::stof(val.substr(pos + 1, pos2 - pos - 1));
                            size_t pos3 = val.find(',', pos2 + 1);
                            if (pos3 != std::string::npos) {
                                g_config.menu.accent_color[2] = std::stof(val.substr(pos2 + 1, pos3 - pos2 - 1));
                                g_config.menu.accent_color[3] = std::stof(val.substr(pos3 + 1));
                            }
                        }
                    }
                }
                // Visuals
                else if (key == "esp.enabled") g_config.visuals.esp.enabled = std::stoi(val);
                else if (key == "esp.key") g_config.visuals.esp.key = std::stoi(val);
                else if (key == "esp.mode") g_config.visuals.esp.mode = std::stoi(val);
                else if (key == "esp_box") g_config.visuals.esp_box = std::stoi(val);
                else if (key == "esp_name") g_config.visuals.esp_name = std::stoi(val);
                
                else if (key == "chams.enabled") g_config.visuals.chams.enabled = std::stoi(val);
                else if (key == "chams.key") g_config.visuals.chams.key = std::stoi(val);
                else if (key == "chams.mode") g_config.visuals.chams.mode = std::stoi(val);

                // Misc
                else if (key == "speed_hack.enabled") g_config.misc.speed_hack.enabled = std::stoi(val);
                else if (key == "speed_hack.key") g_config.misc.speed_hack.key = std::stoi(val);
                else if (key == "speed_hack.mode") g_config.misc.speed_hack.mode = std::stoi(val);
                else if (key == "speed_multiplier") g_config.misc.speed_multiplier = std::stof(val);
                
                else if (key == "noclip.enabled") g_config.misc.no_clip.enabled = std::stoi(val);
                else if (key == "noclip.key") g_config.misc.no_clip.key = std::stoi(val);
                else if (key == "noclip.mode") g_config.misc.no_clip.mode = std::stoi(val);
            } catch(...) {}
        }
        return true;
    }
}
