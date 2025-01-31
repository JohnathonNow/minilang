array
=====

:mini:`fun array(List: list): array`
   Returns a new array containing the values in :mini:`List`.

   The shape and type of the array is determined from the elements in :mini:`List`.


:mini:`type array < address, sequence`
   Base type for multidimensional arrays.


:mini:`meth :degree(Array: array): integer`
   Return the degree of :mini:`Array`.


:mini:`meth :shape(Array: array): list`
   Return the shape of :mini:`Array`.


:mini:`meth :count(Array: array): integer`
   Return the number of elements in :mini:`Array`.


:mini:`meth ^(Array: array): array`
   Returns the transpose of :mini:`Array`, sharing the underlying data.


:mini:`meth :permute(Array: array, Indices: list): array`
   Returns an array sharing the underlying data with :mini:`Array`, permuting the axes according to :mini:`Indices`.


:mini:`meth :swap(Arg₁: array, Arg₂: integer, Arg₃: integer)`
   *TBD*

:mini:`meth :expand(Array: array, Indices: list): array`
   Returns an array sharing the underlying data with :mini:`Array` with additional unit-length axes at the specified :mini:`Indices`.


:mini:`meth :split(Arg₁: array, Arg₂: integer, Arg₃: list)`
   *TBD*

:mini:`meth :join(Arg₁: array, Arg₂: integer, Arg₃: integer)`
   *TBD*

:mini:`meth :strides(Array: array): list`
   Return the strides of :mini:`Array` in bytes.


:mini:`meth :size(Array: array): integer`
   Return the size of :mini:`Array` in bytes.


:mini:`meth (Array: array)[Indices...: any, ...]: array`
   Returns a sub-array of :mini:`Array` sharing the underlying data.

   The :mini:`i`-th dimension is indexed by the corresponding :mini:`Indexᵢ`.

   * If :mini:`Indexᵢ` is :mini:`nil` then the :mini:`i`-th dimension is copied unchanged.

   * If :mini:`Indexᵢ` is an integer then the :mini:`Indexᵢ`-th value is selected and the :mini:`i`-th dimension is dropped from the result.

   * If :mini:`Indexᵢ` is a list of integers then the :mini:`i`-th dimension is copied as a sparse dimension with the respective entries.

   If fewer than :mini:`A:degree` indices are provided then the remaining dimensions are copied unchanged.


:mini:`meth (Array: array)[Indices: map]: array`
   Returns a sub-array of :mini:`Array` sharing the underlying data.

   The :mini:`i`-th dimension is indexed by :mini:`Indices[i]` if present, and :mini:`nil` otherwise.


:mini:`type array::int8 < array`
   An array of int8 values.


:mini:`type array::uint8 < array`
   An array of uint8 values.


:mini:`type array::int16 < array`
   An array of int16 values.


:mini:`type array::uint16 < array`
   An array of uint16 values.


:mini:`type array::int32 < array`
   An array of int32 values.


:mini:`type array::uint32 < array`
   An array of uint32 values.


:mini:`type array::int64 < array`
   An array of int64 values.


:mini:`type array::uint64 < array`
   An array of uint64 values.


:mini:`type array::float32 < array`
   An array of float32 values.


:mini:`type array::float64 < array`
   An array of float64 values.


:mini:`type array::complex32 < array`
   An array of complex32 values.


:mini:`type array::complex64 < array`
   An array of complex64 values.


:mini:`type array::value < array`
   An array of value values.


:mini:`meth :reshape(Arg₁: array, Arg₂: list)`
   *TBD*

:mini:`meth :sums(Array: array, Index: integer): array`
   Returns a new array with the partial sums of :mini:`Array` in the :mini:`Index`-th dimension.


:mini:`meth :prods(Array: array, Index: integer): array`
   Returns a new array with the partial products of :mini:`Array` in the :mini:`Index`-th dimension.


:mini:`meth :sum(Array: array): number`
   Returns the sum of the values in :mini:`Array`.


:mini:`meth :sum(Array: array, Index: integer): array`
   Returns a new array with the sums of :mini:`Array` in the :mini:`Index`-th dimension.


:mini:`meth :prod(Array: array): number`
   Returns the product of the values in :mini:`Array`.


:mini:`meth :prod(Array: array, Index: integer): array`
   Returns a new array with the products of :mini:`Array` in the :mini:`Index`-th dimension.


:mini:`meth -(Array: array): array`
   Returns an array with the negated values from :mini:`Array`.


:mini:`meth (Arg₁: array) + (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) + (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) + (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) + (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) * (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) * (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) * (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) * (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) - (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) - (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) - (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) - (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) / (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) / (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) / (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) / (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) = (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) = (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) = (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) = (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) != (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) != (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) != (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) != (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) < (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) < (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) < (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) < (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) > (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) > (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) > (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) > (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) <= (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) <= (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) <= (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) <= (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) >= (Arg₂: integer)`
   *TBD*

:mini:`meth (Arg₁: integer) >= (Arg₂: array)`
   *TBD*

:mini:`meth (Arg₁: array) >= (Arg₂: double)`
   *TBD*

:mini:`meth (Arg₁: double) >= (Arg₂: array)`
   *TBD*

:mini:`meth :copy(Array: array): array`
   Return a new array with the same values of :mini:`Array` but not sharing the underlying data.


:mini:`meth $(List: list): array`
   Returns an array with the contents of :mini:`List`.


:mini:`meth ^(List: list): array`
   Returns an array with the contents of :mini:`List`, transposed.


:mini:`meth :copy(Array: array, Function: function): array`
   Return a new array with the results of applying :mini:`Function` to each value of :mini:`Array`.


:mini:`meth :update(Array: array, Function: function): array`
   Update the values in :mini:`Array` in place by applying :mini:`Function` to each value.


:mini:`meth (A: array) . (B: array): array`
   Returns the inner product of :mini:`A` and :mini:`B`. The last dimension of :mini:`A` and the first dimension of :mini:`B` must match, skipping any dimensions of size :mini:`1`.


