#pragma once

#include <iostream>
#include <vector>
#include <filesystem>
#include <optional>
#include <string>
#include "gtest/gtest.h"
#include "ecsact/interpret/eval.hh"
#include "ecsact/runtime/meta.hh"
#include "magic_enum.hpp"
#include "bazel_sundry/runfiles.hh"

inline auto get_component_by_name( //
	ecsact_package_id pkg_id,
	std::string       name
) -> std::optional<ecsact_component_id> {
	for(auto comp_id : ecsact::meta::get_component_ids(pkg_id)) {
		if(ecsact::meta::component_name(comp_id) == name) {
			return comp_id;
		}
	}

	return {};
}

inline auto get_system_by_name( //
	ecsact_package_id pkg_id,
	std::string       name
) -> std::optional<ecsact_system_id> {
	for(auto sys_id : ecsact::meta::get_system_ids(pkg_id)) {
		if(ecsact::meta::system_name(sys_id) == name) {
			return sys_id;
		}
	}

	return {};
}

inline auto get_action_by_name( //
	ecsact_package_id pkg_id,
	std::string       name
) -> std::optional<ecsact_action_id> {
	for(auto act_id : ecsact::meta::get_action_ids(pkg_id)) {
		if(ecsact::meta::action_name(act_id) == name) {
			return act_id;
		}
	}

	return {};
}

template<typename CompositeID>
inline auto get_field_by_name( //
	CompositeID id,
	std::string target_field_name
) -> std::optional<ecsact_field_id> {
	auto field_ids = ecsact::meta::get_field_ids(id);
	for(auto field_id : field_ids) {
		std::string field_name =
			ecsact_meta_field_name(ecsact_id_cast<ecsact_composite_id>(id), field_id);

		if(field_name == target_field_name) {
			return field_id;
		}
	}

	return {};
}

inline auto ecsact_interpret_test_files(
	std::vector<std::string> relative_file_paths
) -> std::vector<ecsact::parse_eval_error> {
	auto runfiles = bazel_sundry::CreateDefaultRunfiles();
	[&] { ASSERT_TRUE(runfiles); }();

	std::vector<std::filesystem::path> file_paths;
	file_paths.reserve(relative_file_paths.size());

	for(auto rel_p : relative_file_paths) {
		auto path_rloc = "ecsact_interpret_test/" + rel_p;
		auto path = runfiles->Rlocation(path_rloc);
		[&] {
			ASSERT_FALSE(path.empty()) << "couldn't find: " << path_rloc << "\n";
			ASSERT_TRUE(std::filesystem::exists(path)) << path << " does not exist\n";
		}();
		file_paths.emplace_back(path);
	}

	[&] { ASSERT_FALSE(file_paths.empty()) << "cannot interpret 0 files\n"; }();

	auto errs = ecsact::eval_files(file_paths);

	for(auto& err : errs) {
		std::cerr //
			<< "[ERROR] " << file_paths[err.source_index].generic_string() << ":"
			<< err.line << ":" << err.character << " " << err.error_message << "\n";
	}

	return errs;
}
