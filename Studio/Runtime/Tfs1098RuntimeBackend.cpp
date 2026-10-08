#include "Tfs1098RuntimeBackend.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>
#include <system_error>
#include <thread>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fantasy::studio::runtime {
namespace {

constexpr std::size_t kMaxLogChunk = 64U * 1024U;

bool copyFileChecked(
    const std::filesystem::path& source,
    const std::filesystem::path& destination,
    std::string& error) {

    std::error_code ec;
    std::filesystem::create_directories(destination.parent_path(), ec);
    if (ec) {
        error = "unable to create directory '" + destination.parent_path().string() + "': " + ec.message();
        return false;
    }

    std::filesystem::copy_file(
        source,
        destination,
        std::filesystem::copy_options::overwrite_existing,
        ec);
    if (ec) {
        error = "unable to copy '" + source.string() + "' to '" + destination.string() + "': " + ec.message();
        return false;
    }
    return true;
}

bool copyTree(
    const std::filesystem::path& source,
    const std::filesystem::path& destination,
    std::string& error) {

    std::error_code ec;
    if (!std::filesystem::is_directory(source, ec) || ec) {
        error = "runtime template directory is missing or invalid: " + source.string();
        return false;
    }

    std::filesystem::create_directories(destination, ec);
    if (ec) {
        error = "unable to create runtime output directory '" + destination.string() + "': " + ec.message();
        return false;
    }

    for (std::filesystem::recursive_directory_iterator it(source, ec), end; it != end; it.increment(ec)) {
        if (ec) {
            error = "unable to enumerate runtime template: " + ec.message();
            return false;
        }

        const auto relative = std::filesystem::relative(it->path(), source, ec);
        if (ec) {
            error = "unable to resolve runtime template path: " + ec.message();
            return false;
        }
        const auto target = destination / relative;

        if (it->is_directory(ec)) {
            std::filesystem::create_directories(target, ec);
        } else if (it->is_regular_file(ec)) {
            std::filesystem::create_directories(target.parent_path(), ec);
            if (!ec) {
                std::filesystem::copy_file(
                    it->path(),
                    target,
                    std::filesystem::copy_options::overwrite_existing,
                    ec);
            }
        } else {
            continue;
        }

        if (ec) {
            error = "unable to copy runtime template entry '" + it->path().string() + "': " + ec.message();
            return false;
        }
    }
    return true;
}

std::string trimLeft(std::string value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    value.erase(value.begin(), first);
    return value;
}

#ifdef _WIN32
std::wstring quoteWindowsArgument(const std::wstring& argument) {
    if (argument.find_first_of(L" \t\"") == std::wstring::npos) return argument;

    std::wstring output = L"\"";
    std::size_t backslashes = 0;
    for (const wchar_t ch : argument) {
        if (ch == L'\\') {
            ++backslashes;
            continue;
        }
        if (ch == L'\"') {
            output.append(backslashes * 2U + 1U, L'\\');
            output.push_back(L'\"');
            backslashes = 0;
            continue;
        }
        output.append(backslashes, L'\\');
        backslashes = 0;
        output.push_back(ch);
    }
    output.append(backslashes * 2U, L'\\');
    output.push_back(L'\"');
    return output;
}
#endif

} // namespace

struct Tfs1098RuntimeBackend::ProcessState {
#ifdef _WIN32
    HANDLE handle = nullptr;
#else
    pid_t pid = -1;
#endif
};

Tfs1098RuntimeBackend::Tfs1098RuntimeBackend()
    : process_(std::make_unique<ProcessState>()) {}

Tfs1098RuntimeBackend::~Tfs1098RuntimeBackend() {
#ifdef _WIN32
    if (process_ && process_->handle != nullptr) {
        CloseHandle(process_->handle);
        process_->handle = nullptr;
    }
#endif
}

RuntimeCapabilities Tfs1098RuntimeBackend::capabilities() const noexcept {
    return RuntimeCapabilities{true, true, true, true};
}

bool Tfs1098RuntimeBackend::validMapName(const std::string& value) noexcept {
    if (value.empty()) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isalnum(ch) != 0 || ch == '_' || ch == '-';
    });
}

bool Tfs1098RuntimeBackend::patchMapName(
    const std::filesystem::path& configPath,
    const std::string& mapName,
    std::string& error) {

    std::ifstream input(configPath);
    if (!input) {
        error = "unable to open runtime config: " + configPath.string();
        return false;
    }

    std::vector<std::string> lines;
    std::string line;
    bool replaced = false;
    while (std::getline(input, line)) {
        const std::string trimmed = trimLeft(line);
        if (!replaced && trimmed.rfind("mapName", 0) == 0 && trimmed.find('=') != std::string::npos) {
            lines.emplace_back("mapName = \"" + mapName + "\"");
            replaced = true;
        } else {
            lines.push_back(line);
        }
    }
    if (!input.eof()) {
        error = "unable to read runtime config: " + configPath.string();
        return false;
    }
    if (!replaced) lines.emplace_back("mapName = \"" + mapName + "\"");

    std::ofstream output(configPath, std::ios::trunc);
    if (!output) {
        error = "unable to write runtime config: " + configPath.string();
        return false;
    }
    for (const auto& current : lines) output << current << '\n';
    if (!output) {
        error = "failed while writing runtime config: " + configPath.string();
        return false;
    }
    return true;
}

RuntimePackageReport Tfs1098RuntimeBackend::packageProject(const RuntimePackageRequest& request) {
    RuntimePackageReport report;

    if (request.runtimeTemplateDirectory.empty()) {
        report.errors.emplace_back("runtimeTemplateDirectory is required for TFS1098 packaging");
        return report;
    }
    if (request.outputDirectory.empty()) {
        report.errors.emplace_back("outputDirectory is required for TFS1098 packaging");
        return report;
    }
    if (request.exportedMapPath.empty() || request.itemsOtbPath.empty()) {
        report.errors.emplace_back("exportedMapPath and itemsOtbPath are required for TFS1098 packaging");
        return report;
    }
    if (!validMapName(request.mapName)) {
        report.errors.emplace_back("mapName must contain only letters, digits, '_' or '-'");
        return report;
    }

    std::error_code ec;
    const auto templatePath = std::filesystem::absolute(request.runtimeTemplateDirectory, ec).lexically_normal();
    if (ec) {
        report.errors.emplace_back("unable to resolve runtimeTemplateDirectory: " + ec.message());
        return report;
    }
    const auto outputPath = std::filesystem::absolute(request.outputDirectory, ec).lexically_normal();
    if (ec) {
        report.errors.emplace_back("unable to resolve outputDirectory: " + ec.message());
        return report;
    }
    if (templatePath == outputPath) {
        report.errors.emplace_back("runtime output directory must differ from the template directory");
        return report;
    }

    std::string error;
    if (!copyTree(templatePath, outputPath, error)) {
        report.errors.push_back(std::move(error));
        state_ = RuntimeState::Failed;
        return report;
    }

    const auto worldDirectory = outputPath / "data" / "world";
    const auto itemsDirectory = outputPath / "data" / "items";
    const auto mapDestination = worldDirectory / (request.mapName + ".otbm");
    const auto itemsDestination = itemsDirectory / "items.otb";

    if (!copyFileChecked(request.exportedMapPath, mapDestination, error) ||
        !copyFileChecked(request.itemsOtbPath, itemsDestination, error)) {
        report.errors.push_back(std::move(error));
        state_ = RuntimeState::Failed;
        return report;
    }
    report.generatedFiles.push_back(mapDestination);
    report.generatedFiles.push_back(itemsDestination);

    if (!request.houseXmlPath.empty()) {
        const auto destination = worldDirectory / request.houseXmlPath.filename();
        if (!copyFileChecked(request.houseXmlPath, destination, error)) {
            report.errors.push_back(std::move(error));
            state_ = RuntimeState::Failed;
            return report;
        }
        report.generatedFiles.push_back(destination);
    }
    if (!request.spawnXmlPath.empty()) {
        const auto destination = worldDirectory / request.spawnXmlPath.filename();
        if (!copyFileChecked(request.spawnXmlPath, destination, error)) {
            report.errors.push_back(std::move(error));
            state_ = RuntimeState::Failed;
            return report;
        }
        report.generatedFiles.push_back(destination);
    }

    auto configPath = outputPath / "config.lua";
    if (!std::filesystem::exists(configPath, ec)) {
        const auto distPath = outputPath / "config.lua.dist";
        if (!std::filesystem::exists(distPath, ec)) {
            report.errors.emplace_back("TFS runtime template must contain config.lua or config.lua.dist");
            state_ = RuntimeState::Failed;
            return report;
        }
        if (!copyFileChecked(distPath, configPath, error)) {
            report.errors.push_back(std::move(error));
            state_ = RuntimeState::Failed;
            return report;
        }
    }
    if (!patchMapName(configPath, request.mapName, error)) {
        report.errors.push_back(std::move(error));
        state_ = RuntimeState::Failed;
        return report;
    }
    report.generatedFiles.push_back(configPath);

    preparedDirectory_ = outputPath;
    logFile_ = outputPath / "fantasy-tfs1098.log";
    state_ = RuntimeState::Stopped;
    processId_ = 0;
    report.success = true;
    return report;
}

RuntimeLaunchReport Tfs1098RuntimeBackend::launch(const RuntimeLaunchRequest& request) {
    RuntimeLaunchReport report;
    const RuntimeStatus current = status();
    if (current.state == RuntimeState::Running || current.state == RuntimeState::Starting) {
        report.errors.emplace_back("TFS1098 runtime is already running");
        return report;
    }

    const auto runtimeDirectory = request.runtimeDirectory.empty()
        ? preparedDirectory_
        : request.runtimeDirectory;
    if (runtimeDirectory.empty()) {
        report.errors.emplace_back("runtimeDirectory is required; package the runtime first or provide it explicitly");
        return report;
    }

#ifdef _WIN32
    const auto defaultExecutable = runtimeDirectory / "tfs.exe";
#else
    const auto defaultExecutable = runtimeDirectory / "tfs";
#endif
    auto executable = request.executable.empty() ? defaultExecutable : request.executable;
    if (executable.is_relative()) executable = runtimeDirectory / executable;

    std::error_code ec;
    if (!std::filesystem::is_regular_file(executable, ec) || ec) {
        report.errors.emplace_back("TFS executable was not found: " + executable.string());
        state_ = RuntimeState::Failed;
        return report;
    }

    logFile_ = request.logFile.empty() ? (runtimeDirectory / "fantasy-tfs1098.log") : request.logFile;
    if (logFile_.is_relative()) logFile_ = runtimeDirectory / logFile_;
    std::filesystem::create_directories(logFile_.parent_path(), ec);
    if (ec) {
        report.errors.emplace_back("unable to create runtime log directory: " + ec.message());
        state_ = RuntimeState::Failed;
        return report;
    }

    state_ = RuntimeState::Starting;

#ifdef _WIN32
    HANDLE logHandle = CreateFileW(
        logFile_.wstring().c_str(),
        FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (logHandle == INVALID_HANDLE_VALUE) {
        report.errors.emplace_back("unable to open runtime log file");
        state_ = RuntimeState::Failed;
        return report;
    }
    SetFilePointer(logHandle, 0, nullptr, FILE_END);
    SetHandleInformation(logHandle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

    std::wstring commandLine = quoteWindowsArgument(executable.wstring());
    for (const auto& argument : request.arguments) {
        commandLine.push_back(L' ');
        commandLine += quoteWindowsArgument(std::filesystem::path(argument).wstring());
    }
    std::vector<wchar_t> mutableCommand(commandLine.begin(), commandLine.end());
    mutableCommand.push_back(L'\0');

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = logHandle;
    startup.hStdError = logHandle;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION processInfo{};

    const BOOL created = CreateProcessW(
        executable.wstring().c_str(),
        mutableCommand.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        runtimeDirectory.wstring().c_str(),
        &startup,
        &processInfo);
    CloseHandle(logHandle);

    if (!created) {
        report.errors.emplace_back("CreateProcessW failed for TFS1098 runtime");
        state_ = RuntimeState::Failed;
        return report;
    }

    CloseHandle(processInfo.hThread);
    if (process_->handle != nullptr) CloseHandle(process_->handle);
    process_->handle = processInfo.hProcess;
    processId_ = static_cast<std::uint64_t>(processInfo.dwProcessId);
#else
    const pid_t pid = fork();
    if (pid < 0) {
        report.errors.emplace_back("fork failed for TFS1098 runtime: " + std::string(std::strerror(errno)));
        state_ = RuntimeState::Failed;
        return report;
    }
    if (pid == 0) {
        if (chdir(runtimeDirectory.c_str()) != 0) _exit(126);
        const int logFd = open(logFile_.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (logFd < 0) _exit(126);
        (void)dup2(logFd, STDOUT_FILENO);
        (void)dup2(logFd, STDERR_FILENO);
        close(logFd);

        std::vector<std::string> storage;
        storage.reserve(request.arguments.size() + 1U);
        storage.push_back(executable.string());
        storage.insert(storage.end(), request.arguments.begin(), request.arguments.end());
        std::vector<char*> argv;
        argv.reserve(storage.size() + 1U);
        for (auto& value : storage) argv.push_back(value.data());
        argv.push_back(nullptr);
        execv(executable.c_str(), argv.data());
        _exit(127);
    }

    process_->pid = pid;
    processId_ = static_cast<std::uint64_t>(pid);
#endif

    preparedDirectory_ = runtimeDirectory;
    state_ = RuntimeState::Running;
    report.success = true;
    report.processId = processId_;
    return report;
}

RuntimeStopReport Tfs1098RuntimeBackend::stop() {
    RuntimeStopReport report;
    const RuntimeStatus current = status();
    if (current.state != RuntimeState::Running && current.state != RuntimeState::Starting) {
        state_ = RuntimeState::Stopped;
        processId_ = 0;
        report.success = true;
        return report;
    }

#ifdef _WIN32
    if (process_->handle == nullptr) {
        report.errors.emplace_back("runtime process handle is unavailable");
        state_ = RuntimeState::Failed;
        return report;
    }
    if (!TerminateProcess(process_->handle, 0)) {
        report.errors.emplace_back("TerminateProcess failed for TFS1098 runtime");
        state_ = RuntimeState::Failed;
        return report;
    }
    (void)WaitForSingleObject(process_->handle, 5000);
    CloseHandle(process_->handle);
    process_->handle = nullptr;
#else
    if (process_->pid <= 0) {
        report.errors.emplace_back("runtime process id is unavailable");
        state_ = RuntimeState::Failed;
        return report;
    }
    if (kill(process_->pid, SIGTERM) != 0 && errno != ESRCH) {
        report.errors.emplace_back("SIGTERM failed for TFS1098 runtime: " + std::string(std::strerror(errno)));
        state_ = RuntimeState::Failed;
        return report;
    }

    bool exited = false;
    for (int attempt = 0; attempt < 40; ++attempt) {
        int childStatus = 0;
        const pid_t result = waitpid(process_->pid, &childStatus, WNOHANG);
        if (result == process_->pid || (result < 0 && errno == ECHILD)) {
            exited = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    if (!exited) {
        (void)kill(process_->pid, SIGKILL);
        int childStatus = 0;
        (void)waitpid(process_->pid, &childStatus, 0);
        report.warnings.emplace_back("TFS1098 runtime required forced termination after SIGTERM timeout");
    }
    process_->pid = -1;
#endif

    state_ = RuntimeState::Stopped;
    processId_ = 0;
    report.success = true;
    return report;
}

RuntimeStatus Tfs1098RuntimeBackend::status() const {
    if ((state_ != RuntimeState::Running && state_ != RuntimeState::Starting) || processId_ == 0) {
        return RuntimeStatus{state_, processId_,
            state_ == RuntimeState::Stopped ? "TFS1098 runtime is stopped" :
            state_ == RuntimeState::Failed ? "TFS1098 runtime failed" :
            "TFS1098 runtime is not prepared"};
    }

#ifdef _WIN32
    if (process_->handle == nullptr) {
        state_ = RuntimeState::Failed;
        processId_ = 0;
        return RuntimeStatus{state_, 0, "TFS1098 process handle is unavailable"};
    }
    const DWORD result = WaitForSingleObject(process_->handle, 0);
    if (result == WAIT_TIMEOUT) {
        state_ = RuntimeState::Running;
        return RuntimeStatus{state_, processId_, "TFS1098 runtime is running"};
    }
    CloseHandle(process_->handle);
    process_->handle = nullptr;
#else
    if (process_->pid <= 0) {
        state_ = RuntimeState::Failed;
        processId_ = 0;
        return RuntimeStatus{state_, 0, "TFS1098 process id is unavailable"};
    }
    int childStatus = 0;
    const pid_t result = waitpid(process_->pid, &childStatus, WNOHANG);
    if (result == 0) {
        state_ = RuntimeState::Running;
        return RuntimeStatus{state_, processId_, "TFS1098 runtime is running"};
    }
    process_->pid = -1;
#endif

    state_ = RuntimeState::Stopped;
    processId_ = 0;
    return RuntimeStatus{state_, 0, "TFS1098 runtime process exited"};
}

RuntimeLogChunk Tfs1098RuntimeBackend::readLogs(std::uint64_t cursor) const {
    RuntimeLogChunk chunk;
    chunk.nextCursor = cursor;
    if (logFile_.empty()) return chunk;

    std::error_code ec;
    const auto size = std::filesystem::file_size(logFile_, ec);
    if (ec) {
        if (ec == std::errc::no_such_file_or_directory) return chunk;
        chunk.errors.emplace_back("unable to inspect runtime log: " + ec.message());
        return chunk;
    }

    if (cursor > size) {
        cursor = 0;
        chunk.nextCursor = 0;
    }
    const auto remaining = size - cursor;
    const auto count = static_cast<std::size_t>(std::min<std::uintmax_t>(remaining, kMaxLogChunk));
    if (count == 0U) return chunk;

    std::ifstream input(logFile_, std::ios::binary);
    if (!input) {
        chunk.errors.emplace_back("unable to open runtime log: " + logFile_.string());
        return chunk;
    }
    input.seekg(static_cast<std::streamoff>(cursor), std::ios::beg);
    std::string text(count, '\0');
    input.read(text.data(), static_cast<std::streamsize>(count));
    const auto read = static_cast<std::size_t>(input.gcount());
    text.resize(read);
    chunk.text = std::move(text);
    chunk.nextCursor = cursor + read;
    return chunk;
}

} // namespace fantasy::studio::runtime
