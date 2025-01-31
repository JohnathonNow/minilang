time
====

:mini:`type time`
   A time in UTC with nanosecond resolution.


:mini:`meth time(): time`
   Returns the current UTC time.


:mini:`meth time(String: string): time`
   Parses the :mini:`String` as a time according to ISO 8601.


:mini:`meth time(String: string, Format: string): time`
   Parses the :mini:`String` as a time according to specified format. The time is assumed to be in local time.


:mini:`meth time(String: string, Format: string, UTC: boolean): time`
   Parses the :mini:`String` as a time according to specified format. The time is assumed to be in local time unless UTC is :mini:`true`.


:mini:`meth :nsec(Time: time): integer`
   Returns the nanoseconds component of :mini:`Time`.


:mini:`meth string(Time: time): string`
   Formats :mini:`Time` as a local time.


:mini:`meth string(Time: time, TimeZone: nil): string`
   Formats :mini:`Time` as a UTC time according to ISO 8601.


:mini:`meth string(Time: time, Format: string): string`
   Formats :mini:`Time` as a local time according to the specified format.


:mini:`meth string(Time: time, Format: string, TimeZone: nil): string`
   Formats :mini:`Time` as a UTC time according to the specified format.


:mini:`meth (Arg₁: time) <> (Arg₂: time)`
   *TBD*

:mini:`meth (Arg₁: time) - (Arg₂: time)`
   *TBD*

:mini:`meth (Arg₁: time) + (Arg₂: number)`
   *TBD*

:mini:`meth (Arg₁: time) - (Arg₂: number)`
   *TBD*

