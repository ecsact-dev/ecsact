let dir = $env.FILE_PWD;
let outdir = $dir | path join 'codegen_test';

bazel build //ecsact_cli //ecsact_rt_entt/rt_entt_codegen;

let ecsact_cli = (bazel cquery //ecsact_cli --output=files | lines | get 0);
let ecsact_files = (ls *.ecsact | each {|it| $it.name});
let plugin = $dir | path join (bazel cquery //ecsact_rt_entt/rt_entt_codegen --output=files | lines | get 0);

^$ecsact_cli codegen ...$ecsact_files -p $plugin -o $outdir;
# dbg launch $ecsact_cli codegen ...$ecsact_files -p $plugin -o $outdir;
