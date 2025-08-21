
#include "gtest/gtest.h"

#include "ecsact/runtime/meta.hh"
#include "ecsact/runtime/dynamic.h"
#include "test_lib.hh"

using ecsact::meta::get_field_type;

class AssocCapabilities : public testing::Test {
public:
	ecsact_package_id pkg_id;

protected:
	void SetUp() override {
		auto errs = ecsact_interpret_test_files({"assoc_capabilities.ecsact"});
		ASSERT_EQ(errs.size(), 0) //
			<< "Expected no errors. Instead got: " << errs[0].error_message << "\n";
		pkg_id = ecsact::meta::get_package_ids().at(0);
	}

	void TearDown() override {
		// ecsact_destroy_package(pkg_id);
	}
};

TEST_F(AssocCapabilities, GeneralReads) {
	auto example_comp = get_component_by_name(pkg_id, "Example");
	ASSERT_TRUE(example_comp);

	auto comp = get_component_by_name(pkg_id, "AssocComp");
	ASSERT_TRUE(comp);

	auto sys = get_system_by_name(pkg_id, "ReadwriteSystem");
	auto assoc_ids = ecsact::meta::system_assoc_ids(*sys);
	ASSERT_EQ(assoc_ids.size(), 1);

	auto caps = ecsact::meta::system_assoc_capabilities(*sys, assoc_ids.at(0));
	ASSERT_EQ(caps.size(), 1);

	ASSERT_EQ(
		caps.at(0).first,
		ecsact_id_cast<ecsact_component_like_id>(*example_comp)
	);
	ASSERT_EQ(caps.at(0).second, ECSACT_SYS_CAP_READWRITE);
}

TEST_F(AssocCapabilities, RemovesExists) {
	auto example_comp = get_component_by_name(pkg_id, "Example");
	ASSERT_TRUE(example_comp);

	auto tag_comp = get_component_by_name(pkg_id, "MyTag");
	ASSERT_TRUE(tag_comp);

	auto comp = get_component_by_name(pkg_id, "AssocComp");
	ASSERT_TRUE(comp);

	auto sys = get_system_by_name(pkg_id, "RemovesSystem");
	auto assoc_ids = ecsact::meta::system_assoc_ids(*sys);
	ASSERT_EQ(assoc_ids.size(), 1);

	auto assoc_caps =
		ecsact::meta::system_assoc_capabilities(*sys, assoc_ids.at(0));

	// order isn't guaranteed
	for(auto&& [comp_id, caps] : assoc_caps) {
		if(comp_id == ecsact_id_cast<ecsact_component_like_id>(*tag_comp)) {
			ASSERT_EQ(caps, ECSACT_SYS_CAP_INCLUDE);
		} else if(comp_id ==
							ecsact_id_cast<ecsact_component_like_id>(*example_comp)) {
			ASSERT_EQ(caps, ECSACT_SYS_CAP_REMOVES);
		} else {
			ASSERT_TRUE(false) << "unknown comp id";
		}
	}
}
