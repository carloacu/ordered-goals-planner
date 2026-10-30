# Union types


## What is a union type?

A union type, written `(either a b)`, is the type of a parameter or a fluent value that can be an entity
of type `a` or of type `b`. It is the standard PDDL `:typing` construct.

```
(:predicates
    (engages ?a - (either physical_agent social_agent) ?b - (either physical_agent social_agent))
)
```


## Naming

`(either a b)`, `(either b a)` and `(either a a b)` name the same type, created on the first lookup of any
of them and named `(either a b)`, with its members sorted. Entities and parameters compare their types by
pointer, so every lookup of a union must return the same type.

`(either a)` is `a` itself. A member that is not a declared type is refused.

A union can only follow a `-`. It cannot be declared in `:types`, so it is never the parent of a type.


## Subtyping

A union is a kind of a type when all its members are. Every member is a kind of the union, and so are
the subtypes of its members.

For example, with `robot user - physical_agent` and `social_agent`:

* `user` is an `(either user social_agent)`, and `robot` is not.
* `(either robot user)` is a `physical_agent`.
* `(either user social_agent)` is neither a `physical_agent` nor a `user`.


## Looking facts up

Facts are filed by the types of their arguments and by those types' parents. A union is never a parent,
so no fact is filed under one. A lookup with a union-typed argument is therefore expanded into one
lookup per member, and their results are merged.


## Thread safety

Like the rest of the library, union types are not thread safe. `SetOfTypes::nameToType` is const, but
it adds a union to the set on the first lookup.
