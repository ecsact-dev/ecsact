
#include "gtest/gtest.h"

#include "ecsact/runtime/meta.hh"
#include "ecsact/runtime/dynamic.h"
#include "test_lib.hh"

class StreamComponent : public testing::Test {
public:
	ecsact_package_id pkg_id;

protected:
	void SetUp() override {
		auto errs = ecsact_interpret_test_files({"stream_component.ecsact"});
		ASSERT_EQ(errs.size(), 0) //
			<< "Expected no errors. Instead got: " << errs[0].error_message << "\n";
		pkg_id = ecsact::meta::get_package_ids().at(0);
	}

	void TearDown() override {
		// ecsact_destroy_package(pkg_id);
	}
};

TEST_F(StreamComponent, SanityCheck) {
	auto comp_id = get_component_by_name(pkg_id, "MyNormieComponent");
	ASSERT_TRUE(comp_id);

	auto comp_like_id = ecsact_meta_component_type(
		ecsact_id_cast<ecsact_component_like_id>(*comp_id)
	);

	ASSERT_EQ(comp_like_id, ECSACT_COMPONENT_TYPE_NONE);
}

TEST_F(StreamComponent, HasStreamType) {
	auto comp_id = get_component_by_name(pkg_id, "MyStreamComponent");
	ASSERT_TRUE(comp_id);

	auto comp_type = ecsact_meta_component_type(
		ecsact_id_cast<ecsact_component_like_id>(*comp_id)
	);

	ASSERT_EQ(comp_type, ECSACT_COMPONENT_TYPE_STREAM);
}

TEST_F(StreamComponent, HasLazyStreamType) {
	auto comp_id = get_component_by_name(pkg_id, "MyLazyStreamComponent");
	ASSERT_TRUE(comp_id);

	auto comp_type = ecsact_meta_component_type(
		ecsact_id_cast<ecsact_component_like_id>(*comp_id)
	);

	ASSERT_EQ(comp_type, ECSACT_COMPONENT_TYPE_LAZY_STREAM);
}
