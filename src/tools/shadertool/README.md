# Purpose

This tool parses GLSL shader files (mainly *.vert, *.frag) and generates C++ source files for them.

# Description

The generator uses `ShaderTemplate.h.in` and `UniformBufferTemplate.h.in` as a base to generat the C++ source files.

There are several variables in the template file that are replaced by the generator.

* `$includes$`
* `$namespace$`
* `$name$`

* `$setters$`
* `$attributes$`
* `$uniforms$`
* `$uniformarrayinfo$`

* `$shutdown$`
* `$uniformbuffers$`

The parser includes a preprocessor.
