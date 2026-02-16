# System Clustering Standardization Plan

## Goals

- **Centralize Parallel Execution Analysis**: Move the logic that determines system execution batches from runtimes (like `rt_entt`) into the Ecsact core (`ecsact_interpret`).
- **Standardize Concurrency**: Ensure that all Ecsact runtimes exhibit consistent execution grouping and parallel behavior.
- **Expose via Core API**: Provide a standard way for runtimes and codegen plugins to query execution batches.
- **Enable Explicit `cluster` Syntax**: Provide the foundation for a future language feature that allows users to manually define parallel clusters.

## Implementation Strategy

### 1. Data Structure Definitions

**Parser (`ecsact_parse/ecsact/parse/statements.h`):**
```c
typedef struct {
	ecsact_statement_sv cluster_name; // Optional: cluster name
} ecsact_cluster_statement;

// Update ecsact_statement_data union
typedef union {
	// ... existing statements ...
	ecsact_cluster_statement cluster_statement;
} ecsact_statement_data;
```

**Interpreter Internal State:**
The interpreter should maintain a cache of calculated batches:
`std::map<ecsact_package_id, std::vector<std::vector<ecsact_system_like_id>>> cluster_cache;`

### 2. Core Logic Migration
The existing parallel grouping logic resides in `ecsact_rt_entt/rt_entt_codegen/shared/parallel.cc`. This must be ported to `ecsact_interpret/ecsact/interpret/eval.cc`.

**Capability Safety Bitmask:**
```cpp
// A capability is "exclusive" (writer) if any of these bits are set:
uint32_t exclusive_bits = 
    ECSACT_SYS_CAP_ADDS | 
    ECSACT_SYS_CAP_REMOVES | 
    ECSACT_SYS_CAP_WRITEONLY | 
    ECSACT_SYS_CAP_READWRITE | // Note: Added READWRITE to the exclusion set
    ECSACT_SYS_CAP_STREAM_TOGGLE;

// Filter out filter-only capabilities
exclusive_bits &= ~(ECSACT_SYS_CAP_EXCLUDE | ECSACT_SYS_CAP_INCLUDE);
```

**Clustering Algorithm (Batching):**
```cpp
for (auto sys_id : systems_in_execution_order) {
    bool can_join_current_batch = true;
    auto sys_caps = get_capabilities(sys_id);

    for (auto [comp_id, cap] : sys_caps) {
        bool is_writer = (cap & exclusive_bits) != 0;
        bool is_reader = (cap & (ECSACT_SYS_CAP_READONLY | ECSACT_SYS_CAP_READWRITE)) != 0;

        // Conflict 1: We want to write, but someone else is already reading or writing
        if (is_writer && (batch_readers.contains(comp_id) || batch_writers.contains(comp_id))) {
            can_join_current_batch = false;
            break;
        }
        // Conflict 2: We want to read, but someone else is already writing
        if (is_reader && batch_writers.contains(comp_id)) {
            can_join_current_batch = false;
            break;
        }
    }

    if (can_join_current_batch) {
        add_to_current_batch(sys_id);
        update_batch_sets(sys_caps);
    } else {
        finalize_current_batch();
        start_new_batch(sys_id);
    }
}
```

### 3. Meta API Expansion
New functions will be added to `ecsact/runtime/meta.h` and implemented in `ecsact_interpret`.

```c
ECSACT_META_API_FN(int32_t, ecsact_meta_count_execution_batches)(ecsact_package_id package_id);

ECSACT_META_API_FN(void, ecsact_meta_get_execution_batch)(
	ecsact_package_id      package_id,
	int32_t                batch_index,
	int32_t                max_systems_count,
	ecsact_system_like_id* out_systems,
	int32_t*               out_systems_count
);
```

### 4. Interpreter & Parser Updates
- **Grammar**: Add `cluster` production to `grammar.hh`. It should look like `system_statement` but with a scope `{}` containing other systems.
- **Evaluation**: In `eval.cc`, when `ECSACT_STATEMENT_CLUSTER` is evaluated, the interpreter must mark the start of an explicit batch. Validation must ensure that systems added inside this block do not conflict with each other.

### 5. Runtime & Codegen Refactoring
- **`rt_entt_codegen`**: In `execute_systems.cc`, replace the call to `parallel::get_parallel_execution_cluster` with `ecsact_meta_count_execution_batches` and a loop calling `ecsact_meta_get_execution_batch`.

## Multi-Runtime Improvements

- **Determinism**: Concurrency is now a first-class citizen of the Ecsact specification.
- **Performance**: Centralized optimization for all languages (C++, C#, Rust, JS).
- **Correctness**: User-defined `cluster` blocks are validated at compile-time/parse-time rather than causing runtime race conditions.

## Technical Reference for Future Context
- **Core Algorithm**: `ecsact_rt_entt/rt_entt_codegen/shared/parallel.cc` (`loop_iterator`)
- **Parser Production**: `ecsact_parse/ecsact/detail/grammar.hh`
- **Eval Switch**: `ecsact_interpret/ecsact/interpret/eval.cc`
- **Meta Definitions**: `ecsact_runtime/ecsact/runtime/meta.h`
- **Dynamic API**: `ecsact_runtime/ecsact/runtime/dynamic.h` (to add `ecsact_create_cluster`)
