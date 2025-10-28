# cppdoc 
`cppdoc` is a C++ documentation generator inspired by `rustdoc`

## Features
- **`rustdoc`-style documentation comments**, written in Markdown
- **Documentation tests**: run code blocks in your docs as actual tests 
- **`mdbook`-style hierarchy** for page structure and navigation
- **[Mermaid](https://mermaid.js.org)** support for graphs
- **Customizable themes** with user-supplied CSS and Sublime Text syntax highlighting themes
- **libclang-based parser** with support for records, enums, functions and namespaces
- **Fast**: `cppdoc` has been benchmarked to run around **4-5x faster** than Doxygen with clang-assisted parsing


## Motivation
`cppdoc` intends to bridge the gap between documentation pages and API
references. Unlike most documentation generators such as `Doxygen`
which solely focus on the generation of code references, `cppdoc` enables users to write structured, "book-style" documentation integrated directly with their codebase.


## Usage
See [USAGE.md](USAGE.md)

## Preview
There is a live demo available [here](https://rdmsr.github.io/cppdoc).
