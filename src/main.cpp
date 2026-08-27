#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <format>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <ranges>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;

struct InputConfig {
    fs::path directory;
    std::vector<std::string> extensions;
};

struct OutputConfig {
    fs::path directory;
    std::string base_name;
    int start_index{};
    std::string extension;
};

struct ProcessingConfig {
    double logo_diagonal_divisor{12.0};
    int margin_x{10};
    int margin_y{10};
    std::string gravity{"southeast"};
    unsigned int cpu_usage_percent{75};
};

struct ToolsConfig {
    std::string magick_command{"magick"};
};

struct AppConfig {
    fs::path logo_path;
    InputConfig input;
    OutputConfig output;
    ProcessingConfig processing;
    ToolsConfig tools;
};

struct ImageInfo {
    std::uintmax_t width{};
    std::uintmax_t height{};
};

std::string to_lower(std::string value) {
    std::ranges::transform(value, value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

void validate_absolute(const fs::path& path, const std::string& field_name) {
    if (!path.is_absolute()) {
        throw std::runtime_error(field_name + " must be an absolute path: " + path.string());
    }
}

std::string quote(const fs::path& value) {
    std::string result = "\"";
    for (const char c : value.string()) {
        if (c == '"') result += '\\';
        result += c;
    }
    result += '"';
    return result;
}

std::string quote(const std::string& value) {
    return quote(fs::path(value));
}

json read_json_file(const fs::path& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Unable to open configuration file: " + path.string());
    json j;
    in >> j;
    return j;
}

AppConfig load_config(const fs::path& config_path) {
    const auto j = read_json_file(config_path);
    AppConfig cfg;

    cfg.logo_path = j.at("logo_path").get<fs::path>();
    cfg.input.directory = j.at("input").at("directory").get<fs::path>();
    cfg.input.extensions = j.at("input").at("extensions").get<std::vector<std::string>>();
    cfg.output.directory = j.at("output").at("directory").get<fs::path>();
    cfg.output.base_name = j.at("output").at("base_name").get<std::string>();
    cfg.output.start_index = j.at("output").at("start_index").get<int>();
    cfg.output.extension = j.at("output").at("extension").get<std::string>();

    cfg.processing.logo_diagonal_divisor =
        j.at("processing").value("logo_diagonal_divisor", 12.0);
    cfg.processing.margin_x = j.at("processing").value("margin_x", 10);
    cfg.processing.margin_y = j.at("processing").value("margin_y", 10);
    cfg.processing.gravity =
        j.at("processing").value("gravity", std::string("southeast"));
    cfg.processing.cpu_usage_percent =
        j.at("processing").value("cpu_usage_percent", 75u);

    if (j.contains("tools")) {
        cfg.tools.magick_command =
            j.at("tools").value("magick_command", std::string("magick"));
    }

    validate_absolute(cfg.logo_path, "logo_path");
    validate_absolute(cfg.input.directory, "input.directory");
    validate_absolute(cfg.output.directory, "output.directory");

    if (cfg.input.extensions.empty()) {
        throw std::runtime_error("input.extensions cannot be empty");
    }

    for (auto& ext : cfg.input.extensions) {
        ext = to_lower(ext);
        if (ext.empty() || ext.front() != '.') {
            throw std::runtime_error("Each extension must start with '.', for example: .jpeg");
        }
    }

    if (cfg.output.extension.empty() || cfg.output.extension.front() != '.') {
        throw std::runtime_error("output.extension must start with '.', for example: .jpeg");
    }

    if (cfg.processing.logo_diagonal_divisor <= 0.0) {
        throw std::runtime_error("processing.logo_diagonal_divisor must be > 0");
    }

    if (cfg.processing.cpu_usage_percent == 0 || cfg.processing.cpu_usage_percent > 100) {
        throw std::runtime_error("processing.cpu_usage_percent must be between 1 and 100");
    }

    return cfg;
}

std::string run_command_capture(const std::string& command) {
#if defined(_WIN32)
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif
    if (!pipe) throw std::runtime_error("Unable to execute command: " + command);

    std::string result;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) result += buffer;

#if defined(_WIN32)
    const int rc = _pclose(pipe);
#else
    const int rc = pclose(pipe);
#endif
    if (rc != 0) throw std::runtime_error("Command failed: " + command);
    return result;
}

int run_command(const std::string& command) {
    return std::system(command.c_str());
}

ImageInfo get_image_info(const AppConfig& cfg, const fs::path& file) {
    const auto command = std::format(
        "{} identify -format \"%w %h\" {}",
        quote(cfg.tools.magick_command), quote(file));

    const auto output = run_command_capture(command);
    std::istringstream stream(output);
    ImageInfo info;
    stream >> info.width >> info.height;

    if (!stream || info.width == 0 || info.height == 0) {
        throw std::runtime_error("Invalid dimensions for file: " + file.string());
    }
    return info;
}

std::vector<fs::path> collect_files(const AppConfig& cfg) {
    std::vector<fs::path> files;
    const std::set<std::string> extensions(
        cfg.input.extensions.begin(), cfg.input.extensions.end());

    for (const auto& entry : fs::directory_iterator(cfg.input.directory)) {
        if (!entry.is_regular_file()) continue;
        if (extensions.contains(to_lower(entry.path().extension().string()))) {
            files.push_back(entry.path());
        }
    }

    std::ranges::sort(files);
    return files;
}

std::string gravity_to_magick(std::string gravity) {
    gravity = to_lower(std::move(gravity));
    if (gravity == "southeast") return "SouthEast";
    if (gravity == "southwest") return "SouthWest";
    if (gravity == "northeast") return "NorthEast";
    if (gravity == "northwest") return "NorthWest";
    if (gravity == "center") return "Center";
    if (gravity == "south") return "South";
    if (gravity == "north") return "North";
    if (gravity == "east") return "East";
    if (gravity == "west") return "West";
    return "SouthEast";
}

void process_one(const AppConfig& cfg,
                 int index,
                 const fs::path& input_file,
                 std::atomic<int>& done_count,
                 int total,
                 std::mutex& io_mutex) {
    const auto info = get_image_info(cfg, input_file);
    const auto diagonal = std::sqrt(
        static_cast<double>(info.width) * static_cast<double>(info.width) +
        static_cast<double>(info.height) * static_cast<double>(info.height));
    const auto logo_size = std::max(
        1, static_cast<int>(std::llround(diagonal / cfg.processing.logo_diagonal_divisor)));

    const auto output_name = std::format(
        "{}{:04d}{}", cfg.output.base_name, index, cfg.output.extension);
    const auto output_path = cfg.output.directory / output_name;

    const auto command = std::format(
        "{} -limit thread 1 {} "
        "\\( {} -resize {}x -alpha set "
        "-channel Alpha -evaluate multiply 0.30 +channel \\) "
        "-gravity {} -geometry +{}+{} -composite {}",
        quote(cfg.tools.magick_command),
        quote(input_file),
        quote(cfg.logo_path),
        logo_size,
        gravity_to_magick(cfg.processing.gravity),
        cfg.processing.margin_x,
        cfg.processing.margin_y,
        quote(output_path));

    if (run_command(command) != 0) {
        throw std::runtime_error("Processing failed for file: " + input_file.string());
    }

    const int processed = ++done_count;
    std::scoped_lock lock(io_mutex);
    const int percent = processed * 100 / std::max(total, 1);
    const int bars = processed * 40 / std::max(total, 1);
    std::cout << '\r' << '[' << std::string(bars, '#')
              << std::string(40 - bars, ' ')
              << "] " << percent << "% processed: " << processed
              << " left: " << (total - processed) << std::flush;
}

fs::path parse_config_path(int argc, char* argv[]) {
    if (argc == 2) return fs::path(argv[1]);
    if (argc == 3 && std::string_view(argv[1]) == "--config") return fs::path(argv[2]);
    throw std::runtime_error(
        "Usage: logo_processor /path/config.json or logo_processor --config /path/config.json");
}

int main(int argc, char* argv[]) {
    try {
        const auto config_path = parse_config_path(argc, argv);
        const auto cfg = load_config(config_path);

        if (!fs::exists(cfg.logo_path)) {
            throw std::runtime_error("Logo not found: " + cfg.logo_path.string());
        }
        if (!fs::exists(cfg.input.directory)) {
            throw std::runtime_error("Input directory not found: " + cfg.input.directory.string());
        }

        fs::create_directories(cfg.output.directory);
        const auto files = collect_files(cfg);
        const int total = static_cast<int>(files.size());

        if (total == 0) {
            std::cout << "No files found for the configured extensions.\n";
            return 0;
        }

        const auto hardware_threads = std::max(1u, std::thread::hardware_concurrency());
        const auto requested_jobs = std::max(
            1u, (hardware_threads * cfg.processing.cpu_usage_percent) / 100u);
        const auto jobs = std::min(requested_jobs, static_cast<unsigned int>(total));

        std::cout << "Found " << total
                  << " files. Hardware threads available: " << hardware_threads
                  << ", CPU usage target: " << cfg.processing.cpu_usage_percent << "%"
                  << ", jobs used: " << jobs << ".\n";

        std::atomic<int> done_count{0};
        std::mutex io_mutex;
        std::vector<std::future<void>> futures;
        futures.reserve(jobs);
        std::size_t cursor = 0;

        while (cursor < files.size() || !futures.empty()) {
            while (cursor < files.size() && futures.size() < jobs) {
                const int index = cfg.output.start_index + static_cast<int>(cursor);
                futures.emplace_back(std::async(
                    std::launch::async,
                    process_one,
                    std::cref(cfg),
                    index,
                    files[cursor],
                    std::ref(done_count),
                    total,
                    std::ref(io_mutex)));
                ++cursor;
            }

            for (auto it = futures.begin(); it != futures.end();) {
                if (it->wait_for(std::chrono::milliseconds(10)) ==
                    std::future_status::ready) {
                    it->get();
                    it = futures.erase(it);
                } else {
                    ++it;
                }
            }
        }

        std::cout << "\nCompleted. Processed " << total << " files.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}
