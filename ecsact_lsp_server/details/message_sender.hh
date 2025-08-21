#pragma once

#include <mutex>
#include <condition_variable>
#include <vector>
#include <optional>
#include <atomic>
#include <thread>
#include <iostream>
#include "nlohmann/json.hpp"

namespace ecsact_lsp {

template<typename Stream>
class message_sender {
	using json = nlohmann::json;

	std::condition_variable    _cv;
	std::mutex                 _mutex;
	std::vector<json>          _pending_messages;
	std::optional<std::thread> _send_thread;
	std::atomic_bool           _stop;
	Stream&&                   _stream;

public:
	message_sender(Stream&& stream) : _stream(std::forward<Stream>(stream)) {
	}

	~message_sender() {
		stop();
		if(_send_thread && _send_thread->joinable()) {
			_send_thread->join();
		}
	}

	void start() {
		if(_send_thread) {
			return;
		}

		_send_thread = std::thread([this] {
			while(!_stop) {
				std::unique_lock lk(_mutex);
				_cv.wait(lk, [this] { return !_pending_messages.empty(); });
				auto msg = _pending_messages.back();
				_pending_messages.pop_back();
				lk.unlock();
				_stream.send(msg);
			}
		});
	}

	auto stopped() const -> bool {
		return _stop;
	}

	void stop() {
		if(_send_thread && !_stop.exchange(true)) {
			_send_thread->join();
			_send_thread = {};
		}
	}

	void send(json msg) {
		std::unique_lock lk(_mutex);
		_pending_messages.push_back(std::move(msg));
		lk.unlock();
		_cv.notify_one();
	}
};

template<typename Stream>
message_sender(Stream&&) -> message_sender<Stream>;

} // namespace ecsact_lsp
