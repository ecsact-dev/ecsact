#pragma once

#include <string>
#include <optional>
#include "nlohmann/json.hpp"

namespace ecsact_lsp {

enum class request_id : int;

enum class message_type {
	error = 1,
	warning = 2,
	info = 3,
	log = 4,
};

enum class lsp_error_code {
	parse_error = -32700,
	invalid_request = -32600,
	method_not_found = -32601,
	invalid_param = -32602,
	internal_error = -32603,
};

struct workspace_folder {
	std::string uri;
	std::string name;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(workspace_folder, uri, name);
};

struct message_action_item {
	std::string title;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(message_action_item, title);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocumentIdentifier
 */
struct text_document_identifier {
	std::string uri;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(text_document_identifier, uri);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#versionedTextDocumentIdentifier
 */
struct versioned_text_document_identifier {
	std::string uri;
	int         version;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(
		versioned_text_document_identifier,
		uri,
		version
	);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocumentItem
 */
struct text_document_item {
	std::string uri;
	std::string languageId;
	int         version;
	std::string text;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(
		text_document_item,
		uri,
		languageId,
		version,
		text
	);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textDocumentContentChangeEvent
 */
struct full_text_document_content_change_event {
	std::string text;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(full_text_document_content_change_event, text);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#position
 */
struct position {
	unsigned line;
	unsigned character;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(position, line, character);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#range
 */
struct range {
	position start;
	position end;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(range, start, end);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#location
 */
struct location {
	std::string       uri;
	ecsact_lsp::range range;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(location, uri, range);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#textEdit
 */
struct text_edit {
	range       range;
	std::string newText;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(text_edit, range, newText);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#workspaceEdit
 */
struct workspace_edit {
	std::optional<std::map<std::string, std::vector<text_edit>>> changes;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(workspace_edit, changes);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#referenceContext
 */
struct reference_context {
	bool includeDeclaration;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(reference_context, includeDeclaration);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#referenceParams
 */
struct reference_params {
	text_document_identifier textDocument;
	position                 position;
	reference_context        context;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(
		reference_params,
		textDocument,
		position,
		context
	);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#definitionParams
 */
struct definition_params {
	text_document_identifier textDocument;
	position                 position;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(definition_params, textDocument, position);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#markupKind
 */
enum class markup_kind {
	plaintext,
	markdown,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
	markup_kind,
	{
		{markup_kind::plaintext, "plaintext"},
		{markup_kind::markdown, "markdown"},
	}
)

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#markupContent
 */
struct markup_content {
	markup_kind kind;
	std::string value;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(markup_content, kind, value);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#hover
 */
struct hover {
	markup_content       contents;
	std::optional<range> range;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(hover, contents, range);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#hoverParams
 */
struct hover_params {
	text_document_identifier textDocument;
	position                 position;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(hover_params, textDocument, position);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#renameParams
 */
struct rename_params {
	text_document_identifier textDocument;
	position                 position;
	std::string              newName;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(
		rename_params,
		textDocument,
		position,
		newName
	);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#completionItemKind
 */
enum class completion_item_kind {
	text = 1,
	method = 2,
	function = 3,
	constructor = 4,
	field = 5,
	variable = 6,
	class_kind = 7,
	interface = 8,
	module = 9,
	property = 10,
	unit = 11,
	value = 12,
	enum_kind = 13,
	keyword = 14,
	snippet = 15,
	color = 16,
	file = 17,
	reference = 18,
	folder = 19,
	enum_member = 20,
	constant = 21,
	struct_kind = 22,
	event = 23,
	operator_kind = 24,
	type_parameter = 25,
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#completionItem
 */
struct completion_item {
	std::string          label;
	completion_item_kind kind;
	std::string          detail;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(completion_item, label, kind, detail);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#completionList
 */
struct completion_list {
	bool                         isIncomplete;
	std::vector<completion_item> items;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(completion_list, isIncomplete, items);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#completionParams
 */
struct completion_params {
	text_document_identifier textDocument;
	position                 position;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(completion_params, textDocument, position);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#symbolKind
 */
enum class symbol_kind {
	file = 1,
	module = 2,
	namespace_kind = 3,
	package = 4,
	class_kind = 5,
	method = 6,
	property = 7,
	field = 8,
	constructor = 9,
	enum_kind = 10,
	interface = 11,
	function = 12,
	variable = 13,
	constant = 14,
	string = 15,
	number = 16,
	boolean = 17,
	array = 18,
	object = 19,
	key = 20,
	null_kind = 21,
	enum_member = 22,
	struct_kind = 23,
	event = 24,
	operator_kind = 25,
	type_parameter = 26,
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#documentSymbolParams
 */
struct document_symbol_params {
	text_document_identifier textDocument;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(document_symbol_params, textDocument);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#documentSymbol
 */
struct document_symbol {
	std::string                  name;
	std::string                  detail;
	symbol_kind                  kind;
	range                        range;
	ecsact_lsp::range            selectionRange;
	std::vector<document_symbol> children;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(
		document_symbol,
		name,
		detail,
		kind,
		range,
		selectionRange,
		children
	);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#workspaceSymbolParams
 */
struct workspace_symbol_params {
	std::string query;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(workspace_symbol_params, query);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#symbolInformation
 */
struct symbol_information {
	std::string name;
	symbol_kind kind;
	location    location;
	std::string containerName;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(
		symbol_information,
		name,
		kind,
		location,
		containerName
	);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#command
 */
struct command {
	std::string                                title;
	std::string                                command_name;
	std::optional<std::vector<nlohmann::json>> arguments;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(command, title, command_name, arguments);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#codeLensParams
 */
struct code_lens_params {
	text_document_identifier textDocument;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(code_lens_params, textDocument);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#codeLens
 */
struct code_lens {
	range                         range;
	std::optional<command>        command;
	std::optional<nlohmann::json> data;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(code_lens, range, command, data);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#inlayHintParams
 */
struct inlay_hint_params {
	text_document_identifier textDocument;
	range                    range;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(inlay_hint_params, textDocument, range);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#inlayHint
 */
struct inlay_hint {
	position                      position;
	std::string                   label;
	std::optional<unsigned>       kind;
	std::optional<nlohmann::json> data;
	bool                          paddingLeft = false;
	bool                          paddingRight = false;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(
		inlay_hint,
		position,
		label,
		kind,
		data,
		paddingLeft,
		paddingRight
	);
};

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#diagnosticSeverity
 */
enum class diagnostic_severity {
	error = 1,
	warning = 2,
	information = 3,
	hint = 4,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
	diagnostic_severity,
	{
		{diagnostic_severity::error, 1},
		{diagnostic_severity::warning, 2},
		{diagnostic_severity::information, 3},
		{diagnostic_severity::hint, 4},
	}
)

enum class trace_value {
	off,
	messages,
	verbose,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
	trace_value,
	{
		{trace_value::off, "off"},
		{trace_value::messages, "messages"},
		{trace_value::verbose, "verbose"},
	}
)

/**
 * @SEE:
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/#diagnostic
 */
struct diagnostic {
	ecsact_lsp::range   range;
	diagnostic_severity severity;
	std::string         message;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(diagnostic, range, severity, message);
};

struct ecsact_symbol_params {
	text_document_identifier textDocument;
	position                 position;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ecsact_symbol_params, textDocument, position);
};

struct cpp_symbol_detail {
	std::string type;
	std::string implementation;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(cpp_symbol_detail, type, implementation);
};

struct ecsact_symbol_result {
	std::string       c;
	cpp_symbol_detail cpp;
	std::string       csharp;
	std::string       rust;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ecsact_symbol_result, c, cpp, csharp, rust);
};

} // namespace ecsact_lsp
