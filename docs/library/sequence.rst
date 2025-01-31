sequence
========

:mini:`type sequence`
   The base type for any sequence value.


:mini:`type filter < function`
   *TBD*

:mini:`fun filter(Function?: any): filter`
   Returns a filter for use in chained functions and iterators.


:mini:`type chained < function, sequence`
   *TBD*

:mini:`meth (Sequence: function) -> (Function: function): chained`
   *TBD*

:mini:`meth (Sequence: sequence) -> (Function: function): chained`
   *TBD*

:mini:`meth (Sequence: sequence) => (Function: function): chained`
   *TBD*

:mini:`meth (Sequence: sequence) => (Function: function, Arg₃: function): chained`
   *TBD*

:mini:`meth (Chained: chained) -> (Function: function): chained`
   *TBD*

:mini:`meth (Chained: chained) => (Function: function): chained`
   *TBD*

:mini:`meth (Chained: chained) => (Function: function, Arg₃: function): chained`
   *TBD*

:mini:`meth (Sequence: sequence) ->? (Function: function): chained`
   *TBD*

:mini:`meth (Sequence: sequence) =>? (Function: function): chained`
   *TBD*

:mini:`meth (Chained: chained) ->? (Function: function): chained`
   *TBD*

:mini:`meth (Chained: chained) =>? (Function: function): chained`
   *TBD*

:mini:`meth (Sequence: sequence) ^ (Function: function): sequence`
   Returns a new sequence that generates the keys and values from :mini:`Function(Value)` for each value generated by :mini:`Sequence`.


:mini:`fun all(Sequence: sequence): some | nil`
   Returns :mini:`nil` if :mini:`nil` is produced by :mini:`Sequence`. Otherwise returns :mini:`some`.


:mini:`fun first(Sequence: sequence): any | nil`
   Returns the first value produced by :mini:`Sequence`.


:mini:`fun first2(Sequence: sequence): tuple(any, any) | nil`
   Returns the first key and value produced by :mini:`Sequence`.


:mini:`fun last(Sequence: sequence): any | nil`
   Returns the last value produced by :mini:`Sequence`.


:mini:`fun last2(Sequence: sequence): tuple(any, any) | nil`
   Returns the last key and value produced by :mini:`Sequence`.


:mini:`meth :count(Sequence: sequence): integer`
   Returns the count of the values produced by :mini:`Sequence`.


:mini:`fun count2(Sequence: sequence): map`
   Returns a map of the values produced by :mini:`Sequence` with associated counts.


:mini:`fun reduce(Initial?: any, Sequence: sequence, Fn: function): any | nil`
   Returns :mini:`Fn(Fn( ... Fn(Initial, V₁), V₂) ..., Vₙ)` where :mini:`Vᵢ` are the values produced by :mini:`Sequence`.

   If :mini:`Initial` is omitted, first value produced by :mini:`Sequence` is used.


:mini:`fun reduce2(Initial: any, Sequence: sequence, Fn: function): any | nil`
   Returns :mini:`Fn(Fn( ... Fn(Initial, K₁, V₁), K₂, V₂) ..., Kₙ, Vₙ)` where :mini:`Kᵢ` and :mini:`Vᵢ` are the keys and values produced by :mini:`Sequence`.


:mini:`fun min(Sequence: sequence): any | nil`
   Returns the smallest value (using :mini:`<`) produced by :mini:`Sequence`.


:mini:`fun max(Sequence: sequence): any | nil`
   Returns the largest value (using :mini:`>`) produced by :mini:`Sequence`.


:mini:`fun sum(Sequence: sequence): any | nil`
   Returns the sum of the values (using :mini:`+`) produced by :mini:`Sequence`.


:mini:`fun prod(Sequence: sequence): any | nil`
   Returns the product of the values (using :mini:`*`) produced by :mini:`Sequence`.


:mini:`meth :join(Sequence: sequence, Separator: string): string`
   Joins the elements of :mini:`Sequence` into a string using :mini:`Separator` between elements.


:mini:`fun extremum(Sequence: sequence, Fn: function): tuple | nil`
   *TBD*

:mini:`fun min2(Sequence: sequence): tuple | nil`
   Returns a tuple with the key and value of the smallest value (using :mini:`<`) produced by :mini:`Sequence`.


:mini:`fun max2(Sequence: sequence): tuple | nil`
   Returns a tuple with the key and value of the largest value (using :mini:`>`) produced by :mini:`Sequence`.


:mini:`meth (Sequence: sequence) // (Fn: function): sequence`
   Returns an sequence that produces :mini:`V₁`, :mini:`Fn(V₁, V₂)`, :mini:`Fn(Fn(V₁, V₂), V₃)`, ... .


:mini:`meth (Sequence: sequence) // (Initial: any, Fn: function): sequence`
   Returns an sequence that produces :mini:`Initial`, :mini:`Fn(Initial, V₁)`, :mini:`Fn(Fn(Initial, V₁), V₂)`, ... .


:mini:`meth @(Value: any): sequence`
   Returns an sequence that repeatedly produces :mini:`Value`.


:mini:`meth (Value: any) @ (Update: function): sequence`
   Returns an sequence that repeatedly produces :mini:`Value`.

   :mini:`Value` is replaced with :mini:`Update(Value)` after each iteration.


:mini:`meth (Sequence₁: sequence) >> (Sequence₂: sequence): Sequence`
   Returns an sequence that produces the values from :mini:`Sequence₁` followed by those from :mini:`Sequence₂`.


:mini:`meth >>(Sequence: sequence): Sequence`
   Returns an sequence that repeatedly produces the values from :mini:`Sequence` (for use with :mini:`limit`).


:mini:`meth :limit(Sequence: sequence, Limit: integer): sequence`
   Returns an sequence that produces at most :mini:`Limit` values from :mini:`Sequence`.


:mini:`meth :skip(Sequence: sequence, Skip: integer): sequence`
   Returns an sequence that skips the first :mini:`Skip` values from :mini:`Sequence` and then produces the rest.


:mini:`fun buffered(Size: integer, Sequence: any): Sequence`
   Returns an sequence that buffers the keys and values from :mini:`Sequence` in advance, buffering at most :mini:`Size` pairs.


:mini:`fun unique(Sequence: any): sequence`
   Returns an sequence that returns the unique values produced by :mini:`Sequence`. Uniqueness is determined by using a :mini:`map`.


:mini:`fun zip(Sequence₁: sequence, ...: sequence, Sequenceₙ: sequence, Function: any): sequence`
   Returns a new sequence that draws values :mini:`Vᵢ` from each of :mini:`Sequenceᵢ` and then produces :mini:`Functon(V₁, V₂, ..., Vₙ)`.

   The sequence stops produces values when any of the :mini:`Sequenceᵢ` stops.


:mini:`fun pair(Sequence₁: sequence, Sequence₂: sequence): sequence`
   Returns a new sequence that produces the values from :mini:`Sequence₁` as keys and the values from :mini:`Sequence₂` as values.


:mini:`fun weave(Sequence₁: sequence, ...: sequence, Sequenceₙ: sequence): sequence`
   Returns a new sequence that produces interleaved values :mini:`Vᵢ` from each of :mini:`Sequenceᵢ`.

   The sequence stops produces values when any of the :mini:`Sequenceᵢ` stops.


:mini:`fun fold(Sequence: sequence): sequence`
   Returns a new sequence that treats alternating values produced by :mini:`Sequence` as keys and values respectively.


:mini:`fun unfold(Sequence: sequence): sequence`
   Returns a new sequence that treats produces alternatively the keys and values produced by :mini:`Sequence`.


:mini:`fun swap(Sequence: sequence)`
   Returns a new sequence which swaps the keys and values produced by :mini:`Sequence`.


:mini:`fun key(Sequence: sequence)`
   Returns a new sequence which produces the keys of :mini:`Sequence`.


:mini:`fun batch(Sequence: sequence, Size: integer, Function: function): sequence`
   Returns a new sequence that calls :mini:`Function` with each batch of :mini:`Size` values produced by :mini:`Sequence` and produces the results.


