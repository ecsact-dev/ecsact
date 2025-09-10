let registry_dir = [$env.FILE_PWD, "..", "bazel", "registry"] | path join | path expand;
let modules_dir = [$registry_dir, "modules"] | path join;

bazel shutdown;

for mod in (ls $modules_dir) {
	let module_name = $mod.name | path basename;
	bzlreg calc-integrity $module_name --registry $registry_dir;
}
