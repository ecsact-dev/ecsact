<!-- Generated with Stardoc: http://skydoc.bazel.build -->




<a id="ecsact_codegen"></a>

## ecsact_codegen

<pre>
ecsact_codegen(<a href="#ecsact_codegen-name">name</a>, <a href="#ecsact_codegen-plugins">plugins</a>, <a href="#ecsact_codegen-srcs">srcs</a>)
</pre>

Ecsact codegen rule. Executes `ecsact codegen` with specified plugins.

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="ecsact_codegen-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="ecsact_codegen-plugins"></a>plugins |  List of plugin to use for code generation. Default ones are available at <code>@ecsact//codegen_plugins:*</code>   | <a href="https://bazel.build/concepts/labels">List of labels</a> | required |  |
| <a id="ecsact_codegen-srcs"></a>srcs |  .ecsact source files   | <a href="https://bazel.build/concepts/labels">List of labels</a> | required |  |


<a id="ecsact_codegen_plugin"></a>

## ecsact_codegen_plugin

<pre>
ecsact_codegen_plugin(<a href="#ecsact_codegen_plugin-name">name</a>, <a href="#ecsact_codegen_plugin-output_extension">output_extension</a>, <a href="#ecsact_codegen_plugin-plugin">plugin</a>)
</pre>

Bazel info necessary for ecsact codegen plugin to be used with `ecsact_codegen`. Default plugins are available at `@ecsact//codegen_plugins:*`.

**ATTRIBUTES**


| Name  | Description | Type | Mandatory | Default |
| :------------- | :------------- | :------------- | :------------- | :------------- |
| <a id="ecsact_codegen_plugin-name"></a>name |  A unique name for this target.   | <a href="https://bazel.build/concepts/labels#target-names">Name</a> | required |  |
| <a id="ecsact_codegen_plugin-output_extension"></a>output_extension |  Plugin name. Also used as output file extension. This must match the extension specified by the plugin.   | String | required |  |
| <a id="ecsact_codegen_plugin-plugin"></a>plugin |  Path to plugin or name of builtin plugin.   | String | required |  |


