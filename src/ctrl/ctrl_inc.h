// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef CTRL_INC_H
#define CTRL_INC_H

////////////////////////////////
//~ NOTE(rjf): Control Layer Overview (2023/8/29)
//
// This layer's purpose is to provide access to the asynchronously-running, low
// level parts of a debugger, running on the debugger client. This primarily
// consists of process control, using the Demon layer (the lower level
// abstraction layer for process control, across multiple OSes), but including
// higher-level concepts, like stepping, breakpoint resolution, conditional
// breakpoint evaluation, and so on. Right now, this just includes process
// control *local to the debugger client machine*. But in the future, this can
// also include communication to multiple target machines, all running their
// own process controller, using the Demon layer.
//
// This part of a debugger must run asynchronously to prevent blocking the UI -
// ideally our debugger is designed such that, if targets are running, the
// debugger frontend is still usable for a variety of purposes. So, in short,
// the asynchronously-running "control thread", implemented by this layer, is
// tasked with communicating with a separately executing "user thread". This
// communication happens in two directions - `user -> ctrl`, and the reverse,
// `ctrl -> user`.
//
// In the case of `user -> ctrl` communication, this is done with a ring buffer
// of "messages" (`CTRL_Msg`), pushed via `ctrl_u2c_push_msgs`. These messages
// include commands like: launching targets, attaching to targets, killing
// targets, detaching from targets, stepping/running, or single stepping.
//
// In the case of `ctrl -> user` communication, this is done with a ring buffer
// of "events" (`CTRL_Event`), popped via `ctrl_c2u_pop_events`. These events
// include information about what happened during the execution of targets -
// including: process/module/thread creation, process/module/thread deletion,
// debug strings, thread name events, memory allocation events, and stop events
// (where stops can be caused by: user breakpoints, traps set for stepping,
// exceptions, halts, or errors).
//
// The various stepping algorithms are implemented with two concepts: (a) the
// "trap net", and (b) "spoofs".
//
// A "trap net" is a term which refers to a set of addresses paired with a set
// of behavioral flags. Before targets run, trap instructions are written to
// these addresses. After targets stop, these addresses are reset to their
// original bytes. These trap instructions cause the debugger's targets to
// stop executing, and based on which behavioral flags are associated with
// the instruction causing the stop, the control thread may adjust parameters
// used for running, then continue execution, or it will not resume target
// execution, and will report stopped events. These behavioral flags can
// include: single-stepping the stopped thread to execute the instruction at
// the trap location, saving a stack pointer "check value" (where this check
// value is compared against when making decisions about whether to continue
// running or not), and so on. It's complicated to unpack why exactly these
// behaviors are useful, but the TL;DR of it is that they are used for a
// variety of stepping behaviors. For example, when doing a "step into" step,
// a `call` instruction can have a trap set at it, and will be marked with
// a "single-step-after" trap flag, as well as the "end stepping" trap flag,
// such that the step operation will complete after the `call` has executed.
//
// A "spoof" is a feature the control layer uses to detect when some thread
// returns from a particular sub-callstack. This is useful when implementing
// "step over" in functions that may be recursive. In short, unlike a trap,
// which writes a trap instruction (like `int3`) into an instruction stream,
// a spoof overwrites a *return address* on some thread's *stack*. This return
// address is not a valid address for executing code -- it is simply a value
// that the debugger can recognize, such that it is notified when the thread
// returns from some level in a callstack. When the thread exits some function,
// it will return to the "spoofed" address, and it will immediately hit an
// exception, because the spoofed address will not be a valid address for
// code execution. At that point, the debugger can move the thread back to
// the pre-spoof return address, and resume execution.

#include "ctrl_core.h"

#endif //CTRL_INC_H
