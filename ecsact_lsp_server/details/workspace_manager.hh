#pragma once

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <format>
#include <string>
#include <vector>
#include <string_view>
#include <ranges>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "ecsact/parse.h"
#include "ecsact/parse/statements.h"
#include "ecsact/interpret/eval.h"
#include "ecsact/runtime/dynamic.h"
#include "ecsact/runtime/meta.h"
#include "ecsact/runtime/meta.hh"
#include <magic_enum/magic_enum.hpp>

#include "./messages.hh"
#include "./interfaces.hh"

namespace ecsact_lsp {

namespace detail {
inline auto uri_to_path(std::string uri) -> std::filesystem::path {
	if(uri.starts_with("file:///")) {
		auto path_str = uri.substr(8);
		return std::filesystem::path(path_str);
	}
	if(uri.starts_with("file://")) {
		return std::filesystem::path(uri.substr(7));
	}
	return std::filesystem::path(uri);
}

inline auto path_to_uri(std::filesystem::path path) -> std::string {
	auto path_str = path.generic_string();
	if(!path_str.starts_with("/")) {
		return "file:///" + path_str;
	}
	return "file://" + path_str;
}
} // namespace detail

struct document_state {
	std::optional<ecsact_package_id>           package_id;
	std::vector<ecsact_package_id>             imports;
	std::vector<std::vector<ecsact_statement>> parse_stacks;
	std::vector<ecsact_parse_status>           parse_statuses;
	std::string                                full_text;
	std::string_view                           next_parse_view;
};

struct workspace_state {
	std::string                     uri;
	std::unordered_set<std::string> document_uris;
};

inline auto get_source_range(
	const std::string&         full_text,
	const ecsact_statement_sv& loc
) -> range {
	auto r = range{};
	auto index = 0;
	auto cptr = full_text.data();

	if(loc.data < full_text.data() ||
		 loc.data >= full_text.data() + full_text.size()) {
		return r;
	}

	while(index < full_text.size() && cptr != loc.data) {
		if(*cptr == '\n') {
			r.start.line += 1;
			r.start.character = 0;
		} else {
			r.start.character += 1;
		}

		++cptr;
		++index;
	}

	r.end = r.start;
	r.end.character += loc.length;

	return r;
}

inline auto builtin_type_name(ecsact_builtin_type type) -> std::string {
	switch(type) {
		case ECSACT_BOOL:
			return "bool";
		case ECSACT_I8:
			return "i8";
		case ECSACT_U8:
			return "u8";
		case ECSACT_I16:
			return "i16";
		case ECSACT_U16:
			return "u16";
		case ECSACT_I32:
			return "i32";
		case ECSACT_U32:
			return "u32";
		case ECSACT_F32:
			return "f32";
		case ECSACT_I64:
			return "i64";
		case ECSACT_U64:
			return "u64";
		case ECSACT_F64:
			return "f64";
		case ECSACT_ENTITY_TYPE:
			return "entity";
	}
	return "unknown builtin type (" + std::to_string(static_cast<int>(type)) + ")";
}

template<typename E>
inline auto enum_name_safe(E value, std::string fallback) -> std::string {
	if(magic_enum::enum_cast<E>(value)) {
		return std::string{magic_enum::enum_name(value)};
	}
	return fallback + " (" + std::to_string(static_cast<int>(value)) + ")";
}

inline auto pretty_statement_type_name(ecsact_statement_type type)
	-> std::string {
	switch(type) {
		case ECSACT_STATEMENT_NONE:
			return "none";
		case ECSACT_STATEMENT_UNKNOWN:
			return "unknown";
		case ECSACT_STATEMENT_PACKAGE:
			return "package";
		case ECSACT_STATEMENT_IMPORT:
			return "import";
		case ECSACT_STATEMENT_COMPONENT:
			return "component";
		case ECSACT_STATEMENT_TRANSIENT:
			return "transient";
		case ECSACT_STATEMENT_SYSTEM:
			return "system";
		case ECSACT_STATEMENT_ACTION:
			return "action";
		case ECSACT_STATEMENT_ENUM:
			return "enum";
		case ECSACT_STATEMENT_ENUM_VALUE:
			return "enum value";
		case ECSACT_STATEMENT_BUILTIN_TYPE_FIELD:
		case ECSACT_STATEMENT_USER_TYPE_FIELD:
		case ECSACT_STATEMENT_ENTITY_FIELD:
			return "field";
		case ECSACT_STATEMENT_SYSTEM_COMPONENT:
			return "system capability";
		case ECSACT_STATEMENT_SYSTEM_GENERATES:
			return "generates";
		case ECSACT_STATEMENT_SYSTEM_WITH:
			return "with";
		case ECSACT_STATEMENT_ENTITY_CONSTRAINT:
			return "entity constraint";
		case ECSACT_STATEMENT_SYSTEM_NOTIFY:
			return "system notify";
		case ECSACT_STATEMENT_SYSTEM_NOTIFY_COMPONENT:
			return "system notify component";
		case ECSACT_STATEMENT_CLUSTER:
			return "cluster";
	}

	return enum_name_safe(type, "unknown statement");
}

inline auto parse_stack(
	std::string_view               text,
	ecsact_parse_status&           status,
	std::vector<ecsact_statement>& stack
) -> std::string_view {
	auto& statement = stack.emplace_back();
	auto  context = stack.size() == 1 ? nullptr : &stack[stack.size() - 2];

	auto read_amount = ecsact_parse_statement(
		text.data(),
		static_cast<int>(text.size()),
		context,
		&statement,
		&status
	);

	if(ecsact_is_error_parse_status_code(status.code)) {
		return {};
	}

	return text.substr(read_amount);
}

static inline auto create_eval_error_diagnostics(
	const document_state&    doc,
	const ecsact_statement&  statement,
	const ecsact_eval_error& err
) -> diagnostic {
	using namespace std::string_literals;

	if(err.code == ECSACT_EVAL_ERR_INVALID_CONTEXT) {
		auto r = get_source_range(doc.full_text, err.relevant_content);
		auto message = [&]() -> std::string {
			if(err.context_type == ECSACT_STATEMENT_NONE) {
				return pretty_statement_type_name(statement.type) +
					" is not allowed as a top level statement"s;
			} else {
				return pretty_statement_type_name(statement.type) +
					" is not allowed in "s +
					pretty_statement_type_name(err.context_type) + " context";
			}
		}();

		return diagnostic{
			.range = r,
			.severity = diagnostic_severity::error,
			.message = message,
		};
	} else if(err.code != ECSACT_EVAL_OK) {
		auto r = get_source_range(doc.full_text, err.relevant_content);
		auto message = enum_name_safe(err.code, "unknown eval error");
		if(err.relevant_content.length > 0) {
			message += " ";
			message += std::string_view{
				err.relevant_content.data,
				static_cast<size_t>(err.relevant_content.length),
			};
		}

		return diagnostic{
			.range = r,
			.severity = diagnostic_severity::error,
			.message = message,
		};
	}

	assert(err.code != ECSACT_EVAL_OK);
	return {};
}

template<send_interface Send>
class workspace_manager {
	std::unordered_map<std::string, workspace_state> _workspaces;
	std::unordered_map<std::string, document_state>  _documents;
	Send&&                                           send;

	static auto get_name_from_sv(const ecsact_statement_sv& sv) -> std::string {
		if(sv.data == nullptr || sv.length < 0) {
			return "";
		}
		return std::string(sv.data, static_cast<size_t>(sv.length));
	}

	auto _parse_document(std::string uri, int version, document_state& doc) {
		doc.parse_stacks.clear();
		doc.parse_statuses.clear();

		auto diagnostics = std::vector<diagnostic>{};

		while(!doc.next_parse_view.empty()) {
			auto& status = doc.parse_statuses.emplace_back();
			auto& stack = doc.parse_stacks.emplace_back();

			if(doc.parse_stacks.size() > 1) {
				auto last_status = doc.parse_statuses[doc.parse_statuses.size() - 2];
				stack = doc.parse_stacks[doc.parse_stacks.size() - 2];
				if(last_status.code == ECSACT_PARSE_STATUS_OK) {
					stack.pop_back();
				} else if(last_status.code == ECSACT_PARSE_STATUS_BLOCK_END) {
					stack.pop_back();
					if(!stack.empty()) {
						stack.pop_back();
					}
				}
			}

			auto current_parse_view = doc.next_parse_view;
			doc.next_parse_view = parse_stack(current_parse_view, status, stack);

			if(!ecsact_is_error_parse_status_code(status.code)) {
				auto& statement = stack.back();
				if(statement.type == ECSACT_STATEMENT_UNKNOWN) {
					auto loc = ecsact_statement_sv{
						.data = current_parse_view.data(),
						.length = static_cast<int32_t>(current_parse_view.length()),
					};
					auto r = get_source_range(doc.full_text, loc);
					diagnostics.push_back(
						diagnostic{
							.range = r,
							.severity = diagnostic_severity::error,
							.message{"Unknown statement"},
						}
					);
				}
			}
		}

		for(auto& status : doc.parse_statuses) {
			if(ecsact_is_error_parse_status_code(status.code)) {
				auto r = get_source_range(doc.full_text, status.error_location);

				diagnostics.push_back(
					diagnostic{
						.range = r,
						.severity = diagnostic_severity::error,
						.message{enum_name_safe(status.code, "unknown parse error")},
					}
				);
			}
		}

		send.send_notification(
			"textDocument/publishDiagnostics",
			nlohmann::json{
				{"uri", uri},
				{"version", version},
				{"diagnostics", diagnostics},
			}
		);
	}

	auto _interpret_document(std::string uri, int version, document_state& doc) {
		using std::views::drop;
		using namespace std::string_literals;

		if(doc.parse_stacks.empty()) {
			return;
		}

		if(doc.package_id) {
			ecsact_destroy_package(*doc.package_id);
			doc.package_id = std::nullopt;
		}

		auto  diagnostics = std::vector<diagnostic>{};
		auto& package_parse_stack = doc.parse_stacks.front();

		if(package_parse_stack.empty()) {
			return;
		}

		auto& package_statement = package_parse_stack.front();

		if(package_statement.type != ECSACT_STATEMENT_PACKAGE) {
			diagnostics.push_back(
				diagnostic{
					.range{},
					.severity = diagnostic_severity::warning,
					.message = "First statement must be a package statement",
				}
			);
		} else {
			doc.package_id = ecsact_eval_package_statement( //
				&package_statement.data.package_statement
			);
		}

		if(doc.package_id) {
			auto eval_statement_tracker = std::set<int32_t>{};
			auto parser_to_runtime_id = std::unordered_map<int32_t, int32_t>{};
			auto system_ranges = std::unordered_map<ecsact_system_like_id, range>{};

			// Evaluate all statements in stacks except first (package)
			for(size_t stack_idx = 1; stack_idx < doc.parse_stacks.size();
					++stack_idx) {
				auto& parse_stack = doc.parse_stacks[stack_idx];
				auto& status = doc.parse_statuses[stack_idx];

				for(auto idx = 0; parse_stack.size() > idx; ++idx) {
					auto&      statement = parse_stack[idx];
					auto       original_id = statement.id;
					const auto runtime_id_itr = parser_to_runtime_id.find(original_id);

					if(runtime_id_itr != parser_to_runtime_id.end()) {
						statement.id = runtime_id_itr->second;
						continue;
					}

					if(eval_statement_tracker.contains(original_id)) {
						continue;
					}

					eval_statement_tracker.insert(original_id);

					auto eval_err = ecsact_eval_statement(
						*doc.package_id,
						static_cast<int32_t>(idx + 1),
						parse_stack.data()
					);

					if(eval_err.code != ECSACT_EVAL_OK) {
						diagnostics.push_back(
							create_eval_error_diagnostics(doc, statement, eval_err)
						);
					} else {
						parser_to_runtime_id[original_id] = statement.id;
						if(statement.type == ECSACT_STATEMENT_SYSTEM) {
							system_ranges[static_cast<ecsact_system_like_id>(statement.id)] =
								get_source_range(
									doc.full_text,
									statement.data.system_statement.system_name
								);
						} else if(statement.type == ECSACT_STATEMENT_ACTION) {
							system_ranges[static_cast<ecsact_system_like_id>(statement.id)] =
								get_source_range(
									doc.full_text,
									statement.data.action_statement.action_name
								);
						}
					}
				}

				if(status.code == ECSACT_PARSE_STATUS_BLOCK_END) {
					if(parse_stack.size() >= 2) {
						auto& block_start = parse_stack[parse_stack.size() - 2];
						if(block_start.type == ECSACT_STATEMENT_ACTION) {
							auto& data = block_start.data.action_statement;
							auto  act_id = static_cast<ecsact_action_id>(block_start.id);

							if(ecsact::meta::system_capabilities(act_id).empty()) {
								diagnostics.push_back(
									diagnostic{
										.range = get_source_range(doc.full_text, data.action_name),
										.severity = diagnostic_severity::error,
										.message = "Action must have at least one capability",
									}
								);
							}
						}
					}
				}
			}

			auto make_cluster_error_message =
				[&](ecsact_execution_batches_error err) -> std::string {
				auto sys_name = std::string(
					ecsact_meta_system_name(static_cast<ecsact_system_id>(err.system_id))
				);

				if(static_cast<int32_t>(err.conflicting_system_id) == -1) {
					return std::format(
						"System '{}' cannot be part of the explicit cluster it is in",
						sys_name
					);
				}

				auto conflicting_sys_name = std::string(ecsact_meta_system_name(
					static_cast<ecsact_system_id>(err.conflicting_system_id)
				));

				auto comp_name = std::string(ecsact_meta_component_name(
					static_cast<ecsact_component_id>(err.component_id)
				));

				return std::format(
					"System '{}' conflicts with system '{}' on component '{}' and "
					"cannot be part of the same explicit cluster",
					sys_name,
					conflicting_sys_name,
					comp_name
				);
			};

			auto err = ecsact_check_execution_batches(*doc.package_id);
			if(static_cast<int32_t>(err.system_id) != -1) {
				diagnostics.push_back(
					diagnostic{
						.range = system_ranges[err.system_id],
						.severity = diagnostic_severity::error,
						.message = make_cluster_error_message(err),
					}
				);
			}

			for(auto sys_id : ecsact::meta::get_system_ids(*doc.package_id)) {
				auto nested_err = ecsact_check_system_execution_batches(
					static_cast<ecsact_system_like_id>(sys_id)
				);
				if(static_cast<int32_t>(nested_err.system_id) != -1) {
					diagnostics.push_back(
						diagnostic{
							.range = system_ranges[nested_err.system_id],
							.severity = diagnostic_severity::error,
							.message = make_cluster_error_message(nested_err),
						}
					);
				}
			}
		}

		if(!diagnostics.empty()) {
			send.send_notification(
				"textDocument/publishDiagnostics",
				nlohmann::json{
					{"uri", uri},
					{"version", version},
					{"diagnostics", diagnostics},
				}
			);
		}
	}

public:
	workspace_manager(Send&& send) : send(std::forward<Send>(send)) {
	}

	auto add_workspace(workspace_folder ws) -> void {
		auto ws_path = detail::uri_to_path(ws.uri);
		if(!std::filesystem::exists(ws_path)) {
			return;
		}

		_workspaces[ws.uri] = workspace_state{.uri = ws.uri};
		auto& workspace = _workspaces[ws.uri];

		for(auto const& entry :
				std::filesystem::recursive_directory_iterator(ws_path)) {
			if(entry.is_regular_file() && entry.path().extension() == ".ecsact") {
				auto uri = detail::path_to_uri(entry.path());
				workspace.document_uris.insert(uri);

				if(!_documents.contains(uri)) {
					std::ifstream file(entry.path());
					if(file.is_open()) {
						std::string content(
							(std::istreambuf_iterator<char>(file)),
							std::istreambuf_iterator<char>()
						);
						add_document(uri, 0, content);
					}
				}
			}
		}
	}

	auto remove_workspace(workspace_folder ws) -> void {
		auto it = _workspaces.find(ws.uri);
		if(it != _workspaces.end()) {
			for(auto const& doc_uri : it->second.document_uris) {
				remove_document(doc_uri);
			}
			_workspaces.erase(it);
		}
	}

	auto add_document(std::string uri, int initial_version, std::string text)
		-> void {
		_documents[uri] = document_state{};
		auto& doc = _documents[uri];
		doc.full_text = std::move(text);
		doc.next_parse_view = doc.full_text;
		_parse_document(uri, initial_version, doc);
		_interpret_document(uri, initial_version, doc);
	}

	auto update_document(std::string uri, int version, std::string text) -> void {
		_documents[uri] = document_state{};
		auto& doc = _documents[uri];
		doc.full_text = std::move(text);
		doc.next_parse_view = doc.full_text;
		_parse_document(uri, version, doc);
		_interpret_document(uri, version, doc);
	}

	auto remove_document(std::string uri) -> void {
	}

	auto find_statement_at(std::string uri, position pos)
		-> std::optional<std::pair<std::vector<ecsact_statement>, range>> {
		auto doc_it = _documents.find(uri);
		if(doc_it == _documents.end()) {
			return std::nullopt;
		}

		auto& doc = doc_it->second;

		for(auto& stack : doc.parse_stacks) {
			if(stack.empty()) {
				continue;
			}

			auto& statement = stack.back();

			auto in_range = [&](const ecsact_statement_sv& sv) -> bool {
				if(sv.data == nullptr) {
					return false;
				}
				auto r = get_source_range(doc.full_text, sv);
				bool result = pos.line == r.start.line &&
					pos.character >= r.start.character &&
					pos.character <= r.end.character;
				return result;
			};

			auto name_sv = ecsact_statement_sv{};

			switch(statement.type) {
				case ECSACT_STATEMENT_PACKAGE:
					name_sv = statement.data.package_statement.package_name;
					break;
				case ECSACT_STATEMENT_IMPORT:
					name_sv = statement.data.import_statement.import_package_name;
					break;
				case ECSACT_STATEMENT_COMPONENT:
					name_sv = statement.data.component_statement.component_name;
					break;
				case ECSACT_STATEMENT_TRANSIENT:
					name_sv = statement.data.transient_statement.transient_name;
					break;
				case ECSACT_STATEMENT_SYSTEM:
					name_sv = statement.data.system_statement.system_name;
					break;
				case ECSACT_STATEMENT_ACTION:
					name_sv = statement.data.action_statement.action_name;
					break;
				case ECSACT_STATEMENT_ENUM:
					name_sv = statement.data.enum_statement.enum_name;
					break;
				case ECSACT_STATEMENT_ENUM_VALUE:
					name_sv = statement.data.enum_value_statement.name;
					break;
				case ECSACT_STATEMENT_BUILTIN_TYPE_FIELD:
					name_sv = statement.data.field_statement.field_name;
					break;
				case ECSACT_STATEMENT_USER_TYPE_FIELD:
					name_sv = statement.data.user_type_field_statement.field_name;
					break;
				case ECSACT_STATEMENT_SYSTEM_COMPONENT:
					name_sv = statement.data.system_component_statement.component_name;
					break;
				case ECSACT_STATEMENT_ENTITY_CONSTRAINT:
					name_sv =
						statement.data.entity_constraint_statement.constraint_component_name;
					break;
				case ECSACT_STATEMENT_SYSTEM_NOTIFY_COMPONENT:
					name_sv =
						statement.data.system_notify_component_statement.component_name;
					break;
				default:
					break;
			}

			if(in_range(name_sv)) {
				return std::make_pair(stack, get_source_range(doc.full_text, name_sv));
			}

			if(statement.type == ECSACT_STATEMENT_USER_TYPE_FIELD) {
				name_sv = statement.data.user_type_field_statement.user_type_name;
				if(in_range(name_sv)) {
					return std::make_pair(
						stack,
						get_source_range(doc.full_text, name_sv)
					);
				}
			}
		}

		return std::nullopt;
	}

	auto get_hover(std::string uri, position pos) -> std::optional<hover> {
		auto found = find_statement_at(uri, pos);
		if(!found) {
			return std::nullopt;
		}

		auto& [stack, r] = *found;
		auto& statement = stack.back();

		std::string hover_text;

		auto get_full_name = [&](int32_t id) -> std::string {
			if(id == -1) {
				return "";
			}
			auto full_name =
				ecsact_meta_decl_full_name(static_cast<ecsact_decl_id>(id));
			return full_name ? std::string(full_name) : "";
		};

		auto build_decl_hover =
			[&](int32_t id, ecsact_statement_type type, const ecsact_statement& stmt) {
			auto name = get_full_name(id);
			if(name.empty()) {
				// fallback if meta doesn't have it yet or it's not in a package
				name = "unknown";
				if(type == ECSACT_STATEMENT_COMPONENT) {
					name = get_name_from_sv(stmt.data.component_statement.component_name);
				} else if(type == ECSACT_STATEMENT_TRANSIENT) {
					name = get_name_from_sv(stmt.data.transient_statement.transient_name);
				} else if(type == ECSACT_STATEMENT_SYSTEM) {
					name = get_name_from_sv(stmt.data.system_statement.system_name);
				} else if(type == ECSACT_STATEMENT_ACTION) {
					name = get_name_from_sv(stmt.data.action_statement.action_name);
				} else if(type == ECSACT_STATEMENT_ENUM) {
					name = get_name_from_sv(stmt.data.enum_statement.enum_name);
				}

				// Try to construct full name manually if it's just the short name
				if(id != -1 && name != "unknown" && name.find('.') == std::string::npos) {
					std::vector<ecsact_package_id> packages;
					auto doc_it = _documents.find(uri);
					if(doc_it != _documents.end() && doc_it->second.package_id) {
						packages.push_back(*doc_it->second.package_id);
					}

					int32_t                        packages_count = 0;
					int32_t                        max_packages = 256;
					std::vector<ecsact_package_id> all_packages(max_packages);
					ecsact_meta_get_package_ids(
						max_packages,
						all_packages.data(),
						&packages_count
					);
					for(int32_t i = 0; packages_count > i; ++i) {
						if(doc_it == _documents.end() || all_packages[i] != *doc_it->second.package_id) {
							packages.push_back(all_packages[i]);
						}
					}

					for(auto package_id : packages) {
						bool found = false;
						if(type == ECSACT_STATEMENT_COMPONENT) {
							int32_t              count = ecsact_meta_count_components(package_id);
							std::vector<ecsact_component_id> ids(count);
							ecsact_meta_get_component_ids(package_id, count, ids.data(), &count);
							for(auto comp_id : ids) {
								if(static_cast<int32_t>(comp_id) == id) {
									found = true;
									break;
								}
							}
						} else if(type == ECSACT_STATEMENT_TRANSIENT) {
							int32_t              count = ecsact_meta_count_transients(package_id);
							std::vector<ecsact_transient_id> ids(count);
							ecsact_meta_get_transient_ids(package_id, count, ids.data(), &count);
							for(auto trans_id : ids) {
								if(static_cast<int32_t>(trans_id) == id) {
									found = true;
									break;
								}
							}
						} else if(type == ECSACT_STATEMENT_SYSTEM) {
							int32_t              count = ecsact_meta_count_systems(package_id);
							std::vector<ecsact_system_id> ids(count);
							ecsact_meta_get_system_ids(package_id, count, ids.data(), &count);
							for(auto sys_id : ids) {
								if(static_cast<int32_t>(sys_id) == id) {
									found = true;
									break;
								}
							}
						} else if(type == ECSACT_STATEMENT_ACTION) {
							int32_t              count = ecsact_meta_count_actions(package_id);
							std::vector<ecsact_action_id> ids(count);
							ecsact_meta_get_action_ids(package_id, count, ids.data(), &count);
							for(auto act_id : ids) {
								if(static_cast<int32_t>(act_id) == id) {
									found = true;
									break;
								}
							}
						} else if(type == ECSACT_STATEMENT_ENUM) {
							int32_t              count = ecsact_meta_count_enums(package_id);
							std::vector<ecsact_enum_id> ids(count);
							ecsact_meta_get_enum_ids(package_id, count, ids.data(), &count);
							for(auto enum_id : ids) {
								if(static_cast<int32_t>(enum_id) == id) {
									found = true;
									break;
								}
							}
						}

						if(found) {
							auto pkg_name = ecsact_meta_package_name(package_id);
							if(pkg_name) {
								name = std::format("{}.{}", pkg_name, name);
							}
							break;
						}
					}
				}

				// Last resort: search documents for package statement
				if(name != "unknown" && name.find('.') == std::string::npos) {
					for(auto& [doc_uri, doc_state] : _documents) {
						bool has_decl = false;
						for(auto& d_stack : doc_state.parse_stacks) {
							if(d_stack.empty()) continue;
							auto& d_stmt = d_stack.back();
							std::string d_name;
							if(d_stmt.type == ECSACT_STATEMENT_COMPONENT) d_name = get_name_from_sv(d_stmt.data.component_statement.component_name);
							else if(d_stmt.type == ECSACT_STATEMENT_TRANSIENT) d_name = get_name_from_sv(d_stmt.data.transient_statement.transient_name);
							else if(d_stmt.type == ECSACT_STATEMENT_SYSTEM) d_name = get_name_from_sv(d_stmt.data.system_statement.system_name);
							else if(d_stmt.type == ECSACT_STATEMENT_ACTION) d_name = get_name_from_sv(d_stmt.data.action_statement.action_name);
							else if(d_stmt.type == ECSACT_STATEMENT_ENUM) d_name = get_name_from_sv(d_stmt.data.enum_statement.enum_name);

							if(d_name == name) {
								has_decl = true;
								break;
							}
						}

						if(has_decl) {
							for(auto& d_stack : doc_state.parse_stacks) {
								if(d_stack.empty()) continue;
								auto& d_stmt = d_stack.front();
								if(d_stmt.type == ECSACT_STATEMENT_PACKAGE) {
									auto pkg_name = get_name_from_sv(d_stmt.data.package_statement.package_name);
									if(!pkg_name.empty()) {
										name = pkg_name + "." + name;
									}
									break;
								}
							}
							break;
						}
					}
				}
			}
			hover_text +=
				"### " + pretty_statement_type_name(type) + " `" + name + "`\n";

			if(type == ECSACT_STATEMENT_SYSTEM || type == ECSACT_STATEMENT_ACTION) {
				auto sys_like_id = static_cast<ecsact_system_like_id>(id);

				std::vector<ecsact_package_id> packages;
				int32_t                        packages_count = 0;
				int32_t                        max_packages = 256;
				packages.resize(max_packages);
				ecsact_meta_get_package_ids(
					max_packages,
					packages.data(),
					&packages_count
				);
				packages.resize(packages_count);

				for(auto package_id : packages) {
					int32_t batch_count = ecsact_meta_count_execution_batches(package_id);
					for(int32_t i = 0; batch_count > i; ++i) {
						int32_t                            systems_count = 0;
						std::vector<ecsact_system_like_id> systems;
						int32_t                            max_systems = 256;
						systems.resize(max_systems);
						ecsact_meta_get_execution_batch(
							package_id,
							i,
							max_systems,
							systems.data(),
							&systems_count
						);
						bool in_batch = false;
						for(int32_t j = 0; systems_count > j; ++j) {
							if(systems[j] == sys_like_id) {
								in_batch = true;
								break;
							}
						}

						if(in_batch) {
							hover_text += std::format("\n**Execution Batch {} systems:**\n", i);
							for(int32_t j = 0; systems_count > j; ++j) {
								auto other_sys_id = systems[j];
								auto other_name =
									get_full_name(static_cast<int32_t>(other_sys_id));
								if(other_name.empty()) {
									if(ecsact_meta_is_system(other_sys_id)) {
										other_name = ecsact_meta_system_name(
											static_cast<ecsact_system_id>(other_sys_id)
										);
									} else if(ecsact_meta_is_action(other_sys_id)) {
										other_name = ecsact_meta_action_name(
											static_cast<ecsact_action_id>(other_sys_id)
										);
									}
								}

								if(other_sys_id == sys_like_id) {
									hover_text += std::format(" - **`{}`** (this)\n", other_name);
								} else {
									hover_text += std::format(" - `{}`\n", other_name);
								}
							}
							break;
						}
					}
				}
			}
		};

		if(statement.type == ECSACT_STATEMENT_COMPONENT ||
			 statement.type == ECSACT_STATEMENT_TRANSIENT ||
			 statement.type == ECSACT_STATEMENT_SYSTEM ||
			 statement.type == ECSACT_STATEMENT_ACTION ||
			 statement.type == ECSACT_STATEMENT_ENUM) {
			build_decl_hover(statement.id, statement.type, statement);
		} else if(statement.type == ECSACT_STATEMENT_ENUM_VALUE) {
			std::string parent_name = "unknown";
			if(stack.size() >= 2) {
				auto& parent = stack[stack.size() - 2];
				auto  p_id = parent.id;
				parent_name = get_full_name(p_id);
				if(parent_name.empty()) {
					if(parent.type == ECSACT_STATEMENT_ENUM) {
						parent_name = get_name_from_sv(parent.data.enum_statement.enum_name);
					}
				}
			}
			auto value_name = get_name_from_sv(statement.data.enum_value_statement.name);
			hover_text += "### enum value `" + parent_name + "." + value_name + "`\n";
			hover_text += std::format("value: `{}`\n", statement.data.enum_value_statement.value);
		} else if(statement.type == ECSACT_STATEMENT_SYSTEM_COMPONENT ||
							statement.type == ECSACT_STATEMENT_ENTITY_CONSTRAINT ||
							statement.type == ECSACT_STATEMENT_SYSTEM_NOTIFY_COMPONENT ||
							statement.type == ECSACT_STATEMENT_USER_TYPE_FIELD) {
			std::string name;
			if(statement.type == ECSACT_STATEMENT_SYSTEM_COMPONENT) {
				name = get_name_from_sv(
					statement.data.system_component_statement.component_name
				);
			} else if(statement.type == ECSACT_STATEMENT_ENTITY_CONSTRAINT) {
				name = get_name_from_sv(
					statement.data.entity_constraint_statement.constraint_component_name
				);
			} else if(statement.type == ECSACT_STATEMENT_SYSTEM_NOTIFY_COMPONENT) {
				name = get_name_from_sv(
					statement.data.system_notify_component_statement.component_name
				);
			} else if(statement.type == ECSACT_STATEMENT_USER_TYPE_FIELD) {
				name =
					get_name_from_sv(statement.data.user_type_field_statement.user_type_name);
			}

			std::string search_name = name;
			auto last_dot = search_name.find_last_of('.');
			if(last_dot != std::string::npos) {
				search_name = search_name.substr(last_dot + 1);
			}

			// Find definition for this name
			for(auto& [doc_uri, doc_state] : _documents) {
				for(auto& d_stack : doc_state.parse_stacks) {
					if(d_stack.empty()) {
						continue;
					}
					auto& d_stmt = d_stack.back();
					std::string d_name;
					if(d_stmt.type == ECSACT_STATEMENT_COMPONENT) {
						d_name = get_name_from_sv(d_stmt.data.component_statement.component_name);
					} else if(d_stmt.type == ECSACT_STATEMENT_TRANSIENT) {
						d_name = get_name_from_sv(d_stmt.data.transient_statement.transient_name);
					} else if(d_stmt.type == ECSACT_STATEMENT_SYSTEM) {
						d_name = get_name_from_sv(d_stmt.data.system_statement.system_name);
					} else if(d_stmt.type == ECSACT_STATEMENT_ACTION) {
						d_name = get_name_from_sv(d_stmt.data.action_statement.action_name);
					} else if(d_stmt.type == ECSACT_STATEMENT_ENUM) {
						d_name = get_name_from_sv(d_stmt.data.enum_statement.enum_name);
					}

					if(!d_name.empty() && d_name == search_name) {
						build_decl_hover(d_stmt.id, d_stmt.type, d_stmt);
						break;
					}
				}
				if(!hover_text.empty()) {
					break;
				}
			}
		} else if(statement.type == ECSACT_STATEMENT_BUILTIN_TYPE_FIELD ||
							statement.type == ECSACT_STATEMENT_USER_TYPE_FIELD ||
							statement.type == ECSACT_STATEMENT_ENTITY_FIELD) {
			std::string parent_name = "unknown";
			if(stack.size() >= 2) {
				auto& parent = stack[stack.size() - 2];
				parent_name = get_full_name(parent.id);
				if(parent_name.empty()) {
					if(parent.type == ECSACT_STATEMENT_COMPONENT) {
						parent_name = get_name_from_sv(parent.data.component_statement.component_name);
					} else if(parent.type == ECSACT_STATEMENT_TRANSIENT) {
						parent_name = get_name_from_sv(parent.data.transient_statement.transient_name);
					} else if(parent.type == ECSACT_STATEMENT_ACTION) {
						parent_name = get_name_from_sv(parent.data.action_statement.action_name);
					}
				}

				// Construct parent full name if it's just short name
				if(!parent_name.empty() && parent_name != "unknown" && parent_name.find('.') == std::string::npos) {
					for(auto& [doc_uri, doc_state] : _documents) {
						for(auto& d_stack : doc_state.parse_stacks) {
							if(d_stack.empty()) continue;
							auto& d_stmt = d_stack.front();
							if(d_stmt.type == ECSACT_STATEMENT_PACKAGE) {
								bool has_parent_decl = false;
								for(auto& d_stack2 : doc_state.parse_stacks) {
									if(d_stack2.empty()) continue;
									auto& d_stmt2 = d_stack2.back();
									std::string d_name;
									if(d_stmt2.type == ECSACT_STATEMENT_COMPONENT) d_name = get_name_from_sv(d_stmt2.data.component_statement.component_name);
									else if(d_stmt2.type == ECSACT_STATEMENT_TRANSIENT) d_name = get_name_from_sv(d_stmt2.data.transient_statement.transient_name);
									else if(d_stmt2.type == ECSACT_STATEMENT_ACTION) d_name = get_name_from_sv(d_stmt2.data.action_statement.action_name);

									if(d_name == parent_name) {
										has_parent_decl = true;
										break;
									}
								}

								if(has_parent_decl) {
									auto pkg_name = get_name_from_sv(d_stmt.data.package_statement.package_name);
									if(!pkg_name.empty()) {
										parent_name = pkg_name + "." + parent_name;
									}
									break;
								}
							}
						}
						if(parent_name.find('.') != std::string::npos) break;
					}
				}
			}

			std::string field_name;
			std::string field_type_name;

			if(statement.type == ECSACT_STATEMENT_BUILTIN_TYPE_FIELD) {
				field_name = get_name_from_sv(statement.data.field_statement.field_name);
				field_type_name = builtin_type_name(statement.data.field_statement.field_type);
			} else if(statement.type == ECSACT_STATEMENT_USER_TYPE_FIELD) {
				field_name = get_name_from_sv(statement.data.user_type_field_statement.field_name);
				field_type_name = get_name_from_sv(statement.data.user_type_field_statement.user_type_name);
			} else if(statement.type == ECSACT_STATEMENT_ENTITY_FIELD) {
				field_name = get_name_from_sv(statement.data.field_statement.field_name);
				field_type_name = "entity";
			}

			hover_text += "### field `" + parent_name + "." + field_name + "`\n";
			hover_text += "type: `" + field_type_name + "`\n";
		}

		if(hover_text.empty()) {
			return std::nullopt;
		}

		return hover{
			.contents =
				{
					.kind = markup_kind::markdown,
					.value = hover_text,
				},
			.range = r,
		};
	}

	auto get_completions(std::string uri, position pos)
		-> std::optional<completion_list> {
		auto doc_it = _documents.find(uri);
		if(doc_it == _documents.end()) {
			return std::nullopt;
		}

		auto& doc = doc_it->second;

		auto lines = std::vector<std::string>{};
		auto ss = std::stringstream{doc.full_text};
		auto line = std::string{};
		while(std::getline(ss, line)) {
			lines.push_back(line);
		}

		if(pos.line >= lines.size()) {
			return std::nullopt;
		}

		auto current_line = lines[pos.line];
		auto line_view = std::string_view{current_line};
		auto trimmed_line = line_view;
		auto first_non_space = trimmed_line.find_first_not_of(" \t");
		if(first_non_space != std::string_view::npos) {
			trimmed_line.remove_prefix(first_non_space);
		}

		// What is the word immediately before the cursor?
		std::string word_before;
		if(pos.character > 0 && pos.character <= current_line.size()) {
			auto start = pos.character;
			while(start > 0 && (std::isalnum(current_line[start - 1]) || current_line[start - 1] == '_' || current_line[start - 1] == '.')) {
				start--;
			}
			word_before = current_line.substr(start, pos.character - start);
		}

		completion_list result{.isIncomplete = false};

		if(trimmed_line.starts_with("import")) {
			std::set<std::string> package_names;
			for(auto& [d_uri, d_state] : _documents) {
				for(auto& stack : d_state.parse_stacks) {
					if(stack.empty()) continue;
					auto& stmt = stack.front();
					if(stmt.type == ECSACT_STATEMENT_PACKAGE) {
						package_names.insert(get_name_from_sv(stmt.data.package_statement.package_name));
					}
				}
			}

			for(auto const& pkg_name : package_names) {
				result.items.push_back({
					.label = pkg_name,
					.kind = completion_item_kind::module,
					.detail = "package",
				});
			}
		} else {
			static const std::vector<std::string> caps = {
				"readonly",
				"readwrite",
				"writeonly",
				"adds",
				"removes",
				"exclude",
				"include",
				"required",
			};

			bool is_cap_line = false;
			for(auto const& cap : caps) {
				if(trimmed_line.starts_with(cap)) {
					is_cap_line = true;
					break;
				}
			}

			if(is_cap_line) {
				std::string filter_pkg;
				if(auto dot_pos = word_before.find_last_of('.'); dot_pos != std::string::npos) {
					filter_pkg = word_before.substr(0, dot_pos);
				}

				std::set<std::string> component_names;
				for(auto& [d_uri, d_state] : _documents) {
					std::string doc_pkg;
					for(auto& stack : d_state.parse_stacks) {
						if(stack.empty()) continue;
						if(stack.front().type == ECSACT_STATEMENT_PACKAGE) {
							doc_pkg = get_name_from_sv(stack.front().data.package_statement.package_name);
						}
						
						if(!filter_pkg.empty() && doc_pkg != filter_pkg) {
							continue;
						}

						auto& stmt = stack.back();
						if(stmt.type == ECSACT_STATEMENT_COMPONENT || stmt.type == ECSACT_STATEMENT_TRANSIENT) {
							auto name = (stmt.type == ECSACT_STATEMENT_COMPONENT)
								? get_name_from_sv(stmt.data.component_statement.component_name)
								: get_name_from_sv(stmt.data.transient_statement.transient_name);
							
							if(filter_pkg.empty()) {
								component_names.insert(name);
								if(!doc_pkg.empty()) {
									component_names.insert(doc_pkg + "." + name);
								}
							} else {
								component_names.insert(name);
							}
						}
					}
				}

				for(auto const& comp_name : component_names) {
					result.items.push_back({
						.label = comp_name,
						.kind = completion_item_kind::class_kind,
						.detail = "component",
					});
				}
			}
		}

		if(result.items.empty()) {
			return std::nullopt;
		}

		return result;
	}

	auto goto_definition(std::string uri, position pos)
		-> std::optional<location> {
		auto found = find_statement_at(uri, pos);
		if(!found) {
			return std::nullopt;
		}

		auto& [stack, r] = *found;
		auto& statement = stack.back();

		std::string symbol_name;

		if(statement.type == ECSACT_STATEMENT_IMPORT) {
			symbol_name = get_name_from_sv(statement.data.import_statement.import_package_name);
			for(auto& [doc_uri, doc_state] : _documents) {
				for(auto& d_stack : doc_state.parse_stacks) {
					if(d_stack.empty()) continue;
					auto& d_stmt = d_stack.front();
					if(d_stmt.type == ECSACT_STATEMENT_PACKAGE) {
						auto pkg_name = get_name_from_sv(d_stmt.data.package_statement.package_name);
						if(pkg_name == symbol_name) {
							return location{
								.uri = doc_uri,
								.range = get_source_range(doc_state.full_text, d_stmt.data.package_statement.package_name),
							};
						}
					}
				}
			}
			return std::nullopt;
		}

		if(statement.type == ECSACT_STATEMENT_SYSTEM_COMPONENT) {
			symbol_name = std::string(
				statement.data.system_component_statement.component_name.data,
				statement.data.system_component_statement.component_name.length
			);
		} else if(statement.type == ECSACT_STATEMENT_ENTITY_CONSTRAINT) {
			symbol_name = std::string(
				statement.data.entity_constraint_statement.constraint_component_name.data,
				statement.data.entity_constraint_statement.constraint_component_name.length
			);
		} else if(statement.type == ECSACT_STATEMENT_SYSTEM_NOTIFY_COMPONENT) {
			symbol_name = std::string(
				statement.data.system_notify_component_statement.component_name.data,
				statement.data.system_notify_component_statement.component_name.length
			);
		} else if(statement.type == ECSACT_STATEMENT_USER_TYPE_FIELD) {
			symbol_name = std::string(
				statement.data.user_type_field_statement.user_type_name.data,
				statement.data.user_type_field_statement.user_type_name.length
			);
		}

		if(symbol_name.empty()) {
			return std::nullopt;
		}

		auto last_dot = symbol_name.find_last_of('.');
		auto short_name = last_dot == std::string::npos
			? symbol_name
			: symbol_name.substr(last_dot + 1);

		for(auto& [doc_uri, doc_state] : _documents) {
			for(auto& d_stack : doc_state.parse_stacks) {
				if(d_stack.empty()) {
					continue;
				}
				auto& d_stmt = d_stack.back();
				std::string d_name;
				auto name_sv = ecsact_statement_sv{};
				if(d_stmt.type == ECSACT_STATEMENT_COMPONENT) {
					name_sv = d_stmt.data.component_statement.component_name;
				} else if(d_stmt.type == ECSACT_STATEMENT_TRANSIENT) {
					name_sv = d_stmt.data.transient_statement.transient_name;
				} else if(d_stmt.type == ECSACT_STATEMENT_SYSTEM) {
					name_sv = d_stmt.data.system_statement.system_name;
				} else if(d_stmt.type == ECSACT_STATEMENT_ACTION) {
					name_sv = d_stmt.data.action_statement.action_name;
				} else if(d_stmt.type == ECSACT_STATEMENT_ENUM) {
					name_sv = d_stmt.data.enum_statement.enum_name;
				}

				d_name = get_name_from_sv(name_sv);

				if(!d_name.empty() && d_name == short_name) {
					// If the symbol_name has a package prefix, check if this document is in that package
					if(last_dot != std::string::npos) {
						std::string prefix = symbol_name.substr(0, last_dot);
						bool        pkg_match = false;
						for(auto& d_stack2 : doc_state.parse_stacks) {
							if(d_stack2.empty()) continue;
							auto& d_stmt2 = d_stack2.front();
							if(d_stmt2.type == ECSACT_STATEMENT_PACKAGE) {
								if(get_name_from_sv(d_stmt2.data.package_statement.package_name) == prefix) {
									pkg_match = true;
									break;
								}
							}
						}
						if(!pkg_match) continue;
					}

					return location{
						.uri = doc_uri,
						.range = get_source_range(doc_state.full_text, name_sv),
					};
				}
			}
		}

		return std::nullopt;
	}
};

template<send_interface Send>
workspace_manager(Send&&) -> workspace_manager<Send>;

} // namespace ecsact_lsp
