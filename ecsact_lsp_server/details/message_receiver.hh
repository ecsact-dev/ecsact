#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <condition_variable>
#include <optional>
#include <functional>
#include <iostream>
#include "nlohmann/json.hpp"

namespace ecsact_lsp {

template<typename Stream>
class message_receiver {
	using json = nlohmann::json;

	std::condition_variable                _cv;
	std::mutex                             _mutex;
	std::vector<std::function<void(json)>> _pending_acceptors;
	std::optional<std::thread>             _rcv_thread;
	std::atomic_bool                       _stop = true;
	Stream&&                               _stream;

public:
	message_receiver(Stream&& stream) : _stream(std::forward<Stream>(stream)) {
	}

	~message_receiver() {
		stop();
		if(_rcv_thread && _rcv_thread->joinable()) {
			_rcv_thread->join();
		}
	}

	void start() {
		if(_rcv_thread) {
			return;
		}

		if(!_stop.exchange(false)) {
			return;
		}

		_rcv_thread = std::thread([this] {
			while(!_stop) {
				std::unique_lock lk(_mutex);
				_cv.wait(lk, [this] { return !_pending_acceptors.empty(); });
				auto acceptor = std::move(_pending_acceptors.back());
				_pending_acceptors.pop_back();
				lk.unlock();

				auto res = _stream.receive();
				if(res) {
					acceptor(*res);
				} else {
					_stop = true;
				}
			}
		});
	}

	auto stopped() const -> bool {
		return _stop;
	}

	void stop() {
		if(_rcv_thread && !_stop.exchange(true)) {
			_rcv_thread->join();
			_rcv_thread = {};
		}
	}

	void accept_one(std::function<void(json)> acceptor) {
		std::unique_lock lk(_mutex);
		_pending_acceptors.push_back(std::move(acceptor));
		lk.unlock();
		_cv.notify_one();
	}
};

template<typename Stream>
message_receiver(Stream&&) -> message_receiver<Stream>;

} // namespace ecsact_lsp
