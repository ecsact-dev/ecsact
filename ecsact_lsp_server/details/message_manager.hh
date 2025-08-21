#pragma once

#include <atomic>
#include <mutex>
#include <map>
#include <functional>
#include <unordered_map>
#include <iostream>
#include "nlohmann/json.hpp"

#include "./messages.hh"
#include "./interfaces.hh"

namespace ecsact_lsp {

template<typename Sender, typename Receiver>
class message_manager {
	using json = nlohmann::json;

	Sender&&         _sender;
	Receiver&&       _receiver;
	std::atomic_int  _last_request_id;
	std::atomic_bool _active_acceptor;

	std::mutex                               _req_callbacks_mutex;
	std::map<int, std::function<void(json)>> _req_callbacks;

	using listeners_t =
		std::unordered_multimap<std::string, std::function<void(const json&)>>;
	listeners_t _listeners;

	using request_handlers_t =
		std::unordered_map<std::string, std::function<json(json)>>;
	request_handlers_t _request_handlers;

	auto _pop_req_callback(int req_id) {
		auto lk = std::unique_lock(_req_callbacks_mutex);
		auto callback = std::move(_req_callbacks.at(req_id));
		_req_callbacks.erase(req_id);
		lk.unlock();
		return callback;
	}

	auto _send_error_response(
		json           req,
		lsp_error_code error_code,
		std::string    message
	) {
		auto response = "{}"_json;
		response["jsonrpc"] = "2.0";
		response["id"] = req["id"];
		response["error"] = {
			{"code", static_cast<int>(error_code)},
			{"message", message},
		};
		_sender.send(std::move(response));
	}

	auto _send_response(int req_id, json result) {
		auto response = "{}"_json;
		response["jsonrpc"] = "2.0";
		response["id"] = req_id;
		response["result"] = std::move(result);
		_sender.send(std::move(response));
	}

public:
	using request_callback = std::function<void(json)>;

	message_manager(Sender&& sender, Receiver&& receiver)
		: _sender(std::forward<Sender>(sender))
		, _receiver(std::forward<Receiver>(receiver)) {
	}

	auto is_active() -> bool {
		return !_sender.stopped() && !_receiver.stopped();
	}

	auto pump() {
		if(_active_acceptor.exchange(true)) {
			_active_acceptor.wait(true);
			return;
		}

		_receiver.accept_one([this](json msg) {
			_active_acceptor = false;
			_active_acceptor.notify_all();
			if(msg.contains("method")) {
				auto method = msg.at("method").get<std::string>();
				auto params = msg["params"];

				if(msg.contains("id")) {
					if(_request_handlers.contains(method)) {
						_send_response(
							msg.at("id").get<int>(),
							_request_handlers.at(method)(std::move(params))
						);
					} else {
						_send_error_response(
							std::move(msg),
							lsp_error_code::method_not_found,
							"Method '" + method + "' unimplemented"
						);
					}
				} else {
					auto [itr, last] = _listeners.equal_range(method);
					for(; itr != last; ++itr) {
						itr->second(params);
					}
				}
			} else if(msg.contains("id")) {
				if(msg.at("id").is_number_integer()) {
					auto req_id = msg.at("id").get<int>();
					auto callback = _pop_req_callback(req_id);
					callback(std::move(msg));
				} else {
					_send_error_response(
						std::move(msg),
						lsp_error_code::invalid_param,
						"Response request ID is wrong type"
					);
				}
			} else {
				_send_error_response(
					std::move(msg),
					lsp_error_code::invalid_request,
					"Unknown request ID"
				);
			}
		});
	}

	auto send_request(std::string method, json params, request_callback callback)
		-> request_id {
		auto req_id = ++_last_request_id;
		auto request = "{}"_json;
		request["jsonrpc"] = "2.0";
		request["id"] = req_id;
		request["method"] = std::move(method);
		request["params"] = std::move(params);
		std::unique_lock lk(_req_callbacks_mutex);
		_req_callbacks[req_id] = std::move(callback);
		lk.unlock();
		_sender.send(request);

		return static_cast<request_id>(req_id);
	}

	auto cancel_request(request_id req_id) -> bool {
		auto             req_id_int = static_cast<int>(req_id);
		std::unique_lock lk(_req_callbacks);
		if(!_req_callbacks.contains(req_id_int)) {
			return false;
		}
		lk.unlock();

		auto params = "{}"_json;
		params["id"] = req_id_int;
		send_request("$/cancelRequest", std::move(params), [](json) {});
		return true;
	}

	auto send_notification(std::string method, json params) {
		auto notification = "{}"_json;
		notification["jsonrpc"] = "2.0";
		notification["method"] = std::move(method);
		notification["params"] = std::move(params);
		_sender.send(notification);
	}

	/**
	 * The show message request is sent from a server to a client to ask the
	 * client to display a particular message in the user interface. In addition
	 * to the show message notification the request allows to pass actions and to
	 * wait for an answer from the client.
	 *
	 * SEE:
	 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#window_showMessageRequest
	 */
	auto show_message(
		message_type                                            type,
		std::string                                             message,
		std::vector<message_action_item>                        actions,
		std::function<void(std::optional<message_action_item>)> callback
	) -> request_id {
		auto params = "{}"_json;
		params["type"] = static_cast<int>(type);
		params["message"] = message;
		params["actions"] = actions;
		return send_request(
			"window/showMessageRequest",
			std::move(params),
			[callback = std::move(callback)](json response) {
				auto result = response["result"];
				if(result.is_null()) {
					callback({});
				} else {
					callback(result.get<message_action_item>());
				}
			}
		);
	}

	auto show_message(message_type type, std::string message) {
		auto params = "{}"_json;
		params["type"] = static_cast<int>(type);
		params["message"] = message;
		send_notification("window/showMessage", std::move(params));
	}

	auto log_message(message_type type, std::string message) {
		auto params = "{}"_json;
		params["type"] = static_cast<int>(type);
		params["message"] = message;
		send_notification("window/logMessage", std::move(params));
	}

	auto add_notification_listener(
		std::string                      method,
		std::function<void(const json&)> listener
	) -> void {
		_listeners.insert({method, std::move(listener)});
	}

	auto set_request_handler(
		std::string               method,
		std::function<json(json)> handler
	) {
		_request_handlers[method] = std::move(handler);
	}
};

template<typename Sender, typename Receiver>
message_manager(Sender&&, Receiver&&) -> message_manager<Sender, Receiver>;

} // namespace ecsact_lsp
