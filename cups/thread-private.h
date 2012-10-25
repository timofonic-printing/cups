/*
 * "$Id: thread-private.h 9755 2011-05-09 22:53:31Z mike $"
 *
 *   Private threading definitions for CUPS.
 *
 *   Copyright 2009-2010 by Apple Inc.
 *
 *   These coded instructions, statements, and computer programs are the
 *   property of Apple Inc. and are protected by Federal copyright
 *   law.  Distribution and use rights are outlined in the file "LICENSE.txt"
 *   which should have been included with this file.  If this file is
 *   file is missing or damaged, see the license at "http://www.cups.org/".
 */

#ifndef _CUPS_THREAD_PRIVATE_H_
#  define _CUPS_THREAD_PRIVATE_H_

/*
 * Include necessary headers...
 */

#  include "config.h"


/*
 * C++ magic...
 */

#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */


#  ifdef HAVE_PTHREAD_H
#    include <pthread.h>
typedef void *(*_cups_thread_func_t)(void *arg);
typedef pthread_mutex_t _cups_mutex_t;
typedef pthread_rwlock_t _cups_rwlock_t;
typedef pthread_key_t	_cups_threadkey_t;
#    define _CUPS_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#    define _CUPS_RWLOCK_INITIALIZER PTHREAD_RWLOCK_INITIALIZER
#    define _CUPS_THREADKEY_INITIALIZER -1
#    define _cupsThreadGetData(k) pthread_getspecific(k)
#    define _cupsThreadSetData(k,p) pthread_setspecific(k,p)

#  elif defined(WIN32)
#    include <winsock2.h>
#    include <windows.h>
typedef void *(__stdcall *_cups_thread_func_t)(void *arg);
typedef struct _cups_mutex_s
{
  int			m_init;		/* Flag for on-demand initialization */		
  CRITICAL_SECTION	m_criticalSection;
					/* Win32 Critical Section */
} _cups_mutex_t;
typedef _cups_mutex_t _cups_rwlock_t;	/* TODO: Implement Win32 reader/writer lock */
typedef DWORD	_cups_threadkey_t;
#    define _CUPS_MUTEX_INITIALIZER { 0, 0 }
#    define _CUPS_RWLOCK_INITIALIZER { 0, 0 }
#    define _CUPS_THREADKEY_INITIALIZER 0
#    define _cupsThreadGetData(k) TlsGetValue(k)
#    define _cupsThreadSetData(k,p) TlsSetValue(k,p)

#  else
typedef char	_cups_mutex_t;
typedef char	_cups_rwlock_t;
typedef void	*_cups_threadkey_t;
#    define _CUPS_MUTEX_INITIALIZER 0
#    define _CUPS_RWLOCK_INITIALIZER 0
#    define _CUPS_THREADKEY_INITIALIZER (void *)0
#    define _cupsThreadGetData(k) k
#    define _cupsThreadSetData(k,p) k=p
#  endif /* HAVE_PTHREAD_H */


/*
 * Functions...
 */

extern void	_cupsMutexInit(_cups_mutex_t *mutex);
extern void	_cupsMutexLock(_cups_mutex_t *mutex);
extern void	_cupsMutexUnlock(_cups_mutex_t *mutex);
extern void	_cupsRWInit(_cups_rwlock_t *rwlock);
extern void	_cupsRWLockRead(_cups_rwlock_t *rwlock);
extern void	_cupsRWLockWrite(_cups_rwlock_t *rwlock);
extern void	_cupsRWUnlock(_cups_rwlock_t *rwlock);
extern int	_cupsThreadCreate(_cups_thread_func_t func, void *arg);


#  ifdef __cplusplus
}
#  endif /* __cplusplus */
#endif /* !_CUPS_THREAD_PRIVATE_H_ */

/*
 * End of "$Id: thread-private.h 9755 2011-05-09 22:53:31Z mike $".
 */
