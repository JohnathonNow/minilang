map
===

:mini:`type map < sequence`
   A map of key-value pairs.

   Keys can be of any type supporting hashing and comparison.

   Insert order is preserved.


:mini:`type mapnode`
   A node in a :mini:`map`.

   Dereferencing a :mini:`mapnode` returns the corresponding value from the :mini:`map`.

   Assigning to a :mini:`mapnode` updates the corresponding value in the :mini:`map`.


:mini:`meth map()`
   *TBD*

:mini:`meth map(Arg₁: names, ...)`
   *TBD*

:mini:`meth map(Sequence: sequence): map`
   Returns a map of all the key and value pairs produced by :mini:`Sequence`.


:mini:`meth :grow(Map: map, Sequence: sequence): map`
   Adds of all the key and value pairs produced by :mini:`Sequence` to :mini:`Map` and returns :mini:`Map`.


:mini:`meth :size(Map: map): integer`
   Returns the number of entries in :mini:`Map`.


:mini:`meth :count(Map: map): integer`
   Returns the number of entries in :mini:`Map`.


:mini:`meth (Map: map)[Key: any]: mapnode`
   Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then a new floating node is returned with value :mini:`nil`. This node will insert :mini:`Key` into :mini:`Map` if assigned.


:mini:`meth (Map: map)[Key: any, Default: function]: mapnode`
   Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then :mini:`Default(Key)` is called and the result inserted into :mini:`Map`.


:mini:`meth (Map: map) :: (Key: string): mapnode`
   Same as :mini:`Map[Key]`. This method allows maps to be used as modules.


:mini:`meth :insert(Map: map, Key: any, Value: any): any | nil`
   Inserts :mini:`Key` into :mini:`Map` with corresponding value :mini:`Value`.

   Returns the previous value associated with :mini:`Key` if any, otherwise :mini:`nil`.


:mini:`meth :delete(Map: map, Key: any): any | nil`
   Removes :mini:`Key` from :mini:`Map` and returns the corresponding value if any, otherwise :mini:`nil`.


:mini:`meth :missing(Map: map, Key: any): any | nil`
   Inserts :mini:`Key` into :mini:`Map` with corresponding value :mini:`Value`.

   Returns the previous value associated with :mini:`Key` if any, otherwise :mini:`nil`.


:mini:`meth :append(Arg₁: stringbuffer, Arg₂: map)`
   *TBD*

:mini:`meth (Map₁: map) + (Map₂: map): map`
   Returns a new map combining the entries of :mini:`Map₁` and :mini:`Map₂`.

   If the same key is in both :mini:`Map₁` and :mini:`Map₂` then the corresponding value from :mini:`Map₂` is chosen.


:mini:`meth (Map₁: map) * (Map₂: map): map`
   Returns a new map containing the entries of :mini:`Map₁` which are also in :mini:`Map₂`. The values are chosen from :mini:`Map₁`.


:mini:`meth (Map₁: map) / (Map₂: map): map`
   Returns a new map containing the entries of :mini:`Map₁` which are not in :mini:`Map₂`.


:mini:`meth string(Map: map): string`
   Returns a string containing the entries of :mini:`Map` surrounded by :mini:`"{"`, :mini:`"}"` with :mini:`" is "` between keys and values and :mini:`", "` between entries.


:mini:`meth string(Map: map, Seperator: string, Connector: string): string`
   Returns a string containing the entries of :mini:`Map` with :mini:`Connector` between keys and values and :mini:`Seperator` between entries.


:mini:`meth :sort(Map: map): Map`
   *TBD*

:mini:`meth :sort(Map: map, Compare: function): Map`
   *TBD*

:mini:`meth :sort2(Map: map, Compare: function): Map`
   *TBD*

