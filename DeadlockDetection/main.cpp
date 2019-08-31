//
//  main.cpp
//  DeadlockDetection
//
//  Created by 王彦朝 on 2019/8/31.
//  Copyright © 2019 王彦朝. All rights reserved.
//

#include <iostream>
#include "lock_manager.hpp"

int doGetLock(int pid, int rid) {
    static auto& lock_manager = LockManager::getInstance();
    int ret;
    printf("set lock(pid=%d, res_id=%d)\n", pid, rid);
    lock_manager.getLock(pid, rid, ret);
    if(ret == 1) {
        printf("lock(pid=%d, res_id=%d) will cause deadlock, abord a process\n", pid, rid);
    }
    return ret;
}

int doReleaseLock(int pid, int rid) {
    static auto& lock_manager = LockManager::getInstance();
    auto lock = lock_manager.findLock(pid, rid);
//    printf("release lock(pid=%d, res_id=%d)\n", pid, rid);
    if(lock != nullptr) lock_manager.releaseLock(lock);
    return 0;
}

int main(int argc, const char * argv[]) {
//    doGetLock(1, 1);
//    doGetLock(2, 2);
//    doGetLock(1, 2);
//    doGetLock(2, 1);
    
//    doGetLock(3, 3);
//    doGetLock(4, 4);
//    doGetLock(5, 5);
//    doGetLock(3, 4);
//    doGetLock(4, 5);
//    doGetLock(5, 3);
    
    doGetLock(1, 2);
    doGetLock(1, 3);
    doGetLock(2, 2);
    doGetLock(3, 3);
    doGetLock(2, 3);
    doGetLock(3, 2);
//    LockManager::getInstance().print();
    doReleaseLock(1, 2);
//    LockManager::getInstance().print();
    doReleaseLock(1, 3);
    
    
    return 0;
}
