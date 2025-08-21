#include "gtest/gtest.h"

#include <cstddef>
#include <array>
#include <filesystem>
#include <unordered_map>
#include <functional>
#include "ecsact/runtime/meta.hh"
#include "ecsact/interpret/eval.hh"
#include "bazel_sundry/runfiles.hh"

namespace fs = std::filesystem;

template<typename... CallbackPairs>
void for_each_field(
	ecsact_composite_id compo_id,
	CallbackPairs&&... callback_pairs
) {
	auto                         field_count = ecsact_meta_count_fields(compo_id);
	std::vector<ecsact_field_id> field_ids;
	field_ids.resize(field_count);
	ecsact_meta_get_field_ids(
		compo_id,
		field_ids.size(),
		field_ids.data(),
		&field_count
	);
	ASSERT_EQ(field_count, field_ids.size());

	using pairs_t =
		std::unordered_map<std::string, std::function<void(ecsact_field_id)>>;
	pairs_t pairs{std::forward<CallbackPairs>(callback_pairs)...};

	for(auto& field_id : field_ids) {
		std::string field_name = ecsact_meta_field_name(compo_id, field_id);
		ASSERT_TRUE(pairs.contains(field_name))
			<< "Missing test body for field '" << field_name << "'";
		pairs.at(field_name)(field_id);
	}

	ASSERT_EQ(field_ids.size(), pairs.size());
}

TEST(EcsactParseRuntimeInterop, Simple) {
	auto runfiles = bazel_sundry::CreateDefaultRunfiles();
	ASSERT_TRUE(runfiles);

	auto test_ecsact = runfiles->Rlocation("ecsact_interpret_test/test.ecsact");
	ASSERT_FALSE(test_ecsact.empty());
	ASSERT_TRUE(fs::exists(test_ecsact));

	auto errors = ecsact::eval_files({test_ecsact});
	for(auto& err : errors) {
		std::cerr //
			<< "[ERROR] test/test.ecsact:" << err.line << ":" << err.character << " "
			<< err.error_message << "\n";
	}
	ASSERT_TRUE(errors.empty());

	// Only 1 package
	ASSERT_EQ(ecsact_meta_count_packages(), 1);
	auto package_id = (ecsact_package_id)-1;
	ecsact_meta_get_package_ids(1, &package_id, nullptr);
	ASSERT_NE(package_id, (ecsact_package_id)-1);

	ASSERT_STREQ(ecsact_meta_package_name(package_id), "test");

	// Our test file only contains 1 component
	ASSERT_EQ(ecsact_meta_count_components(package_id), 1);

	auto comp_id = ecsact_invalid_component_id;
	ecsact_meta_get_component_ids(package_id, 1, &comp_id, nullptr);
	ASSERT_NE(comp_id, ecsact_invalid_component_id);
	auto compo_id = ecsact_id_cast<ecsact_composite_id>(comp_id);

	ASSERT_STREQ("TestComponent", ecsact_meta_component_name(comp_id));
	for_each_field(
		compo_id,
		std::make_pair(
			"num",
			[&](ecsact_field_id field_id) {
				auto type = ecsact_meta_field_type(compo_id, field_id);
				ASSERT_EQ(type.kind, ECSACT_TYPE_KIND_BUILTIN);
				ASSERT_EQ(type.type.builtin, ECSACT_I32);
				ASSERT_EQ(type.length, 1);
			}
		),
		std::make_pair(
			"test_entity",
			[&](ecsact_field_id field_id) {
				auto type = ecsact_meta_field_type(compo_id, field_id);
				ASSERT_EQ(type.kind, ECSACT_TYPE_KIND_BUILTIN);
				ASSERT_EQ(type.type.builtin, ECSACT_ENTITY_TYPE);
				ASSERT_EQ(type.length, 1);
			}
		)
	);

	// Here we're creating a function for each system in our test file to make
	// sure we have the expected capabilities
	using test_system_fns_t =
		std::unordered_map<std::string, std::function<void(ecsact_system_id)>>;
	test_system_fns_t test_system_fns{
		{"TestSystem",
		 [&](ecsact_system_id sys_id) {
			 auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(sys_id);

			 ASSERT_EQ(ecsact_meta_system_capabilities_count(sys_like_id), 1);

			 std::array sys_cap_comp_ids{(ecsact_component_like_id)-1};
			 std::array sys_caps{ecsact_system_capability{}};
			 ecsact_meta_system_capabilities(
				 sys_like_id,
				 1,
				 sys_cap_comp_ids.data(),
				 sys_caps.data(),
				 nullptr
			 );

			 ASSERT_EQ(
				 sys_cap_comp_ids[0],
				 ecsact_id_cast<ecsact_component_like_id>(comp_id)
			 );
			 ASSERT_EQ(sys_caps[0], ECSACT_SYS_CAP_READWRITE);

			 auto assoc_ids = ecsact::meta::system_assoc_ids(sys_like_id);
			 ASSERT_EQ(assoc_ids.size(), 1);
			 auto assoc_fields =
				 ecsact::meta::system_assoc_fields(sys_like_id, assoc_ids.at(0));
			 ASSERT_EQ(assoc_fields.size(), 1);
			 auto field_id = *assoc_fields.begin();
			 auto assoc_field_name = ecsact_meta_field_name(
				 ecsact_id_cast<ecsact_composite_id>(comp_id),
				 field_id
			 );
			 ASSERT_STREQ(assoc_field_name, "test_entity");

			 ASSERT_EQ(
				 ecsact_meta_get_system_parallel_execution(
					 ecsact_id_cast<ecsact_system_like_id>(sys_id)
				 ),
				 ECSACT_PAR_EXEC_AUTO
			 );
		 }},
		{"TestNestedSystem",
		 [&](ecsact_system_id sys_id) {
			 auto sys_like_id = ecsact_id_cast<ecsact_system_like_id>(sys_id);

			 ASSERT_EQ(ecsact_meta_system_capabilities_count(sys_like_id), 1);

			 std::array sys_cap_comp_ids{(ecsact_component_like_id)-1};
			 std::array sys_caps{ecsact_system_capability{}};
			 ecsact_meta_system_capabilities(
				 sys_like_id,
				 1,
				 sys_cap_comp_ids.data(),
				 sys_caps.data(),
				 nullptr
			 );

			 ASSERT_EQ(
				 sys_cap_comp_ids[0],
				 ecsact_id_cast<ecsact_component_like_id>(comp_id)
			 );
			 ASSERT_EQ(sys_caps[0], ECSACT_SYS_CAP_READWRITE);

			 ASSERT_EQ(
				 ecsact_meta_get_system_parallel_execution(
					 ecsact_id_cast<ecsact_system_like_id>(sys_id)
				 ),
				 ECSACT_PAR_EXEC_PREFERRED
			 );
		 }},
	};

	ASSERT_EQ(ecsact_meta_count_systems(package_id), test_system_fns.size());
	std::vector<ecsact_system_id> sys_ids;
	sys_ids.resize(test_system_fns.size());

	int32_t out_count = 0;
	ecsact_meta_get_system_ids(
		package_id,
		sys_ids.size(),
		sys_ids.data(),
		&out_count
	);
	ASSERT_EQ(sys_ids.size(), out_count);

	for(auto sys_id : sys_ids) {
		std::string sys_name = ecsact_meta_system_name(sys_id);
		ASSERT_TRUE(test_system_fns.contains(sys_name))
			<< "No test function for the '" << sys_name << "' system";
		test_system_fns.at(sys_name)(sys_id);
	}
}

// Similar to the struct generated by the ecsact c++ code generator
struct TestComponent {
	int32_t          num;
	ecsact_entity_id test_entity;
};

TEST(EcsactParseRuntimeInterop, FieldOffset) {
	auto package_id = (ecsact_package_id)-1;
	ecsact_meta_get_package_ids(1, &package_id, nullptr);
	ASSERT_NE(package_id, (ecsact_package_id)-1);
	auto comp_id = ecsact_invalid_component_id;
	ecsact_meta_get_component_ids(package_id, 1, &comp_id, nullptr);
	ASSERT_NE(comp_id, ecsact_invalid_component_id);
	auto compo_id = ecsact_id_cast<ecsact_composite_id>(comp_id);

	std::map<std::string, int32_t> field_offsets;

	for(auto field_id : ecsact::meta::get_field_ids(compo_id)) {
		std::string field_name = ecsact_meta_field_name(compo_id, field_id);
		field_offsets[field_name] = ecsact_meta_field_offset(compo_id, field_id);
	}

	ASSERT_EQ(field_offsets.at("num"), offsetof(TestComponent, num));
	ASSERT_EQ(
		field_offsets.at("test_entity"),
		offsetof(TestComponent, test_entity)
	);
}

TEST(EcsactParseRuntimeInterop, Enums) {
	auto package_id = (ecsact_package_id)-1;
	ecsact_meta_get_package_ids(1, &package_id, nullptr);

	auto enum_count = ecsact_meta_count_enums(package_id);
	ASSERT_EQ(enum_count, 1);

	ecsact_enum_id enum_id = {};
	ecsact_meta_get_enum_ids(package_id, 1, &enum_id, nullptr);

	auto enum_value_count = ecsact_meta_count_enum_values(enum_id);
	ASSERT_EQ(enum_value_count, 2);
}
