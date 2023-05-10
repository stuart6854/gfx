# Style Guide

## C++

## File Naming

- Use `.hpp` for header files
- Use `.cpp` for source files

### Naming Conventions

- Types (CamelCase): `SomeObjectType`
- Functions (snake_case): `func_to_do_something()`
- Variables:
    - Private Members (camelCase, Prefix: `m_`): `m_isDead`
    - Parameters (camelCase): `someParam`
    - Constants (SNAKE_CASE): `SOME_CONST`
    - Static

### Bracket Style

```c++
void func()
{
    if(true)
    {
        return;
    }
}
```

### Comments/Documentation

```c++
/**
 * @brief This function does something. Who knows?
 */
void some_func()
{
    // Comments should be like this - do not use /**/ for code comments
}

// Prefer #if 0 for disabling large blocks of code over /**/
#if 0
void disabled_func()
{
}
#endif
```

### Error Handling

```c++
// Use asserts for invalid input or state
// Log & return for failed functions
bool do_something(int x)
{
    ASSERT(m_someState != nullptr);
    if(!m_someState.contains(x))
    {
        LOG("This function failed!");
        return false;
    }
    
    return true;
}
```

### Well-Formed Example

```c++
#include "local_include.hpp"

#include <iostream>

class Rect
{
public:
    Rect() : m_width(0.0f), m_height(0.0f) {}
    explicit Rect(float width, float height) : m_width(width), m_height(height) {}
    ~Rect() = default;
    
private:
    float m_width;
    float m_height;
};
```