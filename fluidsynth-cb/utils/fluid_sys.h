/* FluidSynth - A Software Synthesizer
 *
 * Copyright (C) 2003  Peter Hanappe and others.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */


/**

   This header contains a bunch of (mostly) system and machine
   dependent functions:

   - timers
   - current time in milliseconds and microseconds
   - debug logging
   - profiling
   - memory locking
   - checking for floating point exceptions

 */

#ifndef _FLUID_SYS_H
#define _FLUID_SYS_H

#include "fluidsynth_priv.h"

#ifdef LADSPA
#include <gmodule.h>
#endif

#include <glib/gstdio.h>

/**
 * Macro used for safely accessing a message from a GError and using a default
 * message if it is NULL.
 * @param err Pointer to a GError to access the message field of.
 * @return Message string
 */
#define fluid_gerror_message(err)  ((err) ? err->message : "No error details")


void fluid_sys_config(void);
void fluid_log_config(void);
void fluid_time_config(void);


/* Misc */
#if defined(__INTEL_COMPILER)
#define FLUID_RESTRICT restrict
#elif defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#define FLUID_RESTRICT __restrict__
#elif defined(_MSC_VER) && _MSC_VER >= 1400
#define FLUID_RESTRICT __restrict
#else
#warning "Dont know how this compiler handles restrict pointers, refuse to use them."
#define FLUID_RESTRICT
#endif

#define FLUID_INLINE              inline
#define FLUID_POINTER_TO_UINT     GPOINTER_TO_UINT
#define FLUID_UINT_TO_POINTER     GUINT_TO_POINTER
#define FLUID_POINTER_TO_INT      GPOINTER_TO_INT
#define FLUID_INT_TO_POINTER      GINT_TO_POINTER
#define FLUID_N_ELEMENTS(struct)  (sizeof (struct) / sizeof (struct[0]))
#define FLUID_MEMBER_SIZE(struct, member)  ( sizeof (((struct *)0)->member) )

#define FLUID_IS_BIG_ENDIAN       (G_BYTE_ORDER == G_BIG_ENDIAN)

#define FLUID_LE32TOH(x)          GINT32_FROM_LE(x)
#define FLUID_LE16TOH(x)          GINT16_FROM_LE(x)


#define fluid_return_if_fail(cond) \
if(cond) \
    ; \
else \
    return

#define fluid_return_val_if_fail(cond, val) \
 fluid_return_if_fail(cond) (val)


/*
 * Utility functions
 */
char *fluid_strtok (char **str, char *delim);


#if defined(__OS2__)
#define INCL_DOS
#include <os2.h>

typedef int socklen_t;
#endif

unsigned int fluid_curtime(void);
double fluid_utime(void);


/**
    Timers

 */

/* if the callback function returns 1 the timer will continue; if it
   returns 0 it will stop */
typedef int (*fluid_timer_callback_t)(void* data, unsigned int msec);

typedef struct _fluid_timer_t fluid_timer_t;

fluid_timer_t* new_fluid_timer(int msec, fluid_timer_callback_t callback,
                               void* data, int new_thread, int auto_destroy,
                               int high_priority);

void delete_fluid_timer(fluid_timer_t* timer);
int fluid_timer_join(fluid_timer_t* timer);
int fluid_timer_stop(fluid_timer_t* timer);

// Macros to use for pre-processor if statements to test which Glib thread API we have (pre or post 2.32)
#define NEW_GLIB_THREAD_API  (GLIB_MAJOR_VERSION > 2 || (GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION >= 32))
#define OLD_GLIB_THREAD_API  (GLIB_MAJOR_VERSION < 2 || (GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION < 32))

/* Muteces */

#if NEW_GLIB_THREAD_API

/* glib 2.32 and newer */

/* Regular mutex */
typedef GMutex fluid_mutex_t;
#define FLUID_MUTEX_INIT          { 0 }
#define fluid_mutex_init(_m)      g_mutex_init (&(_m))
#define fluid_mutex_destroy(_m)   g_mutex_clear (&(_m))
#define fluid_mutex_lock(_m)      g_mutex_lock(&(_m))
#define fluid_mutex_unlock(_m)    g_mutex_unlock(&(_m))

/* Recursive lock capable mutex */
typedef GRecMutex fluid_rec_mutex_t;
#define fluid_rec_mutex_init(_m)      g_rec_mutex_init(&(_m))
#define fluid_rec_mutex_destroy(_m)   g_rec_mutex_clear(&(_m))
#define fluid_rec_mutex_lock(_m)      g_rec_mutex_lock(&(_m))
#define fluid_rec_mutex_unlock(_m)    g_rec_mutex_unlock(&(_m))

/* Dynamically allocated mutex suitable for fluid_cond_t use */
typedef GMutex    fluid_cond_mutex_t;
#define fluid_cond_mutex_lock(m)        g_mutex_lock(m)
#define fluid_cond_mutex_unlock(m)      g_mutex_unlock(m)

static FLUID_INLINE fluid_cond_mutex_t *
new_fluid_cond_mutex (void)
{
  GMutex *mutex;
  mutex = g_new (GMutex, 1);
  g_mutex_init (mutex);
  return (mutex);
}

static FLUID_INLINE void
delete_fluid_cond_mutex (fluid_cond_mutex_t *m)
{
  fluid_return_if_fail(m != NULL);
  g_mutex_clear (m);
  g_free (m);
}

/* Thread condition signaling */
typedef GCond fluid_cond_t;
#define fluid_cond_signal(cond)         g_cond_signal(cond)
#define fluid_cond_broadcast(cond)      g_cond_broadcast(cond)
#define fluid_cond_wait(cond, mutex)    g_cond_wait(cond, mutex)

static FLUID_INLINE fluid_cond_t *
new_fluid_cond (void)
{
  GCond *cond;
  cond = g_new (GCond, 1);
  g_cond_init (cond);
  return (cond);
}

static FLUID_INLINE void
delete_fluid_cond (fluid_cond_t *cond)
{
  fluid_return_if_fail(cond != NULL);
  g_cond_clear (cond);
  g_free (cond);
}

/* Thread private data */

typedef GPrivate fluid_private_t;
#define fluid_private_init(_priv)                  memset (&_priv, 0, sizeof (_priv))
#define fluid_private_free(_priv)
#define fluid_private_get(_priv)                   g_private_get(&(_priv))
#define fluid_private_set(_priv, _data)            g_private_set(&(_priv), _data)

#else

/* glib prior to 2.32 */

/* Regular mutex */
typedef GStaticMutex fluid_mutex_t;
#define FLUID_MUTEX_INIT          G_STATIC_MUTEX_INIT
#define fluid_mutex_destroy(_m)   g_static_mutex_free(&(_m))
#define fluid_mutex_lock(_m)      g_static_mutex_lock(&(_m))
#define fluid_mutex_unlock(_m)    g_static_mutex_unlock(&(_m))

#define fluid_mutex_init(_m)      do { \
  if (!g_thread_supported ()) g_thread_init (NULL); \
  g_static_mutex_init (&(_m)); \
} while(0)

/* Recursive lock capable mutex */
typedef GStaticRecMutex fluid_rec_mutex_t;
#define fluid_rec_mutex_destroy(_m)   g_static_rec_mutex_free(&(_m))
#define fluid_rec_mutex_lock(_m)      g_static_rec_mutex_lock(&(_m))
#define fluid_rec_mutex_unlock(_m)    g_static_rec_mutex_unlock(&(_m))

#define fluid_rec_mutex_init(_m)      do { \
  if (!g_thread_supported ()) g_thread_init (NULL); \
  g_static_rec_mutex_init (&(_m)); \
} while(0)

/* Dynamically allocated mutex suitable for fluid_cond_t use */
typedef GMutex    fluid_cond_mutex_t;
#define delete_fluid_cond_mutex(m)      g_mutex_free(m)
#define fluid_cond_mutex_lock(m)        g_mutex_lock(m)
#define fluid_cond_mutex_unlock(m)      g_mutex_unlock(m)

static FLUID_INLINE fluid_cond_mutex_t *
new_fluid_cond_mutex (void)
{
  if (!g_thread_supported ()) g_thread_init (NULL);
  return g_mutex_new ();
}

/* Thread condition signaling */
typedef GCond fluid_cond_t;
fluid_cond_t *new_fluid_cond (void);
#define delete_fluid_cond(cond)         g_cond_free(cond)
#define fluid_cond_signal(cond)         g_cond_signal(cond)
#define fluid_cond_broadcast(cond)      g_cond_broadcast(cond)
#define fluid_cond_wait(cond, mutex)    g_cond_wait(cond, mutex)

/* Thread private data */
typedef GStaticPrivate fluid_private_t;
#define fluid_private_get(_priv)                   g_static_private_get(&(_priv))
#define fluid_private_set(_priv, _data)            g_static_private_set(&(_priv), _data, NULL)
#define fluid_private_free(_priv)                  g_static_private_free(&(_priv))

#define fluid_private_init(_priv)                  do { \
  if (!g_thread_supported ()) g_thread_init (NULL); \
  g_static_private_init (&(_priv)); \
} while(0)

#endif


/* Atomic operations */

#define fluid_atomic_int_inc(_pi) g_atomic_int_inc(_pi)
#define fluid_atomic_int_get(_pi) g_atomic_int_get(_pi)
#define fluid_atomic_int_set(_pi, _val) g_atomic_int_set(_pi, _val)
#define fluid_atomic_int_dec_and_test(_pi) g_atomic_int_dec_and_test(_pi)
#define fluid_atomic_int_compare_and_exchange(_pi, _old, _new) \
  g_atomic_int_compare_and_exchange(_pi, _old, _new)

#if GLIB_MAJOR_VERSION > 2 || (GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION >= 30)
#define fluid_atomic_int_exchange_and_add(_pi, _add) \
  g_atomic_int_add(_pi, _add)
#define fluid_atomic_int_add(_pi, _add) \
  g_atomic_int_add(_pi, _add)
#else
#define fluid_atomic_int_exchange_and_add(_pi, _add) \
  g_atomic_int_exchange_and_add(_pi, _add)
#define fluid_atomic_int_add(_pi, _add) \
  g_atomic_int_exchange_and_add(_pi, _add)
#endif

#define fluid_atomic_pointer_get(_pp)           g_atomic_pointer_get(_pp)
#define fluid_atomic_pointer_set(_pp, val)      g_atomic_pointer_set(_pp, val)
#define fluid_atomic_pointer_compare_and_exchange(_pp, _old, _new) \
  g_atomic_pointer_compare_and_exchange(_pp, _old, _new)

static FLUID_INLINE void
fluid_atomic_float_set(volatile float *fptr, float val)
{
  int32_t ival;
  memcpy (&ival, &val, 4);
  fluid_atomic_int_set ((volatile int *)fptr, ival);
}

static FLUID_INLINE float
fluid_atomic_float_get(volatile float *fptr)
{
  int32_t ival;
  float fval;
  ival = fluid_atomic_int_get ((volatile int *)fptr);
  memcpy (&fval, &ival, 4);
  return fval;
}


/* Threads */

/* other thread implementations might change this for their needs */
typedef void* fluid_thread_return_t;
/* static return value for thread functions which requires a return value */
#define FLUID_THREAD_RETURN_VALUE (NULL)

typedef GThread fluid_thread_t;
typedef fluid_thread_return_t (*fluid_thread_func_t)(void* data);

#define FLUID_THREAD_ID_NULL            NULL                    /* A NULL "ID" value */
#define fluid_thread_id_t               GThread *               /* Data type for a thread ID */
#define fluid_thread_get_id()           g_thread_self()         /* Get unique "ID" for current thread */

fluid_thread_t* new_fluid_thread(const char *name, fluid_thread_func_t func, void *data,
                                 int prio_level, int detach);
void delete_fluid_thread(fluid_thread_t* thread);
void fluid_thread_self_set_prio (int prio_level);
int fluid_thread_join(fluid_thread_t* thread);

/* Dynamic Module Loading, currently only used by LADSPA subsystem */
#ifdef LADSPA

typedef GModule fluid_module_t;

#define fluid_module_open(_name)        g_module_open((_name), G_MODULE_BIND_LOCAL)
#define fluid_module_close(_mod)        g_module_close(_mod)
#define fluid_module_error()            g_module_error()
#define fluid_module_name(_mod)         g_module_name(_mod)
#define fluid_module_symbol(_mod, _name, _ptr) g_module_symbol((_mod), (_name), (_ptr))

#endif /* LADSPA */

/* Sockets and I/O */

fluid_istream_t fluid_get_stdin (void);
fluid_ostream_t fluid_get_stdout (void);
int fluid_istream_readline(fluid_istream_t in, fluid_ostream_t out, char* prompt, char* buf, int len);
int fluid_ostream_printf (fluid_ostream_t out, const char* format, ...);

/* The function should return 0 if no error occured, non-zero
   otherwise. If the function return non-zero, the socket will be
   closed by the server. */
typedef int (*fluid_server_func_t)(void* data, fluid_socket_t client_socket, char* addr);

fluid_server_socket_t* new_fluid_server_socket(int port, fluid_server_func_t func, void* data);
void delete_fluid_server_socket(fluid_server_socket_t* sock);
int fluid_server_socket_join(fluid_server_socket_t* sock);
void fluid_socket_close(fluid_socket_t sock);
fluid_istream_t fluid_socket_get_istream(fluid_socket_t sock);
fluid_ostream_t fluid_socket_get_ostream(fluid_socket_t sock);

/* File access */
typedef GStatBuf fluid_stat_buf_t;
#define fluid_stat(_filename, _statbuf)   g_stat((_filename), (_statbuf))


/* Profiling */
#if WITH_PROFILING
/** profiling interface beetween Profiling command shell and Audio
    rendering  API (FluidProfile_0004.pdf- 3.2.2)
*/

/*
  -----------------------------------------------------------------------------
  Shell task side |    Profiling interface              |  Audio task side
  -----------------------------------------------------------------------------
  profiling       |    Internal    |      |             |      Audio
  command   <---> |<-- profling -->| Data |<--macros -->| <--> rendering
  shell           |    API         |      |             |      API

*/

/* default parameters for shell command "prof_start" in fluid_sys.c */
#define FLUID_PROFILE_DEFAULT_BANK 0       /* default bank */
#define FLUID_PROFILE_DEFAULT_PROG 16      /* default prog (organ) */
#define FLUID_PROFILE_FIRST_KEY 12         /* first key generated */
#define FLUID_PROFILE_LAST_KEY 108         /* last key generated */
#define FLUID_PROFILE_DEFAULT_VEL 64       /* default note velocity */
#define FLUID_PROFILE_VOICE_ATTEN -0.04f   /* gain attenuation per voice (dB) */


#define FLUID_PROFILE_DEFAULT_PRINT 0      /* default print mode */
#define FLUID_PROFILE_DEFAULT_N_PROF 1     /* default number of measures */
#define FLUID_PROFILE_DEFAULT_DURATION 500 /* default duration (ms)  */


extern unsigned short fluid_profile_notes; /* number of generated notes */
extern unsigned char fluid_profile_bank;   /* bank,prog preset used by */
extern unsigned char fluid_profile_prog;   /* generated notes */
extern unsigned char fluid_profile_print;  /* print mode */

extern unsigned short fluid_profile_n_prof;/* number of measures */
extern unsigned short fluid_profile_dur;   /* measure duration in ms */
extern fluid_atomic_int_t fluid_profile_lock ; /* lock between multiple shell */
/**/

/*----------------------------------------------
  Internal profiling API (in fluid_sys.c)
-----------------------------------------------*/
/* Starts a profiling measure used in shell command "prof_start" */
void fluid_profile_start_stop(unsigned int end_ticks, short clear_data);

/* Returns status used in shell command "prof_start" */
int fluid_profile_get_status(void);

/* Prints profiling data used in shell command "prof_start" */
void fluid_profiling_print_data(double sample_rate, fluid_ostream_t out);

/* Returns True if profiling cancellation has been requested */
int fluid_profile_is_cancel_req(void);

/* For OS that implement <ENTER> key for profile cancellation:
 1) Adds #define FLUID_PROFILE_CANCEL
 2) Adds the necessary code inside fluid_profile_is_cancel() see fluid_sys.c
*/
#if defined(WIN32)      /* Profile cancellation is supported for Windows */
#define FLUID_PROFILE_CANCEL

#elif defined(__OS2__)  /* OS/2 specific stuff */
/* Profile cancellation isn't yet supported for OS2 */

#else   /* POSIX stuff */
#define FLUID_PROFILE_CANCEL /* Profile cancellation is supported for linux */
#include <unistd.h> /* STDIN_FILENO */
#include <sys/select.h> /* select() */
#endif /* posix */

/* logging profiling data (used on synthesizer instance deletion) */
void fluid_profiling_print(void);

/*----------------------------------------------
  Profiling Data (in fluid_sys.c)
-----------------------------------------------*/
/** Profiling data. Keep track of min/avg/max values to profile a
    piece of code. */
typedef struct _fluid_profile_data_t
{
	const char* description;        /* name of the piece of code under profiling */
	double min, max, total;   /* duration (microsecond) */
	unsigned int count;       /* total count */
	unsigned int n_voices;    /* voices number */
	unsigned int n_samples;   /* audio samples number */
} fluid_profile_data_t;

enum
{
	/* commands/status  (profiling interface) */
	PROFILE_STOP,    /* command to stop a profiling measure */
	PROFILE_START,   /* command to start a profile measure */
	PROFILE_READY,   /* status to signal that a profiling measure has finished
	                    and ready to be printed */
	/*- State returned by fluid_profile_get_status() -*/
	/* between profiling commands and internal profiling API */
	PROFILE_RUNNING, /* a profiling measure is running */
	PROFILE_CANCELED,/* a profiling measure has been canceled */
};

/* Data interface */
extern unsigned char fluid_profile_status ;       /* command and status */
extern unsigned int fluid_profile_end_ticks;      /* ending position (in ticks) */
extern fluid_profile_data_t fluid_profile_data[]; /* Profiling data */

/*----------------------------------------------
  Probes macros
-----------------------------------------------*/
/** Macro to obtain a time reference used for the profiling */
#define fluid_profile_ref() fluid_utime()

/** Macro to create a variable and assign the current reference time for profiling.
 * So we don't get unused variable warnings when profiling is disabled. */
#define fluid_profile_ref_var(name)     double name = fluid_utime()

/**
 * Profile identifier numbers. List all the pieces of code you want to profile
 * here. Be sure to add an entry in the fluid_profile_data table in
 * fluid_sys.c
 */
enum
{
	FLUID_PROF_WRITE,
	FLUID_PROF_ONE_BLOCK,
	FLUID_PROF_ONE_BLOCK_CLEAR,
	FLUID_PROF_ONE_BLOCK_VOICE,
	FLUID_PROF_ONE_BLOCK_VOICES,
	FLUID_PROF_ONE_BLOCK_REVERB,
	FLUID_PROF_ONE_BLOCK_CHORUS,
	FLUID_PROF_VOICE_NOTE,
	FLUID_PROF_VOICE_RELEASE,
	FLUID_PROFILE_NBR	/* number of profile probes */
};
/** Those macros are used to calculate the min/avg/max. Needs a profile number, a
    time reference, the voices and samples number. */

/* local macro : acquiere data */
#define fluid_profile_data(_num, _ref, voices, samples)\
{\
	double _now = fluid_utime();\
	double _delta = _now - _ref;\
	fluid_profile_data[_num].min = _delta < fluid_profile_data[_num].min ?\
                                   _delta : fluid_profile_data[_num].min; \
	fluid_profile_data[_num].max = _delta > fluid_profile_data[_num].max ?\
                                   _delta : fluid_profile_data[_num].max;\
	fluid_profile_data[_num].total += _delta;\
	fluid_profile_data[_num].count++;\
	fluid_profile_data[_num].n_voices += voices;\
	fluid_profile_data[_num].n_samples += samples;\
	_ref = _now;\
}

/** Macro to collect data, called from inner functions inside audio
    rendering API */
#define fluid_profile(_num, _ref, voices, samples)\
{\
	if ( fluid_profile_status == PROFILE_START)\
	{	/* acquires data */\
		fluid_profile_data(_num, _ref, voices, samples)\
	}\
}

/** Macro to collect data, called from audio rendering API (fluid_write_xxxx()).
 This macro control profiling ending position (in ticks).
*/
#define fluid_profile_write(_num, _ref, voices, samples)\
{\
	if (fluid_profile_status == PROFILE_START)\
	{\
		/* acquires data first: must be done before checking that profile is
           finished to ensure at least one valid data sample.
		*/\
		fluid_profile_data(_num, _ref, voices, samples)\
		if (fluid_synth_get_ticks(synth) >= fluid_profile_end_ticks)\
		{\
			/* profiling is finished */\
			fluid_profile_status = PROFILE_READY;\
		}\
	}\
}

#else

/* No profiling */
#define fluid_profiling_print()
#define fluid_profile_ref()  0
#define fluid_profile_ref_var(name)
#define fluid_profile(_num,_ref,voices, samples)
#define fluid_profile_write(_num,_ref, voices, samples)
#endif /* WITH_PROFILING */

/**

    Memory locking

    Memory locking is used to avoid swapping of the large block of
    sample data.
 */

#if defined(HAVE_SYS_MMAN_H) && !defined(__OS2__)
#define fluid_mlock(_p,_n)      mlock(_p, _n)
#define fluid_munlock(_p,_n)    munlock(_p,_n)
#else
#define fluid_mlock(_p,_n)      0
#define fluid_munlock(_p,_n)
#endif


/**

    Floating point exceptions

    fluid_check_fpe() checks for "unnormalized numbers" and other
    exceptions of the floating point processsor.
*/
#ifdef FPE_CHECK
#define fluid_check_fpe(expl) fluid_check_fpe_i386(expl)
#define fluid_clear_fpe() fluid_clear_fpe_i386()
#else
#define fluid_check_fpe(expl)
#define fluid_clear_fpe()
#endif

unsigned int fluid_check_fpe_i386(char * explanation_in_case_of_fpe);
void fluid_clear_fpe_i386(void);

/* System control */
void fluid_msleep(unsigned int msecs);

#endif /* _FLUID_SYS_H */
