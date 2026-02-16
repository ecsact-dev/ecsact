cd $env.FILE_PWD;
let dirs = ls | find ecsact_ --no-highlight | each {|it| $it.name };
$dirs | par-each {|dir| clang-format -i ...(glob $"($dir)/**/*.h") | complete };
$dirs | par-each {|dir| clang-format -i ...(glob $"($dir)/**/*.hh") | complete };
$dirs | par-each {|dir| clang-format -i ...(glob $"($dir)/**/*.cc") | complete };
