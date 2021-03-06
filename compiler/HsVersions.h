#ifndef HSVERSIONS_H
#define HSVERSIONS_H

#if 0

IMPORTANT!  If you put extra tabs/spaces in these macro definitions,
you will screw up the layout where they are used in case expressions!

(This is cpp-dependent, of course)

#endif

/* Useful in the headers that we share with the RTS */
#define COMPILING_GHC 1

/* Pull in all the platform defines for this build (foo_TARGET_ARCH etc.) */
#include "ghc_boot_platform.h"

/* This macro indicates that the target OS supports ELF-like shared libraries */
#if linux_TARGET_OS || freebsd_TARGET_OS || openbsd_TARGET_OS || solaris2_TARGET_OS
#define elf_OBJ_FORMAT 1
#endif

/* Pull in the autoconf defines (HAVE_FOO), but don't include
 * ghcconfig.h, because that will include ghcplatform.h which has the
 * wrong platform settings for the compiler (it has the platform
 * settings for the target plat instead). */
#include "../includes/ghcautoconf.h"

/* Global variables may not work in other Haskell implementations,
 * but we need them currently! so the conditional on GLASGOW won't do. */
#ifndef __HADDOCK__
#if defined(__GLASGOW_HASKELL__) || !defined(__GLASGOW_HASKELL__)
#define GLOBAL_VAR(name,value,ty)  \
{-# NOINLINE name #-};             \
name :: IORef (ty);                \
name = Util.global (value);

#define GLOBAL_MVAR(name,value,ty) \
{-# NOINLINE name #-};             \
name :: MVar (ty);                 \
name = Util.globalMVar (value);
#endif
#else /* __HADDOCK__ */
#define GLOBAL_VAR(name,value,ty)  \
name :: IORef (ty);                \
name = Util.global (value);

#define GLOBAL_MVAR(name,value,ty) \
name :: MVar (ty);                 \
name = Util.globalMVar (value);
#endif

#define COMMA ,

#ifdef DEBUG
#define ASSERT(e)      if (not (e)) then (assertPanic __FILE__ __LINE__) else
#define ASSERT2(e,msg) if (not (e)) then (assertPprPanic __FILE__ __LINE__ (msg)) else
#define WARN( e, msg ) (warnPprTrace (e) __FILE__ __LINE__ (msg)) $
#else
-- We have to actually use all the variables we are given or we may get
-- unused variable warnings when DEBUG is off.
#define ASSERT(e)      if False && (not (e)) then panic "ASSERT" else
#define ASSERT2(e,msg) if False && (const False (e,msg)) then pprPanic "ASSERT2" (msg) else
#define WARN(e,msg)    if False && (e) then pprPanic "WARN" (msg) else
-- Here we deliberately don't use when as Control.Monad might not be imported
#endif

-- Examples:   Assuming   flagSet :: String -> m Bool
-- 
--    do { c   <- getChar; MASSERT( isUpper c ); ... }
--    do { c   <- getChar; MASSERT2( isUpper c, text "Bad" ); ... }
--    do { str <- getStr;  ASSERTM( flagSet str ); .. }
--    do { str <- getStr;  ASSERTM2( flagSet str, text "Bad" ); .. }
--    do { str <- getStr;  WARNM2( flagSet str, text "Flag is set" ); .. }
#define MASSERT(e)      ASSERT(e) return ()
#define MASSERT2(e,msg) ASSERT2(e,msg) return ()
#define ASSERTM(e)      do { bool <- e; MASSERT(bool) }
#define ASSERTM2(e,msg) do { bool <- e; MASSERT2(bool,msg) }
#define WARNM2(e,msg)   do { bool <- e; WARN(bool, msg) return () }

-- Useful for declaring arguments to be strict
#define STRICT1(f) f a | a `seq` False = undefined
#define STRICT2(f) f a b | a `seq` b `seq` False = undefined
#define STRICT3(f) f a b c | a `seq` b `seq` c `seq` False = undefined
#define STRICT4(f) f a b c d | a `seq` b `seq` c `seq` d `seq` False = undefined
#define STRICT5(f) f a b c d e | a `seq` b `seq` c `seq` d `seq` e `seq` False = undefined
#define STRICT6(f) f a b c d e f | a `seq` b `seq` c `seq` d `seq` e `seq` f `seq` False = undefined

#endif /* HsVersions.h */

