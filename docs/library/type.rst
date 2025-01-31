type
====

:mini:`type any`
   Base type for all values.


:mini:`fun type(Value: any): type`
   Returns the type of :mini:`Value`.


:mini:`type type < function`
   Type of all types.

   Every type contains a set of named exports, which allows them to be used as modules.


:mini:`meth :rank(Type: type): integer`
   Returns the rank of :mini:`Type`, i.e. the depth of its inheritence tree.


:mini:`meth :exports(Type: type): map`
   Returns a map of all the exports from :mini:`Type`.


:mini:`meth :parents(Type: type): list`
   *TBD*

:mini:`meth (Type₁: type) | (Type₂: type): type`
   Returns a union interface of :mini:`Type₁` and :mini:`Type₂`.


:mini:`meth string(Type: type): string`
   Returns a string representing :mini:`Type`.


:mini:`meth :append(Arg₁: stringbuffer, Arg₂: type)`
   *TBD*

:mini:`meth (Type: type) :: (Name: string): any | error`
   Returns the value of :mini:`Name` exported from :mini:`Type`.

   Returns an error if :mini:`Name` is not present.

   This allows types to behave as modules.


:mini:`meth (Type₁: type) * (Type₂: type): type`
   Returns the closest common parent type of :mini:`Type₁` and :mini:`Type₂`.


:mini:`meth (Type₁: type) < (Type₂: type): type or nil`
   Returns :mini:`Type₂` if :mini:`Type₂` is a strict parent of :mini:`Type₁`, otherwise returns :mini:`nil`.


:mini:`meth (Type₁: type) <= (Type₂: type): type or nil`
   Returns :mini:`Type₂` if :mini:`Type₂` is a parent of :mini:`Type₁`, otherwise returns :mini:`nil`.


:mini:`meth (Type₁: type) > (Type₂: type): type or nil`
   Returns :mini:`Type₂` if :mini:`Type₂` is a strict sub-type of :mini:`Type₁`, otherwise returns :mini:`nil`.


:mini:`meth (Type₁: type) >= (Type₂: type): type or nil`
   Returns :mini:`Type₂` if :mini:`Type₂` is a sub-type of :mini:`Type₁`, otherwise returns :mini:`nil`.


:mini:`meth (Base: type)[Type₁,...,Typeₙ: type]: type`
   Returns the generic type :mini:`Base[Type₁, ..., Typeₙ]`.


:mini:`meth :in(Value: any, Type: type): Value | nil`
   Returns :mini:`Value` if it is an instance of :mini:`Type` or a type that inherits from :mini:`Type` and :mini:`nil` otherwise.


:mini:`meth :trace(Arg₁: any)`
   *TBD*

:mini:`meth (Value₁: any) <> (Value₂: any): integer`
   Compares :mini:`Value₁` and :mini:`Value₂` and returns :mini:`-1`, :mini:`0` or :mini:`1`.

   This comparison is based on the internal addresses of :mini:`Value₁` and :mini:`Value₂` and thus only has no persistent meaning.


:mini:`meth #(Value: any): integer`
   Returns a hash for :mini:`Value` for use in lookup tables, etc.


:mini:`meth (Value₁: any) = (Value₂: any): Value₂ | nil`
   Returns :mini:`Value₂` if :mini:`Value₁` and :mini:`Value₂` are exactly the same instance and :mini:`nil` otherwise.


:mini:`meth (Value₁: any) != (Value₂: any): Value₂ | nil`
   Returns :mini:`Value₂` if :mini:`Value₁` and :mini:`Value₂` are not exactly the same instance and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) = (Arg₂: any, Arg₃: any): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) != (Arg₂: any, Arg₃: any): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) < (Arg₂: any, Arg₃: any): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) <= (Arg₂: any, Arg₃: any): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) > (Arg₂: any, Arg₃: any): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth (Arg₁: any) >= (Arg₂: any, Arg₃: any): any | nil`
   Returns :mini:`Arg₂` if :mini:`Arg₁ SYMBOL Arg₂` and :mini:`nil` otherwise.


:mini:`meth string(Value: any): string`
   Returns a general (type name only) representation of :mini:`Value` as a string.


