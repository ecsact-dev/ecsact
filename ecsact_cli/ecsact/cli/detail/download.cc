#include "ecsact/cli/detail/download.hh"

#include <span>
#include <cstddef>
#include <vector>
#include <string>
#include <boost/url.hpp>
#include <curl/curl.h>
#undef fopen // curl overrides fopen
#include "magic_enum.hpp"

#include "ecsact/cli/detail/proc_exec.hh"

using namespace std::string_literals;

using ecsact::cli::detail::download_file_buffer_t;
using ecsact::cli::detail::download_file_result;

static auto curl_error_message(CURLcode err) -> std::string {
	auto err_name = magic_enum::enum_name(err);
	return std::string{err_name};
}

static auto _download_file_write_callback(
	const void* buffer,
	size_t      size,
	size_t      count,
	void*       userdata
) -> size_t {
	auto& out_data = *static_cast<download_file_buffer_t*>(userdata);
	auto  buffer_span = std::span{
    static_cast<const std::byte*>(buffer),
    size * count,
  };

	out_data.insert(out_data.end(), buffer_span.begin(), buffer_span.end());
	return size * count;
}

[[maybe_unused]]
static auto download_file_with_libcurl( //
	boost::url url
) -> download_file_result {
	auto ret_out_data = download_file_buffer_t{};

	curl_global_init(CURL_GLOBAL_ALL);
	auto curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret_out_data);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _download_file_write_callback);

	auto res = curl_easy_perform(curl);

	curl_easy_cleanup(curl);
	curl_global_cleanup();

	if(res != CURLE_OK) {
		return std::logic_error{curl_error_message(static_cast<CURLcode>(res))};
	}

	return ret_out_data;
}

[[maybe_unused]]
static auto download_file_with_curl_cli( //
	boost::url url
) -> download_file_result {
	auto curl = ecsact::cli::detail::which("curl");

	if(!curl) {
		return std::logic_error{"Cannot find 'curl' in your PATH"};
	}

	auto file_bytes = ecsact::cli::detail::spawn_get_stdout_bytes(
		*curl,
		std::vector{
			std::string{url.c_str()},
			"--silent"s,
		}
	);

	if(!file_bytes) {
		return std::logic_error{"Failed to download file with curl cli"};
	}

	return *file_bytes;
}

auto ecsact::cli::detail::download_file( //
	std::string_view url_str
) -> download_file_result {
	auto url = boost::url{url_str};
// NOTE: temporary until curl in the BCR supports SSL
// SEE: https://github.com/bazelbuild/bazel-central-registry/pull/1666
#if defined(_WIN32) && !defined(ECSACT_CLI_USE_CURL_CLI)
	return download_file_with_libcurl(url);
#else
	return download_file_with_curl_cli(url);
#endif
}
