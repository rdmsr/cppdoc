# cppdoc demo
This is a demo documentation README

READMEs get the code reference appended

```nomain
#include <iostream>

template<typename T>
concept A = requires(T a) {
    { a.func() } -> std::same_as<bool>;
};

int main()
{
    std::cout << "hello, world!" << std::endl;
    return 0;
}
```



[Test](::A)
