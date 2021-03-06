/* -----------------------------------------------------------------------------
 *
 * (c) The GHC Team 2006-2009
 *
 * Debug and performance tracing
 *
 * ---------------------------------------------------------------------------*/

// external headers
#include "Rts.h"

// internal headers
#include "Trace.h"

#ifdef TRACING

#include "GetTime.h"
#include "Stats.h"
#include "eventlog/EventLog.h"
#include "Threads.h"
#include "Printer.h"

#ifdef DEBUG
// debugging flags, set with +RTS -D<something>
int DEBUG_sched;
int DEBUG_interp;
int DEBUG_weak;
int DEBUG_gccafs;
int DEBUG_gc;
int DEBUG_block_alloc;
int DEBUG_sanity;
int DEBUG_stable;
int DEBUG_stm;
int DEBUG_prof;
int DEBUG_gran;
int DEBUG_par;
int DEBUG_linker;
int DEBUG_squeeze;
int DEBUG_hpc;
int DEBUG_sparks;
#endif

// events
int TRACE_sched;

#ifdef THREADED_RTS
static Mutex trace_utx;
#endif

static rtsBool eventlog_enabled;

/* ---------------------------------------------------------------------------
   Starting up / shuttting down the tracing facilities
 --------------------------------------------------------------------------- */

void initTracing (void)
{
#ifdef THREADED_RTS
    initMutex(&trace_utx);
#endif

#ifdef DEBUG
#define DEBUG_FLAG(name, class) \
    class = RtsFlags.DebugFlags.name ? 1 : 0;

    DEBUG_FLAG(scheduler,    DEBUG_sched);

    DEBUG_FLAG(interpreter,  DEBUG_interp);
    DEBUG_FLAG(weak,         DEBUG_weak);
    DEBUG_FLAG(gccafs,       DEBUG_gccafs);
    DEBUG_FLAG(gc,           DEBUG_gc);
    DEBUG_FLAG(block_alloc,  DEBUG_block_alloc);
    DEBUG_FLAG(sanity,       DEBUG_sanity);
    DEBUG_FLAG(stable,       DEBUG_stable);
    DEBUG_FLAG(stm,          DEBUG_stm);
    DEBUG_FLAG(prof,         DEBUG_prof);
    DEBUG_FLAG(linker,       DEBUG_linker);
    DEBUG_FLAG(squeeze,      DEBUG_squeeze);
    DEBUG_FLAG(hpc,          DEBUG_hpc);
    DEBUG_FLAG(sparks,       DEBUG_sparks);
#endif

    // -Ds turns on scheduler tracing too
    TRACE_sched =
        RtsFlags.TraceFlags.scheduler ||
        RtsFlags.DebugFlags.scheduler;

    eventlog_enabled = RtsFlags.TraceFlags.tracing == TRACE_EVENTLOG;

    if (eventlog_enabled) {
        initEventLogging();
    }
}

void endTracing (void)
{
    if (eventlog_enabled) {
        endEventLogging();
    }
}

void freeTracing (void)
{
    if (eventlog_enabled) {
        freeEventLogging();
    }
}

void resetTracing (void)
{
    if (eventlog_enabled) {
        abortEventLogging(); // abort eventlog inherited from parent
        initEventLogging(); // child starts its own eventlog
    }
}

/* ---------------------------------------------------------------------------
   Emitting trace messages/events
 --------------------------------------------------------------------------- */

#ifdef DEBUG
static void tracePreface (void)
{
#ifdef THREADED_RTS
    debugBelch("%12lx: ", (unsigned long)osThreadId());
#endif
    if (RtsFlags.TraceFlags.timestamp) {
	debugBelch("%9" FMT_Word64 ": ", stat_getElapsedTime());
    }
}
#endif

#ifdef DEBUG
static char *thread_stop_reasons[] = {
    [HeapOverflow] = "heap overflow",
    [StackOverflow] = "stack overflow",
    [ThreadYielding] = "yielding",
    [ThreadBlocked] = "blocked",
    [ThreadFinished] = "finished",
    [THREAD_SUSPENDED_FOREIGN_CALL] = "suspended while making a foreign call",
    [6 + BlockedOnMVar]         = "blocked on an MVar",
    [6 + BlockedOnBlackHole]    = "blocked on a black hole",
    [6 + BlockedOnRead]         = "blocked on a read operation",
    [6 + BlockedOnWrite]        = "blocked on a write operation",
    [6 + BlockedOnDelay]        = "blocked on a delay operation",
    [6 + BlockedOnSTM]          = "blocked on STM",
    [6 + BlockedOnDoProc]       = "blocked on asyncDoProc",
    [6 + BlockedOnCCall]        = "blocked on a foreign call",
    [6 + BlockedOnCCall_Interruptible] = "blocked on a foreign call (interruptible)",
    [6 + BlockedOnMsgThrowTo]   =  "blocked on throwTo",
    [6 + ThreadMigrating]       =  "migrating"
};
#endif

#ifdef DEBUG
static void traceSchedEvent_stderr (Capability *cap, EventTypeNum tag, 
                                    StgTSO *tso, 
                                    StgWord info1 STG_UNUSED,
                                    StgWord info2 STG_UNUSED)
{
    ACQUIRE_LOCK(&trace_utx);

    tracePreface();
    switch (tag) {
    case EVENT_CREATE_THREAD:   // (cap, thread)
        debugBelch("cap %d: created thread %lu\n", 
                   cap->no, (lnat)tso->id);
        break;
    case EVENT_RUN_THREAD:      //  (cap, thread)
        debugBelch("cap %d: running thread %lu (%s)\n", 
                   cap->no, (lnat)tso->id, what_next_strs[tso->what_next]);
        break;
    case EVENT_THREAD_RUNNABLE: // (cap, thread)
        debugBelch("cap %d: thread %lu appended to run queue\n", 
                   cap->no, (lnat)tso->id);
        break;
    case EVENT_RUN_SPARK:       // (cap, thread)
        debugBelch("cap %d: thread %lu running a spark\n", 
                   cap->no, (lnat)tso->id);
        break;
    case EVENT_CREATE_SPARK_THREAD: // (cap, spark_thread)
        debugBelch("cap %d: creating spark thread %lu\n", 
                   cap->no, (long)info1);
        break;
    case EVENT_MIGRATE_THREAD:  // (cap, thread, new_cap)
        debugBelch("cap %d: thread %lu migrating to cap %d\n", 
                   cap->no, (lnat)tso->id, (int)info1);
        break;
    case EVENT_STEAL_SPARK:     // (cap, thread, victim_cap)
        debugBelch("cap %d: thread %lu stealing a spark from cap %d\n", 
                   cap->no, (lnat)tso->id, (int)info1);
        break;
    case EVENT_THREAD_WAKEUP:   // (cap, thread, info1_cap)
        debugBelch("cap %d: waking up thread %lu on cap %d\n", 
                   cap->no, (lnat)tso->id, (int)info1);
        break;
        
    case EVENT_STOP_THREAD:     // (cap, thread, status)
        if (info1 == 6 + BlockedOnBlackHole) {
            debugBelch("cap %d: thread %lu stopped (blocked on black hole owned by thread %lu)\n",
                       cap->no, (lnat)tso->id, (long)info2);
        } else {
            debugBelch("cap %d: thread %lu stopped (%s)\n",
                       cap->no, (lnat)tso->id, thread_stop_reasons[info1]);
        }
        break;
    case EVENT_SHUTDOWN:        // (cap)
        debugBelch("cap %d: shutting down\n", cap->no);
        break;
    case EVENT_REQUEST_SEQ_GC:  // (cap)
        debugBelch("cap %d: requesting sequential GC\n", cap->no);
        break;
    case EVENT_REQUEST_PAR_GC:  // (cap)
        debugBelch("cap %d: requesting parallel GC\n", cap->no);
        break;
    case EVENT_GC_START:        // (cap)
        debugBelch("cap %d: starting GC\n", cap->no);
        break;
    case EVENT_GC_END:          // (cap)
        debugBelch("cap %d: finished GC\n", cap->no);
        break;
    case EVENT_GC_IDLE:        // (cap)
        debugBelch("cap %d: GC idle\n", cap->no);
        break;
    case EVENT_GC_WORK:          // (cap)
        debugBelch("cap %d: GC working\n", cap->no);
        break;
    case EVENT_GC_DONE:          // (cap)
        debugBelch("cap %d: GC done\n", cap->no);
        break;
    default:
        debugBelch("cap %d: thread %lu: event %d\n\n", 
                   cap->no, (lnat)tso->id, tag);
        break;
    }

    RELEASE_LOCK(&trace_utx);
}
#endif

void traceSchedEvent_ (Capability *cap, EventTypeNum tag, 
                       StgTSO *tso, StgWord info1, StgWord info2)
{
#ifdef DEBUG
    if (RtsFlags.TraceFlags.tracing == TRACE_STDERR) {
        traceSchedEvent_stderr(cap, tag, tso, info1, info2);
    } else
#endif
    {
        postSchedEvent(cap,tag,tso ? tso->id : 0, info1, info2);
    }
}

void traceEvent_ (Capability *cap, EventTypeNum tag)
{
#ifdef DEBUG
    if (RtsFlags.TraceFlags.tracing == TRACE_STDERR) {
        traceSchedEvent_stderr(cap, tag, 0, 0, 0);
    } else
#endif
    {
        postEvent(cap,tag);
    }
}

#ifdef DEBUG
static void traceCap_stderr(Capability *cap, char *msg, va_list ap)
{
    ACQUIRE_LOCK(&trace_utx);

    tracePreface();
    debugBelch("cap %d: ", cap->no);
    vdebugBelch(msg,ap);
    debugBelch("\n");

    RELEASE_LOCK(&trace_utx);
}
#endif

void traceCap_(Capability *cap, char *msg, ...)
{
    va_list ap;
    va_start(ap,msg);
    
#ifdef DEBUG
    if (RtsFlags.TraceFlags.tracing == TRACE_STDERR) {
        traceCap_stderr(cap, msg, ap);
    } else
#endif
    {
        postCapMsg(cap, msg, ap);
    }

    va_end(ap);
}

#ifdef DEBUG
static void trace_stderr(char *msg, va_list ap)
{
    ACQUIRE_LOCK(&trace_utx);

    tracePreface();
    vdebugBelch(msg,ap);
    debugBelch("\n");

    RELEASE_LOCK(&trace_utx);
}
#endif

void trace_(char *msg, ...)
{
    va_list ap;
    va_start(ap,msg);

#ifdef DEBUG
    if (RtsFlags.TraceFlags.tracing == TRACE_STDERR) {
        trace_stderr(msg, ap);
    } else
#endif
    {
        postMsg(msg, ap);
    }

    va_end(ap);
}

static void traceFormatUserMsg(Capability *cap, char *msg, ...)
{
    va_list ap;
    va_start(ap,msg);

#ifdef DEBUG
    if (RtsFlags.TraceFlags.tracing == TRACE_STDERR) {
        traceCap_stderr(cap, msg, ap);
    } else
#endif
    {
        if (eventlog_enabled) {
            postUserMsg(cap, msg, ap);
        }
    }
    dtraceUserMsg(cap->no, msg);
}

void traceUserMsg(Capability *cap, char *msg)
{
    traceFormatUserMsg(cap, "%s", msg);
}

void traceThreadStatus_ (StgTSO *tso USED_IF_DEBUG)
{
#ifdef DEBUG
    if (RtsFlags.TraceFlags.tracing == TRACE_STDERR) {
        printThreadStatus(tso);
    } else
#endif
    {
        /* nothing - no event for this one yet */
    }
}


#ifdef DEBUG
void traceBegin (const char *str, ...)
{
    va_list ap;
    va_start(ap,str);

    ACQUIRE_LOCK(&trace_utx);

    tracePreface();
    vdebugBelch(str,ap);
}

void traceEnd (void)
{
    debugBelch("\n");
    RELEASE_LOCK(&trace_utx);
}
#endif /* DEBUG */

#endif /* TRACING */

// If DTRACE is enabled, but neither DEBUG nor TRACING, we need a C land
// wrapper for the user-msg probe (as we can't expand that in PrimOps.cmm)
//
#if !defined(DEBUG) && !defined(TRACING) && defined(DTRACE)

void dtraceUserMsgWrapper(Capability *cap, char *msg)
{
    dtraceUserMsg(cap->no, msg);
}

#endif /* !defined(DEBUG) && !defined(TRACING) && defined(DTRACE) */
