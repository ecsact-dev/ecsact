#include "ecsact/runtime/meta.h"
#include "ecsact/runtime/dynamic.h"

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <utility>
#include <memory>
#include <stdexcept>
#include "parse-resolver-runtime/ids.hh"
#include "parse-resolver-runtime/lifecycle.hh"

using ecsact::interpret::details::event_ref;
using ecsact::interpret::details::gen_next_id;
using ecsact::interpret::details::on_destroy;

struct assoc_info {
	using cap_comp_list_t =
		std::vector<std::pair<ecsact_component_like_id, ecsact_system_capability>>;

	ecsact_system_assoc_id       id;
	ecsact_component_like_id     comp_id;
	std::vector<ecsact_field_id> assoc_fields;

	cap_comp_list_t caps;

	std::vector<event_ref> event_refs;

	assoc_info() = default;
	assoc_info(assoc_info&&) = default;
	~assoc_info() = default;
};

using system_assoc_map_info_t = std::unordered_map<
	ecsact_system_like_id,
	std::vector<std::shared_ptr<assoc_info>>>;

static system_assoc_map_info_t system_assoc_info{};

static auto get_system_assoc_list( //
	ecsact_system_like_id system_id
) -> std::vector<std::shared_ptr<assoc_info>>* {
	auto itr = system_assoc_info.find(system_id);
	if(itr != system_assoc_info.end()) {
		return &itr->second;
	}

	return nullptr;
}

static auto get_assoc_info( //
	ecsact_system_like_id  system_id,
	ecsact_system_assoc_id assoc_id
) -> std::shared_ptr<assoc_info> {
	auto list = get_system_assoc_list(system_id);
	if(!list) {
		return {};
	}

	auto itr = std::find_if( //
		list->begin(),
		list->end(),
		[=](const auto& item) -> bool { return item->id == assoc_id; }
	);

	if(itr == list->end()) {
		return {};
	}

	return *itr;
}

ecsact_system_assoc_id ecsact_add_system_assoc( //
	ecsact_system_like_id    system_id,
	ecsact_component_like_id component_id
) {
	auto itr = system_assoc_info.find(system_id);
	if(itr == system_assoc_info.end()) {
		itr = system_assoc_info.insert(system_assoc_info.end(), {system_id, {}});
	}

	auto& info = itr->second.emplace_back(std::make_shared<assoc_info>());
	info->id = gen_next_id<ecsact_system_assoc_id>();
	info->comp_id = component_id;

	info->event_refs
		.emplace_back(on_destroy(system_id, [system_id, assoc_id = info->id]() {
			ecsact_remove_system_assoc(system_id, assoc_id);
		}));

	info->event_refs
		.emplace_back(on_destroy(component_id, [system_id, assoc_id = info->id]() {
			ecsact_remove_system_assoc(system_id, assoc_id);
		}));

	return info->id;
}

void ecsact_remove_system_assoc( //
	ecsact_system_like_id  system_id,
	ecsact_system_assoc_id assoc_id
) {
	auto list = get_system_assoc_list(system_id);
	if(!list) {
		// No associations. Why did you try to remove? User error.
		return;
	}

	auto itr = std::find_if( //
		list->begin(),
		list->end(),
		[=](const auto& item) -> bool { return item->id == assoc_id; }
	);

	if(itr == list->end()) {
		// Unknown association id. User error.
		return;
	}

	list->erase(itr);
}

void ecsact_add_system_assoc_field(
	ecsact_system_like_id  system_id,
	ecsact_system_assoc_id assoc_id,
	ecsact_field_id        field_id
) {
	auto info = get_assoc_info(system_id, assoc_id);
	if(!info) {
		return;
	}

	auto itr = std::find( //
		info->assoc_fields.begin(),
		info->assoc_fields.end(),
		field_id
	);

	if(itr != info->assoc_fields.end()) {
		// Field already added. User error.
		return;
	}

	info->assoc_fields.push_back(field_id);
}

void ecsact_remove_system_assoc_field(
	ecsact_system_like_id  system_id,
	ecsact_system_assoc_id assoc_id,
	ecsact_field_id        field_id
) {
	auto info = get_assoc_info(system_id, assoc_id);
	if(!info) {
		return;
	}

	auto itr = std::find( //
		info->assoc_fields.begin(),
		info->assoc_fields.end(),
		field_id
	);

	if(itr == info->assoc_fields.end()) {
		// Field is already not associated. User error.
		return;
	}

	info->assoc_fields.erase(itr);
}

void ecsact_set_system_assoc_capability(
	ecsact_system_like_id    system_id,
	ecsact_system_assoc_id   assoc_id,
	ecsact_component_like_id comp_id,
	ecsact_system_capability cap
) {
	auto info = get_assoc_info(system_id, assoc_id);
	if(!info) {
		return;
	}

	auto itr = std::find_if(
		info->caps.begin(),
		info->caps.end(),
		[&](const auto& entry) -> bool { return entry.first == comp_id; }
	);

	if(cap == ECSACT_SYS_CAP_NONE) {
		if(itr != info->caps.end()) {
			info->caps.erase(itr);
		}
		return;
	}

	if(itr == info->caps.end()) {
		info->caps.emplace_back(comp_id, cap);
	} else {
		itr->second = cap;
	}
}

int32_t ecsact_meta_system_assoc_count( //
	ecsact_system_like_id system_id
) {
	auto list = get_system_assoc_list(system_id);
	if(list) {
		return list->size();
	}

	return 0;
}

void ecsact_meta_system_assoc_ids(
	ecsact_system_like_id   system_id,
	int32_t                 max_assoc_count,
	ecsact_system_assoc_id* out_assoc_ids,
	int32_t*                out_assoc_count
) {
	auto list = get_system_assoc_list(system_id);
	if(!list) {
		if(out_assoc_count) {
			*out_assoc_count = 0;
		}
		return;
	}

	if(out_assoc_count) {
		*out_assoc_count = list->size();
	}

	for(int32_t i = 0; max_assoc_count > i; ++i) {
		if(i >= list->size()) {
			break;
		}
		auto& assoc = list->at(i);
		out_assoc_ids[i] = assoc->id;
	}
}

ecsact_component_like_id ecsact_meta_system_assoc_component_id(
	ecsact_system_like_id  system_id,
	ecsact_system_assoc_id assoc_id
) {
	auto info = get_assoc_info(system_id, assoc_id);
	if(!info) {
		// Invalid assoc. User error.
		return ECSACT_INVALID_ID(component_like);
	}

	return info->comp_id;
}

int32_t ecsact_meta_system_assoc_fields_count(
	ecsact_system_like_id  system_id,
	ecsact_system_assoc_id assoc_id
) {
	auto info = get_assoc_info(system_id, assoc_id);
	if(!info) {
		return 0;
	}

	return static_cast<int>(info->assoc_fields.size());
}

void ecsact_meta_system_assoc_fields(
	ecsact_system_like_id  system_id,
	ecsact_system_assoc_id assoc_id,
	int32_t                max_fields_count,
	ecsact_field_id*       out_fields,
	int32_t*               out_fields_count
) {
	auto info = get_assoc_info(system_id, assoc_id);
	if(!info) {
		if(out_fields_count) {
			*out_fields_count = 0;
		}
		return;
	}

	if(out_fields_count) {
		*out_fields_count = info->assoc_fields.size();
	}

	for(int32_t i = 0; max_fields_count > i; ++i) {
		if(i >= info->assoc_fields.size()) {
			break;
		}
		out_fields[i] = info->assoc_fields[i];
	}
}

int32_t ecsact_meta_system_assoc_capabilities_count(
	ecsact_system_like_id  system_id,
	ecsact_system_assoc_id assoc_id
) {
	auto info = get_assoc_info(system_id, assoc_id);
	if(!info) {
		return 0;
	}

	return info->caps.size();
}

void ecsact_meta_system_assoc_capabilities(
	ecsact_system_like_id     system_id,
	ecsact_system_assoc_id    assoc_id,
	int32_t                   max_capabilities_count,
	ecsact_component_like_id* out_capability_component_ids,
	ecsact_system_capability* out_capabilities,
	int32_t*                  out_capabilities_count
) {
	auto info = get_assoc_info(system_id, assoc_id);
	if(!info) {
		if(out_capabilities_count) {
			*out_capabilities_count = 0;
		}
		return;
	}

	if(out_capabilities_count) {
		*out_capabilities_count = static_cast<int32_t>(info->caps.size());
	}

	for(int32_t i = 0; max_capabilities_count > i; ++i) {
		if(i >= info->caps.size()) {
			break;
		}

		out_capability_component_ids[i] = info->caps[i].first;
		out_capabilities[i] = info->caps[i].second;
	}
}

static auto get_legacy_assoc_info( //
	ecsact_system_like_id id
) -> std::shared_ptr<assoc_info> {
	auto list = get_system_assoc_list(id);
	if(!list || list->empty()) {
		return {};
	}
	// shouldn't be using the legacy API!
	if(list->size() > 1) {
		throw std::logic_error{"invalid use of legacy api"};
	}
	return list->at(0);
}

[[deprecated]]
int32_t ecsact_meta_system_association_fields_count(
	ecsact_system_like_id    system_id,
	ecsact_component_like_id component_id
) {
	auto info = get_legacy_assoc_info(system_id);
	if(component_id != info->comp_id) {
		// shouldn't be using the legacy API!
		throw std::logic_error{"(legacy api) invalid association component"};
	}

	return static_cast<int32_t>(info->assoc_fields.size());
}

[[deprecated]]
void ecsact_meta_system_association_fields(
	ecsact_system_like_id    system_id,
	ecsact_component_like_id component_id,
	int32_t                  max_fields_count,
	ecsact_field_id*         out_fields,
	int32_t*                 out_fields_count
) {
	auto info = get_legacy_assoc_info(system_id);
	if(component_id != info->comp_id) {
		// shouldn't be using the legacy API!
		throw std::logic_error{"(legacy api) invalid association component"};
	}
	ecsact_meta_system_assoc_fields(
		system_id,
		info->id,
		max_fields_count,
		out_fields,
		out_fields_count
	);
}

[[deprecated]]
int32_t ecsact_meta_system_association_capabilities_count(
	ecsact_system_like_id    system_id,
	ecsact_component_like_id component_id,
	ecsact_field_id          field_id
) {
	auto info = get_legacy_assoc_info(system_id);
	if(component_id != info->comp_id) {
		// shouldn't be using the legacy API!
		throw std::logic_error{"(legacy api) invalid association component"};
	}
	return ecsact_meta_system_assoc_capabilities_count(system_id, info->id);
}

[[deprecated]]
void ecsact_meta_system_association_capabilities(
	ecsact_system_like_id     system_id,
	ecsact_component_like_id  component_id,
	ecsact_field_id           field_id,
	int32_t                   max_capabilities_count,
	ecsact_component_like_id* out_capability_component_ids,
	ecsact_system_capability* out_capabilities,
	int32_t*                  out_capabilities_count
) {
	auto info = get_legacy_assoc_info(system_id);
	if(component_id != info->comp_id) {
		// shouldn't be using the legacy API!
		throw std::logic_error{"(legacy api) invalid association component"};
	}
	if(info->assoc_fields.size() != 1) {
		// shouldn't be using the legacy API!
		throw std::logic_error{
			"(legacy api) doesn't support multi field association"
		};
	}
	if(info->assoc_fields.at(0) != field_id) {
		// shouldn't be using the legacy API!
		throw std::logic_error{"(legacy api) invalid association field"};
	}

	ecsact_meta_system_assoc_capabilities(
		system_id,
		info->id,
		max_capabilities_count,
		out_capability_component_ids,
		out_capabilities,
		out_capabilities_count
	);
}
