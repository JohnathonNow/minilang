runtime
=======

:mini:`type state < function`
   *TBD*

:mini:`type resumablestate < state`
   *TBD*

:mini:`fun callcc()`
   *TBD*

:mini:`fun markcc(Arg₁: any)`
   *TBD*

:mini:`fun calldc()`
   *TBD*

:mini:`fun swapcc(Arg₁: state)`
   *TBD*

:mini:`type reference`
   *TBD*

:mini:`type uninitialized`
   *TBD*

:mini:`meth (Arg₁: uninitialized) :: (Arg₂: string)`
   *TBD*

:mini:`meth :append(Arg₁: stringbuffer, Arg₂: errorvalue)`
   *TBD*

:mini:`fun break(Condition?: any)`
   If a debugger is present and :mini:`Condition` is omitted or not :mini:`nil` then triggers a breakpoint.


:mini:`type minidebugger`
   *TBD*

:mini:`fun mldebugger(Arg₁: any)`
   *TBD*

:mini:`meth :breakpoint_set(Arg₁: minidebugger, Arg₂: string, Arg₃: integer)`
   *TBD*

:mini:`meth :breakpoint_clear(Arg₁: minidebugger, Arg₂: string, Arg₃: integer)`
   *TBD*

:mini:`meth :error_mode(Arg₁: minidebugger, Arg₂: any)`
   *TBD*

:mini:`meth :step_mode(Arg₁: minidebugger, Arg₂: any)`
   *TBD*

:mini:`meth :step_in(Arg₁: minidebugger, Arg₂: state, Arg₃: any)`
   *TBD*

:mini:`meth :step_over(Arg₁: minidebugger, Arg₂: state, Arg₃: any)`
   *TBD*

:mini:`meth :step_out(Arg₁: minidebugger, Arg₂: state, Arg₃: any)`
   *TBD*

:mini:`meth :continue(Arg₁: minidebugger, Arg₂: state, Arg₃: any)`
   *TBD*

:mini:`meth :locals(Arg₁: state)`
   *TBD*

:mini:`meth :trace(Arg₁: state)`
   *TBD*

:mini:`meth :source(Arg₁: state)`
   *TBD*

:mini:`meth :error(Arg₁: channel, Arg₂: string, Arg₃: string)`
   *TBD*

:mini:`meth :raise(Arg₁: channel, Arg₂: string, Arg₃: any)`
   *TBD*

:mini:`meth :raise(Arg₁: channel, Arg₂: errorvalue)`
   *TBD*

