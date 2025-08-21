#pragma once

#include <string>
#include <functional>
#include <vector>
#include <utility>
#include <concepts>
#include "nlohmann/json.hpp"

#include "./messages.hh"

namespace ecsact_lsp {

template<typename Send>
concept send_interface = requires(Send& send) {
	{
		send.send_request(
			std::declval<std::string>(),
			std::declval<nlohmann::json>(),
			std::declval<std::function<void(nlohmann::json)>>()
		)
	};

	{
		send.send_notification(
			std::declval<std::string>(),
			std::declval<nlohmann::json>()
		)
	};

	{
		send.show_message(
			std::declval<message_type>(),
			std::declval<std::string>(),
			std::declval<std::vector<message_action_item>>(),
			std::declval<std::function<void(std::optional<message_action_item>)>>()
		)
	};

	{
		send.show_message(std::declval<message_type>(), std::declval<std::string>())
	};

	{
		send.log_message(std::declval<message_type>(), std::declval<std::string>())
	};
};
} // namespace ecsact_lsp
