#include "ecsact/runtime/dynamic.h"

// This define is _always_ defined when using the Ecsact CLI build command
#ifndef ECSACT_BUILD
#	error "This test should only have been built through 'ecsact build'"
#endif

void ecsact_system_execution_context_get(struct ecsact_system_execution_context*, ecsact_component_like_id, void*, const void*) {
}

void ecsact_system_execution_context_action(struct ecsact_system_execution_context*, void*) {
}
