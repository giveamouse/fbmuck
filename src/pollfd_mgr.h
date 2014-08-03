/*! @file pollfd_mgr.h
 *
 *  A C interface to a simple pollfd[] wrapper for poll()
 */

#include <sys/poll.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct pollfd_mgr;

    //! Create a new pollfd_mgr
    struct pollfd_mgr *pollfd_mgr_new();
    
    //! Clear an existing pollfd_mgr
    void pollfd_mgr_clear(struct pollfd_mgr *);

    //! Add an event type to a pollfd, returning the underlying pollfd*
    struct pollfd *pollfd_mgr_add(struct pollfd_mgr *, int fd, short event);

    //! Remove an event type from a pollfd, returning the underlying pollfd* (or NULL if it didn't exist)
    struct pollfd *pollfd_mgr_remove(struct pollfd_mgr *, int fd, short event);

    //! Get the pollfd object for an fd, creating it if necessary
    struct pollfd *pollfd_mgr_get(struct pollfd_mgr *, int fd);

    //! Get the actual pollfd_mgr array
    struct pollfd *pollfd_mgr_getArray(struct pollfd_mgr *);

    //! Get the size of the pollfd_mgr's array
    nfds_t pollfd_mgr_getSize(struct pollfd_mgr *);

    //! Call the equivalent to the constructed select()
    int pollfd_mgr_select(struct pollfd_mgr *, struct timeval *);

    //! Get the result event mask for a descriptor
    short pollfd_mgr_result(struct pollfd_mgr *, int fd);

    //! Discard a pollfd_mgr
    void pollfd_mgr_delete(struct pollfd_mgr *);

#ifdef __cplusplus
}
#endif
