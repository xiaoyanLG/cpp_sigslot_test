// sigslot.h: Signal/Slot classes
//
// Written by Sarah Thompson (sarah@telergy.com) 2002.
//
// License: Public domain. You are free to use this code however you like, with the proviso that
//          the author takes on no responsibility or liability for any use.
//
// QUICK DOCUMENTATION
//
//                (see also the full documentation at http://sigslot.sourceforge.net/)
//
//        #define switches
//            SIGSLOT_PURE_ISO            - Define this to force ISO C++ compliance. This also disables
//                                          all of the thread safety support on platforms where it is
//                                          available.
//
//            SIGSLOT_USE_POSIX_THREADS   - Force use of Posix threads when using a C++ compiler other than
//                                          gcc on a platform that supports Posix threads. (When using gcc,
//                                          this is the default - use SIGSLOT_PURE_ISO to disable this if
//                                          necessary)
//
//            SIGSLOT_DEFAULT_MT_POLICY   - Where thread support is enabled, this defaults to multi_threaded_global.
//                                          Otherwise, the default is single_threaded. #define this yourself to
//                                          override the default. In pure ISO mode, anything other than
//                                          single_threaded will cause a compiler error.
//
//        PLATFORM NOTES
//
//            Win32                       - On Win32, the WIN32 symbol must be #defined. Most mainstream
//                                          compilers do this by default, but you may need to define it
//                                          yourself if your build environment is less standard. This causes
//                                          the Win32 thread support to be compiled in and used automatically.
//
//            Unix/Linux/BSD, etc.        - If you're using gcc, it is assumed that you have Posix threads
//                                          available, so they are used automatically. You can override this
//                                          (as under Windows) with the SIGSLOT_PURE_ISO switch. If you're using
//                                          something other than gcc but still want to use Posix threads, you
//                                          need to #define SIGSLOT_USE_POSIX_THREADS.
//
//            ISO C++                     - If none of the supported platforms are detected, or if
//                                          SIGSLOT_PURE_ISO is defined, all multithreading support is turned off,
//                                          along with any code that might cause a pure ISO C++ environment to
//                                          complain. Before you ask, gcc -ansi -pedantic won't compile this
//                                          library, but gcc -ansi is fine. Pedantic mode seems to throw a lot of
//                                          errors that aren't really there. If you feel like investigating this,
//                                          please contact the author.
//
//
//        THREADING MODES
//
//            single_threaded             - Your program is assumed to be single threaded from the point of view
//                                          of signal/slot usage (i.e. all objects using signals and slots are
//                                          created and destroyed from a single thread). Behaviour if objects are
//                                          destroyed concurrently is undefined (i.e. you'll get the occasional
//                                          segmentation fault/memory exception).
//
//            multi_threaded_global       - Your program is assumed to be multi threaded. Objects using signals and
//                                          slots can be safely created and destroyed from any thread, even when
//                                          connections exist. In multi_threaded_global mode, this is achieved by a
//                                          single global mutex (actually a critical section on Windows because they
//                                          are faster). This option uses less OS resources, but results in more
//                                          opportunities for contention, possibly resulting in more context switches
//                                          than are strictly necessary.
//
//            multi_threaded_local        - Behaviour in this mode is essentially the same as multi_threaded_global,
//                                          except that each signal, and each object that inherits has_slots, all
//                                          have their own mutex/critical section. In practice, this means that
//                                          mutex collisions (and hence context switches) only happen if they are
//                                          absolutely essential. However, on some platforms, creating a lot of
//                                          mutexes can slow down the whole OS, so use this option with care.
//
//        USING THE LIBRARY
//
//        See the full documentation at http://sigslot.sourceforge.net/
//
//

#ifndef SIGSLOT_H__
#define SIGSLOT_H__

#include <set>
#include <list>

#if defined(SIGSLOT_PURE_ISO) || (!defined(WIN32) && !defined(__GNUG__) && !defined(SIGSLOT_USE_POSIX_THREADS))
#    define _SIGSLOT_SINGLE_THREADED
#elif defined(WIN32)
#    define _SIGSLOT_HAS_WIN32_THREADS
#    include <windows.h>
#elif defined(__GNUG__) || defined(SIGSLOT_USE_POSIX_THREADS)
#    define _SIGSLOT_HAS_POSIX_THREADS
#    include <pthread.h>
#else
#    define _SIGSLOT_SINGLE_THREADED
#endif

#ifndef SIGSLOT_DEFAULT_MT_POLICY
#    ifdef _SIGSLOT_SINGLE_THREADED
#        define SIGSLOT_DEFAULT_MT_POLICY single_threaded
#    else
#        define SIGSLOT_DEFAULT_MT_POLICY multi_threaded_local
#    endif
#endif


namespace sigslot {

    class single_threaded
    {
    public:
        single_threaded()
        {
        }

        virtual ~single_threaded()
        {
        }

        virtual void lock()
        {
        }

        virtual void unlock()
        {
        }
    };

#ifdef _SIGSLOT_HAS_WIN32_THREADS
    // The multi threading policies only get compiled in if they are enabled.
    class multi_threaded_global
    {
    public:
        multi_threaded_global()
        {
            static bool isinitialised = false;

            if(!isinitialised)
            {
                InitializeCriticalSection(get_critsec());
                isinitialised = true;
            }
        }

        multi_threaded_global(const multi_threaded_global&)
        {
        }

        virtual ~multi_threaded_global()
        {
        }

        virtual void lock()
        {
            EnterCriticalSection(get_critsec());
        }

        virtual void unlock()
        {
            LeaveCriticalSection(get_critsec());
        }

    private:
        CRITICAL_SECTION* get_critsec()
        {
            static CRITICAL_SECTION g_critsec;
            return &g_critsec;
        }
    };

    class multi_threaded_local
    {
    public:
        multi_threaded_local()
        {
            InitializeCriticalSection(&m_critsec);
        }

        multi_threaded_local(const multi_threaded_local&)
        {
            InitializeCriticalSection(&m_critsec);
        }

        virtual ~multi_threaded_local()
        {
            DeleteCriticalSection(&m_critsec);
        }

        virtual void lock()
        {
            EnterCriticalSection(&m_critsec);
        }

        virtual void unlock()
        {
            LeaveCriticalSection(&m_critsec);
        }

    private:
        CRITICAL_SECTION m_critsec;
    };
#endif // _SIGSLOT_HAS_WIN32_THREADS

#ifdef _SIGSLOT_HAS_POSIX_THREADS
    // The multi threading policies only get compiled in if they are enabled.
    class multi_threaded_global
    {
    public:
        multi_threaded_global()
        {
            pthread_mutex_init(get_mutex(), NULL);
        }

        multi_threaded_global(const multi_threaded_global&)
        {
        }

        virtual ~multi_threaded_global()
        {
        }

        virtual void lock()
        {
            pthread_mutex_lock(get_mutex());
        }

        virtual void unlock()
        {
            pthread_mutex_unlock(get_mutex());
        }

    private:
        pthread_mutex_t* get_mutex()
        {
            static pthread_mutex_t g_mutex;
            return &g_mutex;
        }
    };

    class multi_threaded_local
    {
    public:
        multi_threaded_local()
        {
            pthread_mutex_init(&m_mutex, NULL);
        }

        multi_threaded_local(const multi_threaded_local&)
        {
            pthread_mutex_init(&m_mutex, NULL);
        }

        virtual ~multi_threaded_local()
        {
            pthread_mutex_destroy(&m_mutex);
        }

        virtual void lock()
        {
            pthread_mutex_lock(&m_mutex);
        }

        virtual void unlock()
        {
            pthread_mutex_unlock(&m_mutex);
        }

    private:
        pthread_mutex_t m_mutex;
    };
#endif // _SIGSLOT_HAS_POSIX_THREADS

    template<class mt_policy>
    class lock_block
    {
    public:
        mt_policy *m_mutex;

        lock_block(mt_policy *mtx)
            : m_mutex(mtx)
        {
            m_mutex->lock();
        }

        ~lock_block()
        {
            m_mutex->unlock();
        }
    };

    template<class mt_policy>
    class has_slots;

    template<class mt_policy>
    class _signal_base : public mt_policy
    {
    public:
        virtual void disconnect(has_slots<mt_policy>* pslot) = 0;
    };

    template<class mt_policy>
    class _connection_base0
    {
    public:
        virtual ~_connection_base0() {}
        virtual has_slots<mt_policy>* getdest() const = 0;
        virtual void emit() = 0;
    };

    template<class arg1_type, class mt_policy>
    class _connection_base1
    {
    public:
        virtual ~_connection_base1() {}
        virtual has_slots<mt_policy>* getdest() const = 0;
        virtual void emit(arg1_type) = 0;
    };

    template<class arg1_type, class arg2_type, class mt_policy>
    class _connection_base2
    {
    public:
        virtual ~_connection_base2() {}
        virtual has_slots<mt_policy>* getdest() const = 0;
        virtual void emit(arg1_type, arg2_type) = 0;
    };

    template<class arg1_type, class arg2_type, class arg3_type, class mt_policy>
    class _connection_base3
    {
    public:
        virtual ~_connection_base3() {}
        virtual has_slots<mt_policy>* getdest() const = 0;
        virtual void emit(arg1_type, arg2_type, arg3_type) = 0;
    };

    template<class arg1_type, class arg2_type, class arg3_type, class arg4_type, class mt_policy>
    class _connection_base4
    {
    public:
        virtual ~_connection_base4() {}
        virtual has_slots<mt_policy>* getdest() const = 0;
        virtual void emit(arg1_type, arg2_type, arg3_type, arg4_type) = 0;
    };

    template<class arg1_type, class arg2_type, class arg3_type, class arg4_type,
    class arg5_type, class mt_policy>
    class _connection_base5
    {
    public:
        virtual ~_connection_base5() {}
        virtual has_slots<mt_policy>* getdest() const = 0;
        virtual void emit(arg1_type, arg2_type, arg3_type, arg4_type,
            arg5_type) = 0;
    };

    template<class arg1_type, class arg2_type, class arg3_type, class arg4_type,
    class arg5_type, class arg6_type, class mt_policy>
    class _connection_base6
    {
    public:
        virtual ~_connection_base6() {}
        virtual has_slots<mt_policy>* getdest() const = 0;
        virtual void emit(arg1_type, arg2_type, arg3_type, arg4_type, arg5_type,
            arg6_type) = 0;
    };

    template<class arg1_type, class arg2_type, class arg3_type, class arg4_type,
    class arg5_type, class arg6_type, class arg7_type, class mt_policy>
    class _connection_base7
    {
    public:
        virtual ~_connection_base7() {}
        virtual has_slots<mt_policy>* getdest() const = 0;
        virtual void emit(arg1_type, arg2_type, arg3_type, arg4_type, arg5_type,
            arg6_type, arg7_type) = 0;
    };

    template<class arg1_type, class arg2_type, class arg3_type, class arg4_type,
    class arg5_type, class arg6_type, class arg7_type, class arg8_type, class mt_policy>
    class _connection_base8
    {
    public:
        virtual ~_connection_base8() {}
        virtual has_slots<mt_policy>* getdest() const = 0;
        virtual void emit(arg1_type, arg2_type, arg3_type, arg4_type, arg5_type,
            arg6_type, arg7_type, arg8_type) = 0;
    };

    template<class dest_type, class mt_policy>
    class _connection0 : public _connection_base0<mt_policy>
    {
    public:
        _connection0(dest_type* pobject, void (dest_type::*pmemfun)())
        {
            m_pobject = pobject;
            m_pmemfun = pmemfun;
        }

        virtual void emit()
        {
            (m_pobject->*m_pmemfun)();
        }

        virtual has_slots<mt_policy>* getdest() const
        {
            return m_pobject;
        }

    private:
        dest_type* m_pobject;
        void (dest_type::* m_pmemfun)();
    };

    template<class dest_type, class arg1_type, class mt_policy>
    class _connection1 : public _connection_base1<arg1_type, mt_policy>
    {
    public:
        _connection1(dest_type* pobject, void (dest_type::*pmemfun)(arg1_type))
        {
            m_pobject = pobject;
            m_pmemfun = pmemfun;
        }

        virtual void emit(arg1_type a1)
        {
            (m_pobject->*m_pmemfun)(a1);
        }

        virtual has_slots<mt_policy>* getdest() const
        {
            return m_pobject;
        }

    private:
        dest_type* m_pobject;
        void (dest_type::* m_pmemfun)(arg1_type);
    };

    template<class dest_type, class arg1_type, class arg2_type, class mt_policy>
    class _connection2 : public _connection_base2<arg1_type, arg2_type, mt_policy>
    {
    public:
        _connection2(dest_type* pobject, void (dest_type::*pmemfun)(arg1_type,
            arg2_type))
        {
            m_pobject = pobject;
            m_pmemfun = pmemfun;
        }

        virtual void emit(arg1_type a1, arg2_type a2)
        {
            (m_pobject->*m_pmemfun)(a1, a2);
        }

        virtual has_slots<mt_policy>* getdest() const
        {
            return m_pobject;
        }

    private:
        dest_type* m_pobject;
        void (dest_type::* m_pmemfun)(arg1_type, arg2_type);
    };

    template<class dest_type, class arg1_type, class arg2_type, class arg3_type, class mt_policy>
    class _connection3 : public _connection_base3<arg1_type, arg2_type, arg3_type, mt_policy>
    {
    public:
        _connection3(dest_type* pobject, void (dest_type::*pmemfun)(arg1_type,
            arg2_type, arg3_type))
        {
            m_pobject = pobject;
            m_pmemfun = pmemfun;
        }

        virtual void emit(arg1_type a1, arg2_type a2, arg3_type a3)
        {
            (m_pobject->*m_pmemfun)(a1, a2, a3);
        }

        virtual has_slots<mt_policy>* getdest() const
        {
            return m_pobject;
        }

    private:
        dest_type* m_pobject;
        void (dest_type::* m_pmemfun)(arg1_type, arg2_type, arg3_type);
    };

    template<class dest_type, class arg1_type, class arg2_type, class arg3_type,
    class arg4_type, class mt_policy>
    class _connection4 : public _connection_base4<arg1_type, arg2_type,
        arg3_type, arg4_type, mt_policy>
    {
    public:
        _connection4(dest_type* pobject, void (dest_type::*pmemfun)(arg1_type,
            arg2_type, arg3_type, arg4_type))
        {
            m_pobject = pobject;
            m_pmemfun = pmemfun;
        }

        virtual void emit(arg1_type a1, arg2_type a2, arg3_type a3,
            arg4_type a4)
        {
            (m_pobject->*m_pmemfun)(a1, a2, a3, a4);
        }

        virtual has_slots<mt_policy>* getdest() const
        {
            return m_pobject;
        }

    private:
        dest_type* m_pobject;
        void (dest_type::* m_pmemfun)(arg1_type, arg2_type, arg3_type,
            arg4_type);
    };

    template<class dest_type, class arg1_type, class arg2_type, class arg3_type,
    class arg4_type, class arg5_type, class mt_policy>
    class _connection5 : public _connection_base5<arg1_type, arg2_type,
        arg3_type, arg4_type, arg5_type, mt_policy>
    {
    public:
        _connection5(dest_type* pobject, void (dest_type::*pmemfun)(arg1_type,
            arg2_type, arg3_type, arg4_type, arg5_type))
        {
            m_pobject = pobject;
            m_pmemfun = pmemfun;
        }

        virtual void emit(arg1_type a1, arg2_type a2, arg3_type a3, arg4_type a4,
            arg5_type a5)
        {
            (m_pobject->*m_pmemfun)(a1, a2, a3, a4, a5);
        }

        virtual has_slots<mt_policy>* getdest() const
        {
            return m_pobject;
        }

    private:
        dest_type* m_pobject;
        void (dest_type::* m_pmemfun)(arg1_type, arg2_type, arg3_type, arg4_type,
            arg5_type);
    };

    template<class dest_type, class arg1_type, class arg2_type, class arg3_type,
    class arg4_type, class arg5_type, class arg6_type, class mt_policy>
    class _connection6 : public _connection_base6<arg1_type, arg2_type,
        arg3_type, arg4_type, arg5_type, arg6_type, mt_policy>
    {
    public:
        _connection6(dest_type* pobject, void (dest_type::*pmemfun)(arg1_type,
            arg2_type, arg3_type, arg4_type, arg5_type, arg6_type))
        {
            m_pobject = pobject;
            m_pmemfun = pmemfun;
        }

        virtual void emit(arg1_type a1, arg2_type a2, arg3_type a3, arg4_type a4,
            arg5_type a5, arg6_type a6)
        {
            (m_pobject->*m_pmemfun)(a1, a2, a3, a4, a5, a6);
        }

        virtual has_slots<mt_policy>* getdest() const
        {
            return m_pobject;
        }

    private:
        dest_type* m_pobject;
        void (dest_type::* m_pmemfun)(arg1_type, arg2_type, arg3_type, arg4_type,
            arg5_type, arg6_type);
    };

    template<class dest_type, class arg1_type, class arg2_type, class arg3_type,
    class arg4_type, class arg5_type, class arg6_type, class arg7_type, class mt_policy>
    class _connection7 : public _connection_base7<arg1_type, arg2_type,
        arg3_type, arg4_type, arg5_type, arg6_type, arg7_type, mt_policy>
    {
    public:
        _connection7(dest_type* pobject, void (dest_type::*pmemfun)(arg1_type,
            arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, arg7_type))
        {
            m_pobject = pobject;
            m_pmemfun = pmemfun;
        }

        virtual void emit(arg1_type a1, arg2_type a2, arg3_type a3, arg4_type a4,
            arg5_type a5, arg6_type a6, arg7_type a7)
        {
            (m_pobject->*m_pmemfun)(a1, a2, a3, a4, a5, a6, a7);
        }

        virtual has_slots<mt_policy>* getdest() const
        {
            return m_pobject;
        }

    private:
        dest_type* m_pobject;
        void (dest_type::* m_pmemfun)(arg1_type, arg2_type, arg3_type, arg4_type,
            arg5_type, arg6_type, arg7_type);
    };

    template<class dest_type, class arg1_type, class arg2_type, class arg3_type,
    class arg4_type, class arg5_type, class arg6_type, class arg7_type,
    class arg8_type, class mt_policy>
    class _connection8 : public _connection_base8<arg1_type, arg2_type,
        arg3_type, arg4_type, arg5_type, arg6_type, arg7_type, arg8_type, mt_policy>
    {
    public:
        _connection8(dest_type* pobject, void (dest_type::*pmemfun)(arg1_type,
            arg2_type, arg3_type, arg4_type, arg5_type, arg6_type,
            arg7_type, arg8_type))
        {
            m_pobject = pobject;
            m_pmemfun = pmemfun;
        }

        virtual void emit(arg1_type a1, arg2_type a2, arg3_type a3, arg4_type a4,
            arg5_type a5, arg6_type a6, arg7_type a7, arg8_type a8)
        {
            (m_pobject->*m_pmemfun)(a1, a2, a3, a4, a5, a6, a7, a8);
        }

        virtual has_slots<mt_policy>* getdest() const
        {
            return m_pobject;
        }

    private:
        dest_type* m_pobject;
        void (dest_type::* m_pmemfun)(arg1_type, arg2_type, arg3_type, arg4_type,
            arg5_type, arg6_type, arg7_type, arg8_type);
    };

    template<class mt_policy= SIGSLOT_DEFAULT_MT_POLICY>
    class signal0 : public _signal_base<mt_policy>
    {
    public:
        typedef typename std::list<_connection_base0<mt_policy> *>  connections_list;
        ~signal0()
        {
            signal_destroy(true);
        }

        template<class desttype>
        void connect(desttype* pclass, void (desttype::*pmemfun)())
        {
            lock_block<mt_policy> lock(this);
            _connection0<desttype, mt_policy>* conn =
                new _connection0<desttype, mt_policy>(pclass, pmemfun);
            m_connected_slots.push_back(conn);
            pclass->signal_connect(this);
        }

        void emit()
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::const_iterator itNext, it = m_connected_slots.begin();
            typename connections_list::const_iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                itNext = it;
                ++itNext;

                (*it)->emit();

                it = itNext;
            }
        }

        void operator()()
        {
            emit();
        }

        void disconnect(has_slots<mt_policy>* pslot = NULL)
        {
            signal_destroy(false, pslot);
        }

    private:
        void signal_destroy(bool destroy, has_slots<mt_policy>* pslot = NULL)
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::iterator it = m_connected_slots.begin();
            typename connections_list::iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                typename connections_list::iterator itNext = it;
                ++itNext;

                if (destroy)
                    (*it)->getdest()->signal_disconnect(this);

                if(pslot == NULL || (*it)->getdest() == pslot)
                {
                    delete *it;
                    m_connected_slots.erase(it);
                }

                it = itNext;
            }
        }

    protected:
        connections_list m_connected_slots;
    };

    template<class arg1_type, class mt_policy= SIGSLOT_DEFAULT_MT_POLICY>
    class signal1 : public _signal_base<mt_policy>
    {
    public:
        typedef typename std::list<_connection_base1<arg1_type, mt_policy> *>  connections_list;

        ~signal1()
        {
            signal_destroy(true);
        }

        template<class desttype>
        void connect(desttype* pclass, void (desttype::*pmemfun)(arg1_type))
        {
            lock_block<mt_policy> lock(this);
            _connection1<desttype, arg1_type, mt_policy>* conn =
                new _connection1<desttype, arg1_type, mt_policy>(pclass, pmemfun);
            m_connected_slots.push_back(conn);
            pclass->signal_connect(this);
        }

        void emit(arg1_type a1)
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::const_iterator itNext, it = m_connected_slots.begin();
            typename connections_list::const_iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                itNext = it;
                ++itNext;

                (*it)->emit(a1);

                it = itNext;
            }
        }

        void operator()(arg1_type a1)
        {
            emit(a1);
        }

        void disconnect(has_slots<mt_policy>* pslot = NULL)
        {
            signal_destroy(false, pslot);
        }

    private:
        void signal_destroy(bool destroy, has_slots<mt_policy>* pslot = NULL)
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::iterator it = m_connected_slots.begin();
            typename connections_list::iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                typename connections_list::iterator itNext = it;
                ++itNext;

                if (destroy)
                    (*it)->getdest()->signal_disconnect(this);

                if(pslot == NULL || (*it)->getdest() == pslot)
                {
                    delete *it;
                    m_connected_slots.erase(it);
                }

                it = itNext;
            }
        }


    protected:
        connections_list m_connected_slots;
    };

    template<class arg1_type, class arg2_type, class mt_policy= SIGSLOT_DEFAULT_MT_POLICY>
    class signal2 : public _signal_base<mt_policy>
    {
    public:
        typedef typename std::list<_connection_base2<arg1_type, arg2_type, mt_policy> *>
            connections_list;

        ~signal2()
        {
            signal_destroy(true);
        }

        template<class desttype>
        void connect(desttype* pclass, void (desttype::*pmemfun)(arg1_type,
        arg2_type))
        {
            lock_block<mt_policy> lock(this);
            _connection2<desttype, arg1_type, arg2_type, mt_policy>* conn = new
                _connection2<desttype, arg1_type, arg2_type, mt_policy>(pclass, pmemfun);
            m_connected_slots.push_back(conn);
            pclass->signal_connect(this);
        }

        void emit(arg1_type a1, arg2_type a2)
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::const_iterator itNext, it = m_connected_slots.begin();
            typename connections_list::const_iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                itNext = it;
                ++itNext;

                (*it)->emit(a1, a2);

                it = itNext;
            }
        }

        void operator()(arg1_type a1, arg2_type a2)
        {
            emit(a1, a2);
        }

        void disconnect(has_slots<mt_policy>* pslot = NULL)
        {
            signal_destroy(false, pslot);
        }

    private:
        void signal_destroy(bool destroy, has_slots<mt_policy>* pslot = NULL)
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::iterator it = m_connected_slots.begin();
            typename connections_list::iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                typename connections_list::iterator itNext = it;
                ++itNext;

                if (destroy)
                    (*it)->getdest()->signal_disconnect(this);

                if(pslot == NULL || (*it)->getdest() == pslot)
                {
                    delete *it;
                    m_connected_slots.erase(it);
                }

                it = itNext;
            }
        }

    protected:
        connections_list m_connected_slots;
    };

    template<class arg1_type, class arg2_type, class arg3_type, class mt_policy= SIGSLOT_DEFAULT_MT_POLICY>
    class signal3 : public _signal_base<mt_policy>
    {
    public:
        typedef typename std::list<_connection_base3<arg1_type, arg2_type, arg3_type, mt_policy> *>
            connections_list;

        ~signal3()
        {
            signal_destroy(true);
        }

        template<class desttype>
        void connect(desttype* pclass, void (desttype::*pmemfun)(arg1_type,
        arg2_type, arg3_type))
        {
            lock_block<mt_policy> lock(this);
            _connection3<desttype, arg1_type, arg2_type, arg3_type, mt_policy>* conn =
                new _connection3<desttype, arg1_type, arg2_type, arg3_type, mt_policy>(pclass,
                pmemfun);
            m_connected_slots.push_back(conn);
            pclass->signal_connect(this);
        }

        void emit(arg1_type a1, arg2_type a2, arg3_type a3)
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::const_iterator itNext, it = m_connected_slots.begin();
            typename connections_list::const_iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                itNext = it;
                ++itNext;

                (*it)->emit(a1, a2, a3);

                it = itNext;
            }
        }

        void operator()(arg1_type a1, arg2_type a2, arg3_type a3)
        {
            emit(a1, a2, a3);
        }

        void disconnect(has_slots<mt_policy>* pslot = NULL)
        {
            signal_destroy(false, pslot);
        }

    private:
        void signal_destroy(bool destroy, has_slots<mt_policy>* pslot = NULL)
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::iterator it = m_connected_slots.begin();
            typename connections_list::iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                typename connections_list::iterator itNext = it;
                ++itNext;

                if (destroy)
                    (*it)->getdest()->signal_disconnect(this);

                if(pslot == NULL || (*it)->getdest() == pslot)
                {
                    delete *it;
                    m_connected_slots.erase(it);
                }

                it = itNext;
            }
        }

    protected:
        connections_list m_connected_slots;
    };

    template<class arg1_type, class arg2_type, class arg3_type, class arg4_type, class mt_policy= SIGSLOT_DEFAULT_MT_POLICY>
    class signal4 : public _signal_base<mt_policy>
    {
    public:
        typedef typename std::list<_connection_base4<arg1_type, arg2_type, arg3_type,
            arg4_type, mt_policy> *>  connections_list;

        ~signal4()
        {
            signal_destroy(true);
        }

        template<class desttype>
        void connect(desttype* pclass, void (desttype::*pmemfun)(arg1_type,
        arg2_type, arg3_type, arg4_type))
        {
            lock_block<mt_policy> lock(this);
            _connection4<desttype, arg1_type, arg2_type, arg3_type, arg4_type, mt_policy>*
                conn = new _connection4<desttype, arg1_type, arg2_type, arg3_type,
                arg4_type, mt_policy>(pclass, pmemfun);
            m_connected_slots.push_back(conn);
            pclass->signal_connect(this);
        }

        void emit(arg1_type a1, arg2_type a2, arg3_type a3, arg4_type a4)
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::const_iterator itNext, it = m_connected_slots.begin();
            typename connections_list::const_iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                itNext = it;
                ++itNext;

                (*it)->emit(a1, a2, a3, a4);

                it = itNext;
            }
        }

        void operator()(arg1_type a1, arg2_type a2, arg3_type a3, arg4_type a4)
        {
            emit(a1, a2, a3, a4);
        }

        void disconnect(has_slots<mt_policy>* pslot = NULL)
        {
            signal_destroy(false, pslot);
        }

    private:
        void signal_destroy(bool destroy, has_slots<mt_policy>* pslot = NULL)
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::iterator it = m_connected_slots.begin();
            typename connections_list::iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                typename connections_list::iterator itNext = it;
                ++itNext;

                if (destroy)
                    (*it)->getdest()->signal_disconnect(this);

                if(pslot == NULL || (*it)->getdest() == pslot)
                {
                    delete *it;
                    m_connected_slots.erase(it);
                }

                it = itNext;
            }
        }

    protected:
        connections_list m_connected_slots;
    };

    template<class arg1_type, class arg2_type, class arg3_type, class arg4_type,
    class arg5_type, class mt_policy= SIGSLOT_DEFAULT_MT_POLICY>
    class signal5 : public _signal_base<mt_policy>
    {
    public:
        typedef typename std::list<_connection_base5<arg1_type, arg2_type, arg3_type,
            arg4_type, arg5_type, mt_policy> *>  connections_list;

        ~signal5()
        {
            signal_destroy(true);
        }

        template<class desttype>
        void connect(desttype* pclass, void (desttype::*pmemfun)(arg1_type,
        arg2_type, arg3_type, arg4_type, arg5_type))
        {
            lock_block<mt_policy> lock(this);
            _connection5<desttype, arg1_type, arg2_type, arg3_type, arg4_type,
                arg5_type, mt_policy>* conn = new _connection5<desttype, arg1_type, arg2_type,
                arg3_type, arg4_type, arg5_type, mt_policy>(pclass, pmemfun);
            m_connected_slots.push_back(conn);
            pclass->signal_connect(this);
        }

        void emit(arg1_type a1, arg2_type a2, arg3_type a3, arg4_type a4,
            arg5_type a5)
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::const_iterator itNext, it = m_connected_slots.begin();
            typename connections_list::const_iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                itNext = it;
                ++itNext;

                (*it)->emit(a1, a2, a3, a4, a5);

                it = itNext;
            }
        }

        void operator()(arg1_type a1, arg2_type a2, arg3_type a3, arg4_type a4,
            arg5_type a5)
        {
            emit(a1, a2, a3, a4, a5);
        }

        void disconnect(has_slots<mt_policy>* pslot = NULL)
        {
            signal_destroy(false, pslot);
        }

    private:
        void signal_destroy(bool destroy, has_slots<mt_policy>* pslot = NULL)
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::iterator it = m_connected_slots.begin();
            typename connections_list::iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                typename connections_list::iterator itNext = it;
                ++itNext;

                if (destroy)
                    (*it)->getdest()->signal_disconnect(this);

                if(pslot == NULL || (*it)->getdest() == pslot)
                {
                    delete *it;
                    m_connected_slots.erase(it);
                }

                it = itNext;
            }
        }

    protected:
        connections_list m_connected_slots;
    };

    template<class arg1_type, class arg2_type, class arg3_type, class arg4_type,
    class arg5_type, class arg6_type, class mt_policy= SIGSLOT_DEFAULT_MT_POLICY>
    class signal6 : public _signal_base<mt_policy>
    {
    public:
        typedef typename std::list<_connection_base6<arg1_type, arg2_type, arg3_type,
            arg4_type, arg5_type, arg6_type, mt_policy> *>  connections_list;

        ~signal6()
        {
            signal_destroy(true);
        }

        template<class desttype>
        void connect(desttype* pclass, void (desttype::*pmemfun)(arg1_type,
        arg2_type, arg3_type, arg4_type, arg5_type, arg6_type))
        {
            lock_block<mt_policy> lock(this);
            _connection6<desttype, arg1_type, arg2_type, arg3_type, arg4_type,
                arg5_type, arg6_type, mt_policy>* conn =
                new _connection6<desttype, arg1_type, arg2_type, arg3_type,
                arg4_type, arg5_type, arg6_type, mt_policy>(pclass, pmemfun);
            m_connected_slots.push_back(conn);
            pclass->signal_connect(this);
        }

        void emit(arg1_type a1, arg2_type a2, arg3_type a3, arg4_type a4,
            arg5_type a5, arg6_type a6)
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::const_iterator itNext, it = m_connected_slots.begin();
            typename connections_list::const_iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                itNext = it;
                ++itNext;

                (*it)->emit(a1, a2, a3, a4, a5, a6);

                it = itNext;
            }
        }

        void operator()(arg1_type a1, arg2_type a2, arg3_type a3, arg4_type a4,
            arg5_type a5, arg6_type a6)
        {
            emit(a1, a2, a3, a4, a5, a6);
        }

        void disconnect(has_slots<mt_policy>* pslot = NULL)
        {
            signal_destroy(false, pslot);
        }

    private:
        void signal_destroy(bool destroy, has_slots<mt_policy>* pslot = NULL)
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::iterator it = m_connected_slots.begin();
            typename connections_list::iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                typename connections_list::iterator itNext = it;
                ++itNext;

                if (destroy)
                    (*it)->getdest()->signal_disconnect(this);

                if(pslot == NULL || (*it)->getdest() == pslot)
                {
                    delete *it;
                    m_connected_slots.erase(it);
                }

                it = itNext;
            }
        }

    protected:
        connections_list m_connected_slots;
    };

    template<class arg1_type, class arg2_type, class arg3_type, class arg4_type,
    class arg5_type, class arg6_type, class arg7_type, class mt_policy= SIGSLOT_DEFAULT_MT_POLICY>
    class signal7 : public _signal_base<mt_policy>
    {
    public:
        typedef typename std::list<_connection_base7<arg1_type, arg2_type, arg3_type,
            arg4_type, arg5_type, arg6_type, arg7_type, mt_policy> *>  connections_list;

        ~signal7()
        {
            signal_destroy(true);
        }

        template<class desttype>
        void connect(desttype* pclass, void (desttype::*pmemfun)(arg1_type,
        arg2_type, arg3_type, arg4_type, arg5_type, arg6_type,
        arg7_type))
        {
            lock_block<mt_policy> lock(this);
            _connection7<desttype, arg1_type, arg2_type, arg3_type, arg4_type,
                arg5_type, arg6_type, arg7_type, mt_policy>* conn =
                new _connection7<desttype, arg1_type, arg2_type, arg3_type,
                arg4_type, arg5_type, arg6_type, arg7_type, mt_policy>(pclass, pmemfun);
            m_connected_slots.push_back(conn);
            pclass->signal_connect(this);
        }

        void emit(arg1_type a1, arg2_type a2, arg3_type a3, arg4_type a4,
            arg5_type a5, arg6_type a6, arg7_type a7)
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::const_iterator itNext, it = m_connected_slots.begin();
            typename connections_list::const_iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                itNext = it;
                ++itNext;

                (*it)->emit(a1, a2, a3, a4, a5, a6, a7);

                it = itNext;
            }
        }

        void operator()(arg1_type a1, arg2_type a2, arg3_type a3, arg4_type a4,
            arg5_type a5, arg6_type a6, arg7_type a7)
        {
            emit(a1, a2, a3, a4, a5, a6, a7);
        }

        void disconnect(has_slots<mt_policy>* pslot = NULL)
        {
            signal_destroy(false, pslot);
        }

    private:
        void signal_destroy(bool destroy, has_slots<mt_policy>* pslot = NULL)
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::iterator it = m_connected_slots.begin();
            typename connections_list::iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                typename connections_list::iterator itNext = it;
                ++itNext;

                if (destroy)
                    (*it)->getdest()->signal_disconnect(this);

                if(pslot == NULL || (*it)->getdest() == pslot)
                {
                    delete *it;
                    m_connected_slots.erase(it);
                }

                it = itNext;
            }
        }

    protected:
        connections_list m_connected_slots;
    };

    template<class arg1_type, class arg2_type, class arg3_type, class arg4_type,
    class arg5_type, class arg6_type, class arg7_type, class arg8_type, class mt_policy= SIGSLOT_DEFAULT_MT_POLICY>
    class signal8 : public _signal_base<mt_policy>
    {
    public:
        typedef typename std::list<_connection_base8<arg1_type, arg2_type, arg3_type,
            arg4_type, arg5_type, arg6_type, arg7_type, arg8_type, mt_policy> *>
            connections_list;

        ~signal8()
        {
            signal_destroy(true);
        }

        template<class desttype>
        void connect(desttype* pclass, void (desttype::*pmemfun)(arg1_type,
        arg2_type, arg3_type, arg4_type, arg5_type, arg6_type,
        arg7_type, arg8_type))
        {
            lock_block<mt_policy> lock(this);
            _connection8<desttype, arg1_type, arg2_type, arg3_type, arg4_type,
                arg5_type, arg6_type, arg7_type, arg8_type, mt_policy>* conn =
                new _connection8<desttype, arg1_type, arg2_type, arg3_type,
                arg4_type, arg5_type, arg6_type, arg7_type,
                arg8_type, mt_policy>(pclass, pmemfun);
            m_connected_slots.push_back(conn);
            pclass->signal_connect(this);
        }

        void emit(arg1_type a1, arg2_type a2, arg3_type a3, arg4_type a4,
            arg5_type a5, arg6_type a6, arg7_type a7, arg8_type a8)
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::const_iterator itNext, it = m_connected_slots.begin();
            typename connections_list::const_iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                itNext = it;
                ++itNext;

                (*it)->emit(a1, a2, a3, a4, a5, a6, a7, a8);

                it = itNext;
            }
        }

        void operator()(arg1_type a1, arg2_type a2, arg3_type a3, arg4_type a4,
            arg5_type a5, arg6_type a6, arg7_type a7, arg8_type a8)
        {
            emit(a1, a2, a3, a4, a5, a6, a7, a8);
        }

        void disconnect(has_slots<mt_policy>* pslot = NULL)
        {
            signal_destroy(false, pslot);
        }

    private:
        void signal_destroy(bool destroy, has_slots<mt_policy>* pslot = NULL)
        {
            lock_block<mt_policy> lock(this);
            typename connections_list::iterator it = m_connected_slots.begin();
            typename connections_list::iterator itEnd = m_connected_slots.end();

            while(it != itEnd)
            {
                typename connections_list::iterator itNext = it;
                ++itNext;

                if (destroy)
                    (*it)->getdest()->signal_disconnect(this);

                if(pslot == NULL || (*it)->getdest() == pslot)
                {
                    delete *it;
                    m_connected_slots.erase(it);
                }

                it = itNext;
            }
        }

    protected:
        connections_list m_connected_slots;
    };

    template<class mt_policy = SIGSLOT_DEFAULT_MT_POLICY>
    class has_slots : public mt_policy
    {
    private:
        typedef std::set<_signal_base<mt_policy> *> sender_set;
        typedef typename sender_set::const_iterator const_iterator;
    public:
        void signal_connect(_signal_base<mt_policy>* sender)
        {
            lock_block<mt_policy> lock(this);
            m_senders.insert(sender);
        }

        void signal_disconnect(_signal_base<mt_policy>* sender)
        {
            lock_block<mt_policy> lock(this);
            m_senders.erase(sender);
        }

        virtual ~has_slots()
        {
            disconnect();
        }

        void disconnect()
        {
            lock_block<mt_policy> lock(this);
            const_iterator it = m_senders.begin();
            const_iterator itEnd = m_senders.end();

            while(it != itEnd)
            {
                (*it)->disconnect(this);
                ++it;
            }

            m_senders.clear();
        }

    private:
        sender_set m_senders;
    };
}; // namespace sigslot

#endif // SIGSLOT_H__

