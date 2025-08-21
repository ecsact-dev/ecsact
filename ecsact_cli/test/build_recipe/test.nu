let dir = $env.FILE_PWD;
let recipe_file = $dir + '/test-recipe.yml';
let recipe_bundle_file = $dir + '/test-recipe.tar';
let temp_dir = $dir + '/tmp';

bazel run //ecsact_cli -- recipe-bundle $recipe_file -o $recipe_bundle_file -f text
# ecsact build $ecsact_file -r $recipe_file -o $output_file --temp_dir $temp_dir -f text
