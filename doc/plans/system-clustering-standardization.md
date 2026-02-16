# System Clustering Standardization Plan

## Goals

- **Centralize Parallel Execution Analysis**: Move the logic that determines system execution batches from runtimes (like `rt_entt`) into the Ecsact core (`ecsact_interpret`).
- **Standardize Concurrency**: Ensure that all Ecsact runtimes exhibit consistent execution grouping and parallel behavior.
- **Expose via Core API**: Provide a standard way for runtimes and codegen plugins to query execution batches.
- **Enable Explicit `cluster` Syntax**: Provide the foundation for a future language feature that allows users to manually define parallel clusters.

## Implementation Strategy

### 1. Core Logic Migration
The existing parallel grouping logic in `ecsact_rt_entt/rt_entt_codegen/shared/parallel.cc` will be moved to `ecsact_interpret`. This logic analyzes system capabilities (read/write sets) to determine which systems can safely run in parallel.

### 2. Meta API Expansion
New functions will be added to the Ecsact Meta API (`ecsact/runtime/meta.h`) to expose the calculated clusters.

```c
/**
 * Count the number of execution batches (clusters) for a package.
 */
ECSACT_META_API_FN(int32_t, ecsact_meta_count_execution_batches)
(
	ecsact_package_id package_id
);

/**
 * Get the system-like IDs for a specific execution batch.
 */
ECSACT_META_API_FN(void, ecsact_meta_get_execution_batch)
(
	ecsact_package_id      package_id,
	int32_t                batch_index,
	int32_t                max_systems_count,
	ecsact_system_like_id* out_systems,
	int32_t*               out_systems_count
);
```

### 3. Interpreter Validation
The `ecsact_interpret` module will be updated to:
- Automatically calculate "Implicit Clusters" based on capability safety.
- Validate any "Explicit Clusters" defined in the source (once the keyword is added), ensuring they do not contain overlapping write capabilities.

### 4. Runtime & Codegen Refactoring
- **`ecsact_rt_entt_codegen`**: Will be simplified to remove its internal parallel analysis. It will instead iterate through the batches provided by `ecsact_meta_get_execution_batch` and generate the appropriate dispatch code (e.g., using `std::execution::par_unseq`).
- **Other Runtimes**: Runtimes like `ecsact_lang_csharp` or `ecsact_si_wasmer` can utilize the same batches to implement language-specific concurrency primitives (Tasks, WebWorker pools, etc.).

## Multi-Runtime Improvements

- **Determinism**: By centralizing the clustering logic, we guarantee that an Ecsact file will behave the same way regarding concurrency regardless of the runtime used.
- **Performance**: Future optimizations to the clustering algorithm (e.g., smarter resource partitioning) only need to be implemented once in the core.
- **Cross-Language Support**: New runtimes in different languages will get optimal parallel execution "for free" without having to reimplement complex capability analysis.
- **Simpler Codegen**: Reduces the maintenance burden on codegen plugins, as they no longer need to understand the nuances of Ecsact concurrency safety.
