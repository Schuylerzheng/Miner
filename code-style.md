# Miner coding style

This is supposed to be a guideline about the coding style I would ideally like to use. Some rules may be more important than others, it will say. You don't have to follow the guidelines, but you should at least keep them in mind.

## 1. Indents

Use 4 character tabs for indents. This makes every indent 1 character so it's easy to navigate, and you can customize it easily in .editorconfig. This is also so that people can't just inject hidden code by putting 1 million spaces to the right and putting malware there.

Don't put multiple things in 1 line.

```cpp
if (Yes()) test(); test1(); bruh();
```

If an if loop only does 1 thing, put it like this:

```cpp
if (Statement()) { DoThis(); }
```

That's about it for now
