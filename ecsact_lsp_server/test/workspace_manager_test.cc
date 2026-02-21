#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <functional>
#include <optional>
#include <iostream>
#include "ecsact_lsp_server/details/workspace_manager.hh"

struct mock_sender {
	void send_request(
		std::string                         method,
		nlohmann::json                      params,
		std::function<void(nlohmann::json)> callback
	) {
	}

	void send_notification(std::string method, nlohmann::json params) {
		if(method == "textDocument/publishDiagnostics") {
			all_diagnostics.push_back(params);
			last_diagnostics = params;
		}
	}

	void show_message(ecsact_lsp::message_type type, std::string message) {
	}

	void show_message(
		ecsact_lsp::message_type                                            type,
		std::string                                                         message,
		std::vector<ecsact_lsp::message_action_item>                        actions,
		std::function<void(std::optional<ecsact_lsp::message_action_item>)> callback
	) {
	}

	void log_message(ecsact_lsp::message_type type, std::string message) {
		last_log_message = message;
	}

	void trace_log(std::string message) {
		if(trace == ecsact_lsp::trace_value::verbose) {
			log_message(ecsact_lsp::message_type::log, message);
		}
	}

	nlohmann::json              last_diagnostics;
	std::vector<nlohmann::json> all_diagnostics;
	std::string                 last_log_message;

	ecsact_lsp::trace_value trace = ecsact_lsp::trace_value::off;
};

static void expect_no_errors(const nlohmann::json& publish_diagnostics) {
	if(publish_diagnostics.is_null() ||
		 !publish_diagnostics.contains("diagnostics")) {
		return;
	}
	auto diagnostics = publish_diagnostics["diagnostics"];
	for(auto& diag : diagnostics) {
		EXPECT_NE(diag["severity"], 1) << "Error: " << diag["message"];
	}
}

TEST(WorkspaceManager, ClusterCrashRepro) {
	mock_sender                   sender;
	ecsact_lsp::workspace_manager manager(std::move(sender));

	std::string uri = "file:///test.ecsact";
	std::string text = R"(
main package test;
cluster {
	system Foo;
}
)";

	manager.add_document(uri, 1, text);
	expect_no_errors(sender.last_diagnostics);
}

TEST(WorkspaceManager, EmptyCluster) {
	mock_sender                   sender;
	ecsact_lsp::workspace_manager manager(std::move(sender));

	std::string uri = "file:///test_empty.ecsact";
	std::string text = R"(
main package test_empty;
cluster {}
)";

	manager.add_document(uri, 1, text);
	expect_no_errors(sender.last_diagnostics);
}

TEST(WorkspaceManager, MultipleClusters) {
	mock_sender                   sender;
	ecsact_lsp::workspace_manager manager(std::move(sender));

	std::string uri = "file:///test_multiple.ecsact";
	std::string text = R"(
main package test_multiple;
cluster A {}
cluster B {}
)";

	manager.add_document(uri, 1, text);
	expect_no_errors(sender.last_diagnostics);
}

TEST(WorkspaceManager, InvalidClusterContext) {
	mock_sender                   sender;
	ecsact_lsp::workspace_manager manager(std::move(sender));

	std::string uri = "file:///test_invalid_ctx.ecsact";
	std::string text = R"(
main package test_invalid_ctx;
component Foo {
	cluster Bar {}
}
)";

	manager.add_document(uri, 1, text);
	// We EXPECT an error here
	auto diagnostics = sender.last_diagnostics["diagnostics"];
	bool found_error = false;
	for(auto& diag : diagnostics) {
		if(diag["severity"] == 1) {
			found_error = true;
		}
	}
	EXPECT_TRUE(found_error) << "Expected an error for invalid cluster context";
}

TEST(WorkspaceManager, SyntaxErrorInCluster) {
	mock_sender                   sender;
	ecsact_lsp::workspace_manager manager(std::move(sender));

	std::string uri = "file:///test_syntax_err.ecsact";
	std::string text = R"(
main package test_syntax_err;
cluster {
	asdf
}
)";

	manager.add_document(uri, 1, text);
	// We EXPECT an error here
	auto diagnostics = sender.last_diagnostics["diagnostics"];
	bool found_error = false;
	for(auto& diag : diagnostics) {
		if(diag["severity"] == 1) {
			found_error = true;
		}
	}
	EXPECT_TRUE(found_error) << "Expected an error for syntax error in cluster";
}

TEST(WorkspaceManager, StackManagement) {
	mock_sender                   sender;
	ecsact_lsp::workspace_manager manager(std::move(sender));

	std::string uri = "file:///test_stack.ecsact";
	std::string text = R"(
main package test_stack;
cluster A {}
component C {}
)";

	manager.add_document(uri, 1, text);
	expect_no_errors(sender.last_diagnostics);
}

TEST(WorkspaceManager, ActionNoCapabilities) {
	mock_sender                   sender;
	ecsact_lsp::workspace_manager manager(std::move(sender));

	std::string uri = "file:///action_no_caps.ecsact";
	std::string text = R"(
main package action_no_caps;
action Foo {}
)";

	manager.add_document(uri, 1, text);
	bool found_error = false;
	for(auto const& publish_diagnostics : sender.all_diagnostics) {
		auto diagnostics = publish_diagnostics["diagnostics"];
		for(auto& diag : diagnostics) {
			if(diag["message"] == "Action must have at least one capability") {
				found_error = true;
			}
		}
	}
	EXPECT_TRUE(found_error) << "Expected error for action without capabilities";
}

TEST(WorkspaceManager, ClusterConflict) {
	mock_sender sender;
	sender.trace = ecsact_lsp::trace_value::verbose;
	ecsact_lsp::workspace_manager manager(std::move(sender));

	std::string uri = "file:///cluster_conflict.ecsact";
	std::string text = R"(
main package cluster_conflict;
component CompA { i32 a; }
cluster {
	system SysA { readonly CompA; }
	system SysB { readwrite CompA; }
}
)";

	manager.add_document(uri, 1, text);
	bool found_error = false;
	for(auto const& publish_diagnostics : sender.all_diagnostics) {
		auto diagnostics = publish_diagnostics["diagnostics"];
		for(auto& diag : diagnostics) {
			if(diag["message"].get<std::string>().find(
					 "cannot be part of the same explicit cluster"
				 ) != std::string::npos) {
				found_error = true;
			}
		}
	}
	EXPECT_TRUE(found_error) << "Expected error for cluster conflict";
}

TEST(WorkspaceManager, GotoDefinition) {
	mock_sender                   sender;
	ecsact_lsp::workspace_manager manager(std::move(sender));

	std::string uri = "file:///test.ecsact";
	std::string text = R"(
main package test;

component CompA {
	i32 a;
}

system SysA {
	readonly CompA;
}

action ActA {
	readwrite CompA;
}

transient TransA {
	i32 b;
}

system SysB {
	readonly TransA;
	generates {
		required CompA;
	}
}
)";

	manager.add_document(uri, 1, text);

	// Goto definition for CompA in SysA
	// readonly CompA; is at line 8 (0-indexed)
	auto result = manager.goto_definition(uri, {8, 12});
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result->uri, uri);
	EXPECT_EQ(result->range.start.line, 3); // component CompA { is at line 3

	// Goto definition for CompA in ActA
	// readwrite CompA; is at line 12
	result = manager.goto_definition(uri, {12, 12});
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result->uri, uri);
	EXPECT_EQ(result->range.start.line, 3);

	// Goto definition for TransA in SysB
	// readonly TransA; is at line 20
	result = manager.goto_definition(uri, {20, 12});
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result->uri, uri);
	EXPECT_EQ(result->range.start.line, 15); // transient TransA is at line 15

	// Goto definition for CompA in SysB generates block
	// required CompA; is at line 22
	result = manager.goto_definition(uri, {22, 12});
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result->uri, uri);
	EXPECT_EQ(result->range.start.line, 3);
}

TEST(WorkspaceManager, GotoDefinitionMultiFile) {
	mock_sender                   sender;
	ecsact_lsp::workspace_manager manager(std::move(sender));

	std::string uri_a = "file:///a.ecsact";
	std::string text_a = R"(
package a;
component CompA { i32 a; }
)";

	std::string uri_b = "file:///b.ecsact";
	std::string text_b = R"(
package b;
import a;
system SysB {
	readonly a.CompA;
}
)";

	manager.add_document(uri_a, 1, text_a);
	manager.add_document(uri_b, 1, text_b);

	// Goto definition for a.CompA in SysB
	// readonly a.CompA; is at line 4
	auto result = manager.goto_definition(uri_b, {4, 12});
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result->uri, uri_a);
	EXPECT_EQ(result->range.start.line, 2); // component CompA is at line 2
}

TEST(WorkspaceManager, Hover) {
	mock_sender                   sender;
	ecsact_lsp::workspace_manager manager(std::move(sender));

	std::string uri = "file:///test.ecsact";
	std::string text = R"(
package test;

component CompA {
	i32 a;
}

system SysA {
	readonly CompA;
}
)";

	manager.add_document(uri, 1, text);

	// Hover over CompA definition
	// line 3 (0-indexed)
	auto result = manager.get_hover(uri, {3, 12});
	ASSERT_TRUE(result.has_value());
	ASSERT_TRUE(
		result->contents.value.find("component `test.CompA`") != std::string::npos
	) << result->contents.value;

	// Hover over field a
	result = manager.get_hover(uri, {4, 5});
	ASSERT_TRUE(result.has_value());
	ASSERT_TRUE(
		result->contents.value.find("field `test.CompA.a`") != std::string::npos
	) << result->contents.value;
	ASSERT_TRUE(result->contents.value.find("type: `i32`") != std::string::npos)
		<< result->contents.value;

	// Hover over SysA definition
	result = manager.get_hover(uri, {7, 8});
	ASSERT_TRUE(result.has_value());
	ASSERT_TRUE(result->contents.value.find("system") != std::string::npos)
		<< result->contents.value;
	ASSERT_TRUE(result->contents.value.find("test.SysA") != std::string::npos)
		<< result->contents.value;
	ASSERT_TRUE(
		result->contents.value.find("**Execution Batch 0 systems:**") !=
		std::string::npos
	) << result->contents.value;
	ASSERT_TRUE(
		result->contents.value.find("- **`test.SysA`** (this)") != std::string::npos
	) << result->contents.value;

	// Hover over CompA in SysA (system capability)
	result = manager.get_hover(uri, {8, 12});
	ASSERT_TRUE(result.has_value());
	// It should show info for CompA
	ASSERT_TRUE(
		result->contents.value.find("component `test.CompA`") != std::string::npos
	) << result->contents.value;
}

TEST(WorkspaceManager, HoverMultiFile) {
	mock_sender                   sender;
	ecsact_lsp::workspace_manager manager(std::move(sender));

	std::string uri_a = "file:///a.ecsact";
	std::string text_a = R"(
package a;
component CompA { i32 a; }
)";

	std::string uri_b = "file:///b.ecsact";
	std::string text_b = R"(
package b;
import a;
system SysB {
	readonly a.CompA;
}
)";

	manager.add_document(uri_a, 1, text_a);
	manager.add_document(uri_b, 1, text_b);

	// Hover over a.CompA in SysB
	// readonly a.CompA; is at line 4
	auto result = manager.get_hover(uri_b, {4, 12});
	ASSERT_TRUE(result.has_value());
	ASSERT_TRUE(
		result->contents.value.find("component `a.CompA`") != std::string::npos
	) << result->contents.value;
}

TEST(WorkspaceManager, GotoDefinitionImport) {
	mock_sender                   sender;
	ecsact_lsp::workspace_manager manager(std::move(sender));

	std::string uri_a = "file:///a.ecsact";
	std::string text_a = R"(
package a;
component CompA { i32 a; }
)";

	std::string uri_b = "file:///b.ecsact";
	std::string text_b = R"(
package b;
import a;
system SysB {
	readonly a.CompA;
}
)";

	manager.add_document(uri_a, 1, text_a);
	manager.add_document(uri_b, 1, text_b);

	// Goto definition for 'import a;'
	// import a; is at line 2 (0-indexed)
	auto result = manager.goto_definition(uri_b, {2, 8});
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result->uri, uri_a);
	EXPECT_EQ(result->range.start.line, 1); // package a; is at line 1
}

TEST(WorkspaceManager, Completion) {
	mock_sender                   sender;
	ecsact_lsp::workspace_manager manager(std::move(sender));

	std::string uri_a = "file:///a.ecsact";
	std::string text_a = R"(
package package_a;
component CompA { i32 a; }
)";

	std::string uri_b = "file:///b.ecsact";
	std::string text_b = R"(
package package_b;
import 
system SysB {
	readonly 
}
)";

	manager.add_document(uri_a, 1, text_a);
	manager.add_document(uri_b, 1, text_b);

	// Completion for 'import '
	// import is at line 2
	auto result = manager.get_completions(uri_b, {2, 7});
	ASSERT_TRUE(result.has_value());
	bool found_package_a = false;
	for(auto const& item : result->items) {
		if(item.label == "package_a") found_package_a = true;
	}
	EXPECT_TRUE(found_package_a);

	// Completion for 'readonly '
	// readonly is at line 4
	result = manager.get_completions(uri_b, {4, 10});
	ASSERT_TRUE(result.has_value());
	bool found_comp_a = false;
	bool found_full_comp_a = false;
	for(auto const& item : result->items) {
		if(item.label == "CompA") found_comp_a = true;
		if(item.label == "package_a.CompA") found_full_comp_a = true;
	}
	EXPECT_TRUE(found_comp_a);
	EXPECT_TRUE(found_full_comp_a);
}

TEST(WorkspaceManager, DotCompletion) {
	mock_sender                   sender;
	ecsact_lsp::workspace_manager manager(std::move(sender));

	std::string uri_a = "file:///a.ecsact";
	std::string text_a = R"(
package package_a;
component CompA { i32 a; }
)";

	std::string uri_b = "file:///b.ecsact";
	std::string text_b = R"(
package package_b;
import package_a;
system SysB {
	readonly package_a.
}
)";

	manager.add_document(uri_a, 1, text_a);
	manager.add_document(uri_b, 1, text_b);

	// Completion for 'readonly package_a.'
	// line 4, character 20 (after the dot)
	auto result = manager.get_completions(uri_b, {4, 20});
	ASSERT_TRUE(result.has_value());
	bool found_comp_a = false;
	for(auto const& item : result->items) {
		if(item.label == "CompA") found_comp_a = true;
	}
	EXPECT_TRUE(found_comp_a);
}
