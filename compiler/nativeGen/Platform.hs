
-- | A description of the platform we're compiling for.
--	Used by the native code generator.
--	In the future, this module should be the only one that references
--	the evil #defines for each TARGET_ARCH and TARGET_OS
--
module Platform (
	Platform(..),
	Arch(..),
	OS(..),

	defaultTargetPlatform,
	osElfTarget
)

where

#include "HsVersions.h"


-- | Contains enough information for the native code generator to emit
--	code for this platform.
data Platform
	= Platform 
	{ platformArch	:: Arch
	, platformOS	:: OS }


-- | Architectures that the native code generator knows about.
--	TODO: It might be nice to extend these constructors with information
--	about what instruction set extensions an architecture might support.
--
data Arch
	= ArchAlpha
	| ArchX86
	| ArchX86_64
	| ArchPPC
	| ArchPPC_64
	| ArchSPARC
	deriving (Show, Eq)
	

-- | Operating systems that the native code generator knows about.
--	Having OSUnknown should produce a sensible default, but no promises.
data OS
	= OSUnknown
	| OSLinux
	| OSDarwin
	| OSSolaris2
	| OSMinGW32
	| OSFreeBSD
	| OSOpenBSD
	deriving (Show, Eq)


-- | This predicates tells us whether the OS supports ELF-like shared libraries.
osElfTarget :: OS -> Bool
osElfTarget OSLinux   = True
osElfTarget OSFreeBSD = True
osElfTarget OSOpenBSD = True
osElfTarget OSSolaris2 = True
osElfTarget _         = False

-- | This is the target platform as far as the #ifdefs are concerned.
--	These are set in includes/ghcplatform.h by the autoconf scripts
defaultTargetPlatform :: Platform
defaultTargetPlatform
	= Platform defaultTargetArch defaultTargetOS


-- | Move the evil TARGET_ARCH #ifdefs into Haskell land.
defaultTargetArch :: Arch
#if   alpha_TARGET_ARCH
defaultTargetArch	= ArchAlpha
#elif i386_TARGET_ARCH
defaultTargetArch	= ArchX86
#elif x86_64_TARGET_ARCH
defaultTargetArch	= ArchX86_64
#elif powerpc_TARGET_ARCH
defaultTargetArch	= ArchPPC
#elif powerpc64_TARGET_ARCH
defaultTargetArch	= ArchPPC_64
#elif sparc_TARGET_ARCH
defaultTargetArch	= ArchSPARC
#else
#error	"Platform.buildArch: undefined"
#endif


-- | Move the evil TARGET_OS #ifdefs into Haskell land.
defaultTargetOS :: OS
#if   linux_TARGET_OS
defaultTargetOS	= OSLinux
#elif darwin_TARGET_OS
defaultTargetOS	= OSDarwin
#elif solaris2_TARGET_OS
defaultTargetOS	= OSSolaris2
#elif mingw32_TARGET_OS
defaultTargetOS	= OSMinGW32
#elif freebsd_TARGET_OS
defaultTargetOS	= OSFreeBSD
#elif openbsd_TARGET_OS
defaultTargetOS	= OSOpenBSD
#else
defaultTargetOS	= OSUnknown
#endif

