#include "ecsact/runtime/common.h"

ECSACT_IMPORT("mod", "minst_test_import_fn")
auto minst_test_import_fn() -> void;

ECSACT_EXPORT("minst_test_export_fn") auto minst_test_export_fn() -> void {
	minst_test_import_fn();
}
