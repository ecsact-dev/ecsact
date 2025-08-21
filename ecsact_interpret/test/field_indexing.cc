#include "gtest/gtest.h"

#include "ecsact/runtime/meta.hh"
#include "ecsact/runtime/dynamic.h"
#include "test_lib.hh"

using ecsact::meta::get_field_type;

class FieldIndexing : public testing::Test {
public:
	ecsact_package_id pkg_id;

protected:
	void SetUp() override {
		auto errs = ecsact_interpret_test_files({"field_indexing.ecsact"});
		ASSERT_EQ(errs.size(), 0) //
			<< "Expected no errors. Instead got: " << errs[0].error_message << "\n";
		pkg_id = ecsact::meta::get_package_ids().at(0);
	}

	void TearDown() override {
		// ecsact_destroy_package(pkg_id);
	}
};

TEST_F(FieldIndexing, SingleFieldIndex) {
	auto example_comp = get_component_by_name(pkg_id, "Example");
	ASSERT_TRUE(example_comp);

	auto comp = get_component_by_name(pkg_id, "SingleFieldIndex");
	ASSERT_TRUE(comp);

	auto field = get_field_by_name(*comp, "hello");
	ASSERT_TRUE(field);

	auto field_type = get_field_type(*comp, *field);
	ASSERT_EQ(field_type.kind, ECSACT_TYPE_KIND_FIELD_INDEX);
	ASSERT_EQ(
		field_type.type.field_index.composite_id,
		ecsact_id_cast<ecsact_composite_id>(*example_comp)
	);
}

TEST_F(FieldIndexing, FieldIndexAction) {
	auto example_comp = get_component_by_name(pkg_id, "Example");
	ASSERT_TRUE(example_comp);

	auto act = get_action_by_name(pkg_id, "FieldIndexAction");
	ASSERT_TRUE(act);

	auto field = get_field_by_name(*act, "cool_field_name");
	ASSERT_TRUE(field);

	auto field_type = get_field_type(*act, *field);
	ASSERT_EQ(field_type.kind, ECSACT_TYPE_KIND_FIELD_INDEX);
	ASSERT_EQ(
		field_type.type.field_index.composite_id,
		ecsact_id_cast<ecsact_composite_id>(*example_comp)
	);
}

TEST_F(FieldIndexing, MutltiField) {
	auto multi_field_comp = get_component_by_name(pkg_id, "MultiField");
	ASSERT_TRUE(multi_field_comp);

	auto comp = get_component_by_name(pkg_id, "IndexedMultiField");
	ASSERT_TRUE(comp);

	auto field_x = get_field_by_name(*comp, "indexed_x");
	auto field_y = get_field_by_name(*comp, "indexed_y");
	ASSERT_TRUE(field_x);
	ASSERT_TRUE(field_y);

	auto field_x_type = get_field_type(*comp, *field_x);
	auto field_y_type = get_field_type(*comp, *field_x);

	ASSERT_EQ(field_x_type.kind, ECSACT_TYPE_KIND_FIELD_INDEX);
	ASSERT_EQ(
		field_x_type.type.field_index.composite_id,
		ecsact_id_cast<ecsact_composite_id>(*multi_field_comp)
	);
	ASSERT_EQ(field_y_type.kind, ECSACT_TYPE_KIND_FIELD_INDEX);
	ASSERT_EQ(
		field_y_type.type.field_index.composite_id,
		ecsact_id_cast<ecsact_composite_id>(*multi_field_comp)
	);
}

TEST_F(FieldIndexing, MultiFieldSystem) {
	auto comp = get_component_by_name(pkg_id, "IndexedMultiField");
	ASSERT_TRUE(comp);

	auto sys = get_system_by_name(pkg_id, "MultiFieldIndexSystem");
	ASSERT_TRUE(sys);

	auto assoc_ids = ecsact::meta::system_assoc_ids(*sys);
	ASSERT_EQ(assoc_ids.size(), 1);

	ASSERT_EQ(
		ecsact::meta::system_assoc_component_id(*sys, assoc_ids.at(0)),
		ecsact_id_cast<ecsact_component_like_id>(*comp)
	);
}
