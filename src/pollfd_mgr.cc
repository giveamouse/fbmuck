/*! @file pollfd_mgr.cpp
 *
 *  @brief Implementation of the pollfd_mgr functions
 *
 *  There is a very good reason for this to be in C++...
 */

// this is the reason
#include <map>
#include <vector>
#include <stdlib.h>

#include "pollfd_mgr.h"

struct pollfd_mgr {
    //! maps fd to position in the store
    std::map<int, int> posMap;
    //! pollfd storage
    std::vector<struct pollfd> store; 
};

struct pollfd_mgr *pollfd_mgr_new() {
    return new pollfd_mgr();
}

void pollfd_mgr_clear(struct pollfd_mgr *mgr) {
    mgr->posMap.clear();
    mgr->store.clear();
}

void pollfd_mgr_add(struct pollfd_mgr *mgr, int fd, short event) {
    std::map<int, int>::iterator iter = mgr->posMap.find(fd);
    std::vector<struct pollfd>::size_type pos;
    if (iter == mgr->posMap.end()) {
        pos = mgr->store.size();
        mgr->posMap.insert(std::make_pair(fd, pos));
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = event;
        pfd.revents = 0;
        mgr->store.push_back(pfd);
    } else {
        pos = iter->second;
        mgr->store[pos].events |= event;
    }
}

void pollfd_mgr_remove(struct pollfd_mgr *mgr, int fd, short event) {
    std::map<int, int>::iterator iter = mgr->posMap.find(fd);
    if (iter != mgr->posMap.end()) {
        mgr->store[iter->second].events &= ~event;
    }
}

struct pollfd *pollfd_mgr_get(struct pollfd_mgr *mgr, int fd) {
    std::map<int, int>::iterator iter = mgr->posMap.find(fd);
    if (iter != mgr->posMap.end()) {
        return &(mgr->store[iter->second]);
    }
    return NULL;
}

struct pollfd *pollfd_mgr_getArray(struct pollfd_mgr *mgr) {
    return &(mgr->store.front());
}

nfds_t pollfd_mgr_getSize(struct pollfd_mgr *mgr) {
    return mgr->store.size();
}

int pollfd_mgr_select(struct pollfd_mgr *mgr, struct timeval *tv) {
    return poll(&(mgr->store.front()), mgr->store.size(), tv->tv_sec*1000 + tv->tv_usec/1000);
}

void pollfd_mgr_delete(struct pollfd_mgr *mgr) {
    delete mgr;
}
