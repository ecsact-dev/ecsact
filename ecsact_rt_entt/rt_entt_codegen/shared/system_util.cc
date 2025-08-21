#include "system_util.hh"

#include <array>
#include <unordered_map>

#include "ecsact/runtime/common.h"

using capability_t =
	std::unordered_map<ecsact_component_like_id, ecsact_system_capability>;

auto ecsact::rt_entt_codegen::system_util::is_notify_system(
	ecsact_system_like_id system_id
) -> bool {
	auto count = ecsact::meta::system_notify_settings_count(system_id);

	if(count == 0) {
		return false;
	}

	auto notify_settings = ecsact::meta::system_notify_settings(system_id);

	for(const auto [comp_id, notify_setting] : notify_settings) {
		if(notify_setting == ECSACT_SYS_NOTIFY_ALWAYS ||
			 notify_setting == ECSACT_SYS_NOTIFY_NONE) {
			return false;
		}
	}

	return true;
}

auto ecsact::rt_entt_codegen::system_util::is_trivial_system(
	ecsact_system_like_id system_id
) -> bool {
	using ecsact::meta::get_field_ids;
	using ecsact::meta::system_capabilities;

	auto sys_capabilities = system_capabilities(system_id);

	auto non_trivial_capabilities = std::array{
		ECSACT_SYS_CAP_READONLY,
		ECSACT_SYS_CAP_READWRITE,
		ECSACT_SYS_CAP_WRITEONLY,
		ECSACT_SYS_CAP_OPTIONAL_READONLY,
		ECSACT_SYS_CAP_OPTIONAL_READWRITE,
		ECSACT_SYS_CAP_OPTIONAL_WRITEONLY,
		ECSACT_SYS_CAP_STREAM_TOGGLE,
	};

	bool has_non_tag_adds = false;
	bool has_read_write = false;
	for(auto&& [comp_id, sys_cap] : sys_capabilities) {
		if((ECSACT_SYS_CAP_ADDS & sys_cap) == ECSACT_SYS_CAP_ADDS) {
			auto field_count =
				ecsact_meta_count_fields(ecsact_id_cast<ecsact_composite_id>(comp_id));
			if(field_count > 0) {
				has_non_tag_adds = true;
			}
		}

		for(auto non_trivial_cap : non_trivial_capabilities) {
			if((non_trivial_cap & sys_cap) == sys_cap) {
				has_read_write = true;
			}
		}
	}
	if(has_non_tag_adds || has_read_write) {
		return false;
	}
	return true;
}

auto ecsact::rt_entt_codegen::system_util::get_unique_view_name()
	-> std::string {
	static int counter = 0;
	return "view" + std::to_string(counter++);
}
