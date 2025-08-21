#pragma once

#include <iostream>
#include <tuple>
#include <string>
#include <string_view>
#include "nlohmann/json.hpp"

namespace ecsact_lsp {

template<typename InputStream, typename OutputStream>
class jsonrpc_stream {
	static auto _parse_header(std::string_view header) {
		using namespace std::string_view_literals;
		constexpr auto header_name_end = ": "sv;
		const auto     colon_idx = header.find(header_name_end[0]);

		auto header_name = header.substr(0, colon_idx);
		auto header_value = header.substr(colon_idx + header_name_end.size());

		return std::make_tuple(header_name, header_value);
	}

public:
	InputStream&&  input_stream;
	OutputStream&& output_stream;

	auto send(const nlohmann::json& msg) {
		auto msg_str = msg.dump();
		output_stream //
			<< "Content-Length: " << msg_str.size() << "\r\n"
			<< "Content-Type: application/vscode-jsonrpc; charset=utf-8\r\n"
			<< "\r\n"
			<< msg_str;
		output_stream.flush();
	}

	auto receive() -> std::optional<nlohmann::json> {
		using namespace std::string_literals;

		int  content_length = 0;
		auto content_type = "application/vscode-jsonrpc; charset=utf-8"s;

		std::string current_header;
		std::getline(input_stream, current_header, '\n');
		while(!current_header.empty() && current_header != "\r") {
			assert(current_header[current_header.size() - 1] == '\r');
			current_header = current_header.substr(0, current_header.size() - 1);
			auto [name, value] = _parse_header(current_header);

			if(name == "Content-Length") {
				auto result = std::from_chars(
					value.data(),
					value.data() + value.size(),
					content_length
				);
				if(result.ec == std::errc::invalid_argument) {
					std::cerr << "[ERROR] Invalid Content-Length (" << value << ")\n";
					return {};
				}
			} else if(name == "Content-Type") {
				content_type = value;
			} else {
				std::cerr << "[ERROR] Invalid header '" << current_header << "'\n";
				return {};
			}

			std::getline(input_stream, current_header, '\n');
		}

		std::string rpc_message_str;
		rpc_message_str.resize(content_length);
		input_stream.read(rpc_message_str.data(), rpc_message_str.size());

		return nlohmann::json::parse(rpc_message_str);
	}
};

template<typename InputStream, typename OutputStream>
jsonrpc_stream(InputStream&&, OutputStream&&)
	-> jsonrpc_stream<InputStream&&, OutputStream&&>;

} // namespace ecsact_lsp
