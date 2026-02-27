#include <iostream>
#include <tuple>
#include <atomic>
#include <mutex>
#include <map>
#include <unordered_map>
#include <charconv>
#include <string_view>
#include <string>
#include "docopt.h"
#include "nlohmann/json.hpp"
#include "nlohmann/adl_serializer.hpp"

#include "./details/jsonrpc_stream.hh"
#include "./details/message_manager.hh"
#include "./details/message_receiver.hh"
#include "./details/message_sender.hh"
#include "./details/messages.hh"
#include "./details/workspace_manager.hh"

#ifdef _WIN32
#	include <fcntl.h>
#	include <io.h>
#endif

using nlohmann::json;
using namespace std::string_literals;

constexpr auto USAGE = R"(
Usage:
	ecsact_lsp_server --stdio
	ecsact_lsp_server --socket [--port=<port>]

Options:
	--stdio
		Communicate over stdout and stdin.

	--socket
		Communicate over a socket.

	-p, --port=<port>
		Port to communicate over. Only valid while using `--socket`
)";

auto main(int argc, char* argv[]) -> int {
#ifdef _WIN32
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
	_setmode(_fileno(stderr), _O_BINARY);
#endif

	auto options = docopt::docopt(USAGE, {argv + 1, argv + argc});

	if(options["--port"]) {
		std::cerr << "non-stdio communication unimplemented\n";
		return 1;
	}

	std::ios_base::sync_with_stdio(false);
	auto stream = ecsact_lsp::jsonrpc_stream{std::cin, std::cout};
	auto sender = ecsact_lsp::message_sender(stream);
	auto receiver = ecsact_lsp::message_receiver(stream);
	auto manager = ecsact_lsp::message_manager(sender, receiver);

	ecsact_lsp::workspace_manager workspace_manager(manager);

	manager.set_request_handler("initialize", [&](json params) -> json {
		if(params.contains("trace")) {
			manager.set_trace(params["trace"].get<ecsact_lsp::trace_value>());
		}

		if(params.contains("workspaceFolders") &&
			 params["workspaceFolders"].is_array()) {
			auto workspace_folders =
				params["workspaceFolders"]
					.get<std::vector<ecsact_lsp::workspace_folder>>();
			for(auto const& folder : workspace_folders) {
				workspace_manager.add_workspace(folder);
			}
		}

		return nlohmann::json{
			{
				"capabilities",
				{
					{
						"textDocumentSync",
						{
							{"openClose", true},
							{"change", 1 /* full */},
						},
					},
					{
						"workspace",
						{
							"workspaceFolders",
							{
								{"supported", true},
								{"changeNotifications", true},
							},
						},
					},
					{"definitionProvider", true},
					{"hoverProvider", true},
					{"referencesProvider", true},
					{"documentSymbolProvider", true},
					{"workspaceSymbolProvider", true},
					{
						"completionProvider",
						{
							{"resolveProvider", false},
							{"triggerCharacters", {"."}},
						},
					},
					{
						"codeLensProvider",
						{
							{"resolveProvider", false},
						},
					},
					{
						"inlayHintProvider",
						{
							{"resolveProvider", false},
						},
					},
					{"renameProvider", true},
				},
			},
			{
				"serverInfo",
				{
					{"name", "ecsact_lsp_server"},
					{"version", "0.1.0"},
				},
			},
		};
	});

	manager.set_request_handler("textDocument/rename", [&](json params) -> json {
		auto rename_params = params.get<ecsact_lsp::rename_params>();
		auto result = workspace_manager.rename(
			rename_params.textDocument.uri,
			rename_params.position,
			rename_params.newName
		);

		if(result) {
			return *result;
		}

		return "null"_json;
	});

	manager.set_request_handler(
		"textDocument/inlayHint",
		[&](json params) -> json {
			auto inlay_hint_params = params.get<ecsact_lsp::inlay_hint_params>();
			return workspace_manager.get_inlay_hints(
				inlay_hint_params.textDocument.uri,
				inlay_hint_params.range
			);
		}
	);

	manager.set_request_handler(
		"textDocument/definition",
		[&](json params) -> json {
			auto definition_params = params.get<ecsact_lsp::definition_params>();
			auto result = workspace_manager.goto_definition(
				definition_params.textDocument.uri,
				definition_params.position
			);

			if(result) {
				return *result;
			}

			return "null"_json;
		}
	);

	manager.set_request_handler(
		"textDocument/references",
		[&](json params) -> json {
			auto reference_params = params.get<ecsact_lsp::reference_params>();
			auto result = workspace_manager.find_references(
				reference_params.textDocument.uri,
				reference_params.position,
				reference_params.context
			);

			if(result) {
				return *result;
			}

			return "null"_json;
		}
	);

	manager.set_request_handler(
		"textDocument/documentSymbol",
		[&](json params) -> json {
			auto document_symbol_params =
				params.get<ecsact_lsp::document_symbol_params>();
			return workspace_manager.get_document_symbols(
				document_symbol_params.textDocument.uri
			);
		}
	);

	manager.set_request_handler("workspace/symbol", [&](json params) -> json {
		auto workspace_symbol_params =
			params.get<ecsact_lsp::workspace_symbol_params>();
		return workspace_manager.get_workspace_symbols(
			workspace_symbol_params.query
		);
	});

	manager.set_request_handler("ecsact/symbols", [&](json params) -> json {
		auto symbol_params = params.get<ecsact_lsp::ecsact_symbol_params>();
		auto result = workspace_manager.get_ecsact_symbols(
			symbol_params.textDocument.uri,
			symbol_params.position
		);

		if(result) {
			return *result;
		}

		return "null"_json;
	});

	manager.set_request_handler("textDocument/hover", [&](json params) -> json {
		auto hover_params = params.get<ecsact_lsp::hover_params>();
		auto result = workspace_manager.get_hover(
			hover_params.textDocument.uri,
			hover_params.position
		);

		if(result) {
			return *result;
		}

		return "null"_json;
	});

	manager.set_request_handler(
		"textDocument/completion",
		[&](json params) -> json {
			auto completion_params = params.get<ecsact_lsp::completion_params>();
			auto result = workspace_manager.get_completions(
				completion_params.textDocument.uri,
				completion_params.position
			);

			if(result) {
				return *result;
			}

			return "null"_json;
		}
	);

	manager.set_request_handler(
		"textDocument/codeLens",
		[&](json params) -> json {
			auto code_lens_params = params.get<ecsact_lsp::code_lens_params>();
			return workspace_manager.get_code_lenses(
				code_lens_params.textDocument.uri
			);
		}
	);

	manager.add_notification_listener("initialized", [&](json) {

	});

	manager.add_notification_listener("$/setTrace", [&](json params) {
		if(params.contains("value")) {
			manager.set_trace(params["value"].get<ecsact_lsp::trace_value>());
		}
	});

	bool shutdown_requested = false;
	manager.set_request_handler("shutdown", [&](json) -> json {
		shutdown_requested = true;
		return "null"_json;
	});
	manager.add_notification_listener("exit", [&](json) {
		// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#exit
		std::exit(shutdown_requested ? 0 : 1);
	});

	auto show_unexpected_language_message = [&](std::string lang) {
		manager.show_message(
			ecsact_lsp::message_type::error,
			"Unexpected languageId '" + lang + "'. Only 'ecsact' is allowed."
		);
	};

	manager.add_notification_listener("textDocument/didOpen", [&](json ev) {
		auto text_document =
			ev["textDocument"].get<ecsact_lsp::text_document_item>();

		if(text_document.languageId != "ecsact") {
			show_unexpected_language_message(text_document.languageId);
			return;
		}

		workspace_manager.add_document(
			text_document.uri,
			text_document.version,
			text_document.text
		);
	});

	manager.add_notification_listener("textDocument/didChange", [&](json ev) {
		auto text_document =
			ev["textDocument"].get<ecsact_lsp::versioned_text_document_identifier>();
		auto content_changes =
			ev["contentChanges"]
				.get<
					std::vector<ecsact_lsp::full_text_document_content_change_event>>();

		if(content_changes.size() != 1) {
			return;
		}

		workspace_manager.update_document(
			text_document.uri,
			text_document.version,
			content_changes[0].text
		);
	});

	manager.add_notification_listener("textDocument/didClose", [&](json ev) {
		auto text_document =
			ev["textDocument"].get<ecsact_lsp::text_document_identifier>();

		workspace_manager.remove_document(text_document.uri);
	});

	manager.add_notification_listener(
		"workspace/didChangeWorkspaceFolders",
		[&](json notification) {
			auto& ev = notification["event"];
			auto  added_ws_dirs =
				ev["added"].get<std::vector<ecsact_lsp::workspace_folder>>();
			auto removed_ws_dirs =
				ev["removed"].get<std::vector<ecsact_lsp::workspace_folder>>();

			for(auto& removed : removed_ws_dirs) {
				workspace_manager.remove_workspace(removed);
			}

			for(auto& added : added_ws_dirs) {
				workspace_manager.add_workspace(added);
			}
		}
	);

	sender.start();
	receiver.start();

	while(manager.is_active()) {
		manager.pump();
	}

	sender.stop();
	receiver.stop();

	return 0;
}
