#ifdef unix
#undef unix
#endif

#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mesos/mesos.hpp>
#include <mesos/module/container_logger.hpp>
#include <mesos/slave/container_logger.hpp>
#include <mesos/slave/containerizer.hpp>
#include <process/dispatch.hpp>
#include <process/future.hpp>
#include <process/process.hpp>
#include <process/subprocess.hpp>
#include <stout/os/constants.hpp>
#include <stout/os/pipe.hpp>
#include <string>
#include <unistd.h>
#include <vector>
#include "../lib/fd.hpp"

using namespace std::literals;

using module_entry_type = mesos::modules::Module<mesos::slave::ContainerLogger>(*)(const char*, const char*, const char*, const char*, const char*, bool(*)(), mesos::slave::ContainerLogger* (*)(const mesos::Parameters&));

class ForwardLogger final : public mesos::slave::ContainerLogger {
public:
	ForwardLogger(std::string&& json_file) : m_json(json_file), m_subordinates({}) {}
	~ForwardLogger() noexcept {
		while (!m_subordinates.empty()) {
			::dlclose(m_subordinates.back());
			m_subordinates.pop_back();
		}
	}

	auto initialize() -> Try<Nothing> override { return Nothing(); }

#ifdef MESOS_1_6_API
	auto prepare(const mesos::ExecutorInfo& executorInfo, const std::string& sandboxDirectory, const Option<std::string>& user) -> process::Future<mesos::slave::ContainerIO> override {
		auto p = mesos::Parameters();
		// Parse json
		if (const auto j = JSON::parse(m_json); j.isSome()) {
			if (const auto libs = j->as<JSON::Object>().at<JSON::Array>("libraries"); libs.isSome()) {
				for (const auto& lib : libs.get().values) {
					const auto entry = lib.as<JSON::Object>();
					if (const auto file_ = entry.at<std::string>("file"); file_.isSome()) {
						const auto file = file_.get();
						if (!std::filesystem::exists(file)) {
							// Error 
							return process::Failure(file + " was not found."s);
						}
						void* handle = ::dlopen(file.c_str(), RTLD_NOW);
						if (const auto modules = entry.at<JSON::Array>("modules"); modules.isSome()) {
							for (const auto& module : modules.get().values) {
								const auto mod = module.as<JSON::Object>();
								if (const auto name = mod.at<std::string>("name"); name.isSome()) {
									void* entry = reinterpret_cast<void*>(::dlsym(handle, name.get().c_str()));
									if (const auto parameters = mod.at<JSON::Array>("parameters"); parameters.isSome()) {
										p.clear_parameter();
										for (const auto& parameter : parameters.get().values) {
											const auto param = parameter.as<JSON::Object>();
											if (const auto key = param.at<std::string>("key"); key.isSome()) {
												if (const auto value = param.at<std::string>("value"); value.isSome()) {
													auto* p_ = p.add_parameter();
													p_->set_key(key.get());
													p_->set_value(value.get());
												}
											}
										}
									}
								}
							}
						}
						m_subordinates.push_back(handle);
					}
				}
			}
		}

		mesos::modules::Module<mesos::slave::ContainerLogger>* mod;
		auto logger = mod->create(p);
		logger->initialize();
		auto process_io = logger->prepare(executorInfo, sandboxDirectory, user);
		m_subordinates.push_back(handle);
		// TODO: How to manage stdout/stderr FDs from multiple subordinate modules?
		p.Clear();
	}
#else
	auto prepare(const mesos::ContainerID& containerId, const mesos::slave::ContainerConfig& containerConfig) -> process::Future<mesos::slave::ContainerIO> override {
		return process::dispatch(m_process.get(), &ForwardLoggerProcess::prepare, containerId, containerConfig);
	}
#endif

private:
	std::string m_json;
	std::vector<void*> m_subordinates;
};

__attribute__((visibility("default")))
mesos::modules::Module<mesos::slave::ContainerLogger> org_personal_mesos_ForwardLogger(
	MESOS_MODULE_API_VERSION,
	MESOS_VERSION,
	"Personal",
	"undisputed.seraphim@gmail.com",
	"container logger module that forwards logs to multiple logger modules",
	[]() -> bool { return true; },
	[](const mesos::Parameters& params) -> mesos::slave::ContainerLogger* {
		for (auto& param : params.parameter()) {
			if (param.key() == "subordinate_loggers") {
				if (std::filesystem::exists(param.value())) {
					auto ifs = std::ifstream(param.value());
					auto sz = std::filesystem::file_size(param.value());
					auto file = std::string(sz, '\0');
					ifs.read(file.data(), sz);
					return new ForwardLogger(std::move(file));
				}
			}
		}
		throw new std::runtime_error("No subordinate loggers found. Please check module configuration.");
	});
