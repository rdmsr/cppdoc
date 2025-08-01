# Usage
Projects using `cppdoc` must have a `cppdoc.toml` configuration file, see [CONFIG.md](CONFIG.md) for the available configuration options.

## Command-line options:
To build documentation for a project:

```
cppdoc build
```

To output JSON representing the codebase:

```
cppdoc build -d
```

More command-line options can be displayed using `-h` or `--help`.


## Comments
Comments are written using `///` or `///<`, the latter being used for inline documentation, as such:

```cpp
/// Documentation for 'MyEnum'
enum MyEnum {
	A ///< Documentation for 'A' 
}
```

Comments content are parsed as cppdoc-flavored markdown, there is no support for `javadoc`/Doxygen-style comments.

### Hiding nodes

Nodes can be hidden by annotating them with `/// #[doc(hidden)]`

```cpp
/// #[doc(hidden)]
struct Hidden {}
```

## Markdown syntax
`cppdoc` introduces a few extensions to markdown.

- To link documentation objects, one must prefix the link path with `::`. For example:

```
See [MyStruct](::MyStruct)
```

- Mermaid graphs can be displayed using `mermaid` codeblocks:

```mermaid
graph TD;
    A-->B;
    A-->C;
    B-->D;
    C-->D;
```

## Documentation tests
`cppdoc` supports running documentation tests akin to `rustdoc`, these tests are written in `cpp` and `c++` codeblocks and help ensure that code examples are up-to-date with API usage.

Documentation codeblocks feature special syntax:
- Lines prefixed with `@` won't be displayed, but will be added to the source code:

```
@int a = 1;
int b = a + 2;
```

Will only display:

```
int b = a + 2;
```

But will be compiled fully, that is with `int a = 1` included.


- Similarly, `@include` allows for quiet inclusion of a header file:

```
@include "file.h"
int a = 1;
```

Will only display:

```
int a = 1;
```

Notice that documentation tests don't need a `main` function, this is because documentation tests run by default in `main`. To disable this behavior one must set the codeblock language to `nomain` instead of `c++` or `cpp`.


### Test framework
`cppdoc` includes a basic test framework for documentation tests, this is useful for testing that examples still run successfully.
The following macros are defined:

- `ASSERT` and `ASSERT_EQ`: test for truth and equality, respectively
- `ASSERT_FALSE` and `ASSERT_NE`: test for falsity and inequality, respectively
- `ASSERT_GT` and `ASSERT_LT`: test for greater than and less than, respectively
- `ASSERT_GE` and `ASSERT_LE`: test for greater than or equal and less than or equal, respectively

Therefore, it is possible to write code like so:

```c++
int a = 1;
ASSERT(a == 1);
```

The code above will compile and run successfully, but the following code will fail to run:

```c++
int a = 2;
ASSERT(a == 1);
```


# Styling
`cppdoc` expects a `style.css` file to be present at the root of the generated documentation. It is recommended to put the stylesheets in a static directory and set the `static` option in the configuration file.
