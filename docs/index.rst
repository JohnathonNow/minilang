Minilang
========

Introduction
------------

*Minilang* is a small imperative language, designed to be embedded into programs
written in *C*. It has the following goals / features:

**Minimal dependencies**
   *Minilang* has no dependencies other than the Hans Boehm garbage
   collector and standard *C* libraries.

**Minimal language**
   *Minilang* is a fairly simple language with no built-in class declarations or module system. This allows it to be embedded in applications with strict control on available features. There is no loss in expression though as the syntax allows classes and modules to be added seamlessly.

**Flexible runtime**
   All functions in *Minilang* can be suspended if necessary (using a limited form of continuations). This allows asynchronous functions and preemptive multitasking to be added if required, without imposing them when they are not needed.

**Full closures**
   Functions in *Minilang* automatically capture their environment, creating
   closures at runtime. Closures can be passed around as first-class values,
   which are used to build targets in *Rabs*. 

**Dynamic scoping**
   The *Minilang* interpreter provides a callback for identifier resolution,
   allowing programs to provide dynamic scoping. This is used within *Rabs* to
   provide dynamic symbol resolution.

**Easy to embed & extend**
   *Minilang* provides a comprehensive embedding API to support a wider range of use cases. It is easy to create new functions in *C* to use in *Minilang*.

Sample
------

.. code-block:: mini

   print("Hello world!\n")
   
   var L := [1, 2, 3, 4, 5]
   
   for X in L do
      print('X = {X}\n')
   end

Details
=======

.. toctree::
   :maxdepth: 1
   
   /start
   /language
   /library
   /features
   /embedding
   /extending
   /api
   /internals   
