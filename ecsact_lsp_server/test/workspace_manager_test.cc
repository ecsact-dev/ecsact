#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <functional>
#include <optional>
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

	nlohmann::json          last_diagnostics;
	std::string             last_log_message;
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

TEST(WorkspaceManager, ZCPXRepro) {
	mock_sender                   sender;
	ecsact_lsp::workspace_manager manager(std::move(sender));

	std::string uri = "file:///zcpx.ecsact";
	std::string text = R"(
main package zcpx;

component Spritesheet { u64 asset_id; }
component Player { i32 player_id; }
component Position { f32 x; f32 y; i8 layer; }
component Velocity { f32 x; f32 y; }
component MoveDirection { f32 dir_x; f32 dir_y; }
component FaceDirection { f32 dir_x; f32 dir_y; }
component Lifetime { f32 alive_time; }
component AnimState { u32 lifetime_frame; u32 frame; u64 anim_id; }

component Grounded;

action PlayerMove {
	i32 player_id;
	f32 dir_x;
	f32 dir_y;
	
	readonly Player;
	readwrite MoveDirection;
}

action PlayerJump {
	i32 player_id;
	
	readonly Player;
	readwrite Velocity;
	include Grounded;
}

component Attacking {
	f32 time_active;
}

component StopAttacking;

action PlayerAttack {
	i32 player_id;

	readonly Player;
	adds Attacking;
}

system LifetimeTracker {
	readwrite Lifetime;
}

system AttackTimer {
	readwrite Attacking;
	adds StopAttacking;
}

system AttackCleanup {
	removes Attacking;
	removes StopAttacking;
}

system Movement {
	readonly MoveDirection;
	readwrite Velocity;
	exclude Attacking;
}

system Facing {
	readonly MoveDirection;
	readwrite FaceDirection;
}

system ApplyGravity {
	readwrite Velocity;
	exclude Grounded;
}

system ApplyVelocity {
	readonly Velocity;
	readwrite Position;
}

system AirDetection {
	readonly Position;
	removes Grounded;
}

system GroundDetection {
	readwrite Position;
	readwrite Velocity;
	adds Grounded;
}

system GroundFriction {
	readwrite Velocity;
	include Grounded;
}

cluster AnimEval {
	system AnimEvalGeneric {
		readwrite AnimState;
		readonly Lifetime;
		exclude MoveDirection;
		exclude Attacking;
	}

	system AnimEvalMovement {
		include MoveDirection;
		readonly Position;
		readonly Velocity;
		readonly Lifetime;
		readwrite AnimState;
		exclude Attacking;
	}

	system AnimEvalAttacking {
		readonly Position;
		readonly Velocity;
		readonly Lifetime;
		readwrite AnimState;
		readonly Attacking;
	}
}

system DrawSpritesheet(parallel: false) {
	readonly FaceDirection;
	readonly Position;
	readonly Spritesheet;
	readonly AnimState;
}

system LocalPlayerTracker(parallel: false) {
	readonly Player;
	readonly Position;
}
)";

	manager.add_document(uri, 1, text);
}
