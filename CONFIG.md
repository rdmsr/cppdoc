# Configuration options
The configuration file is a TOML file with the following sections:

## `project`
- `name`: Project name.
- `version`: Project version.

## `input`
- `glob`: Glob to use for finding documented files.
- `compiler_arguments`: List of arguments to pass to libclang while parsing code.


## `output`
- `static_dir`: Path to a directory containing static files that will be copied in the output directory, this is where `style.css` will typically be located.
- `path`: Path to the output directory.
- `root_namespace` (optional): Namespace to use as the root, this is useful for libraries that only globally expose one namespace and want the index to be based on that namespace.
- `base_url`: Base URL to prepend all paths with.

## `pages`
- `index` (optional): Markdown file to use as the index file, if an index page is not specified, the root namespace's comment will be used instead.
- `extra` (optional): List of file paths to serve as extra documentation pages.

## `doctests` (optional)
- `enable`: Whether to enable documentation tests or not.
- `run`: Whether to run documentation tests or not (if disabled, tests will only be compiled).
- `compiler_invocation`: Compiler invocation to use to compile documentation tests, this is represented as an array containing `argv`. The sentinel values `{file}` and `{out}` are replaced at runtime by the appropriate values.


# Example

```toml
[project]
name = "Example"
version = "0.1.0"

[input]
glob = "include/**/*.hpp"
compiler_arguments = ["-Iinclude", "-std=gnu++20", "-xc++"]

[pages]
index = "README.md"
extra = ["extra-page.md"]

[output]
static_dir = "static"
path = "docs"
base_url = "/cppdoc"

[doctests]
enable = false 
run = true
compiler_invocation = ["clang++", "{file}", "-o", "{out}", "-Iinclude", "-std=c++20"]
```

