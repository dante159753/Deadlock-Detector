//
//  lock_manager.hpp
//  DeadlockDetection
//
//  Created by 王彦朝 on 2019/8/31.
//  Copyright © 2019 王彦朝. All rights reserved.
//

#ifndef lock_manager_hpp
#define lock_manager_hpp

#include <stdio.h>
#include <iostream>
#include <memory>
#include <map>
#include <list>
#include <vector>
#include <set>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include "lock.hpp"

using namespace std;

typedef pair<int, list<shared_ptr<Lock> > > pivec;

class LockManager{
public:
    static LockManager& getInstance();
    ~LockManager();
    
    // ret: 0=ok, 1=duplicate lock
    shared_ptr<Lock> getLock(int pid, int res_id, int & ret);
    
    // find existing lock
    shared_ptr<Lock> findLock(int pid, int res_id);
    
    int releaseLock(shared_ptr<Lock> lock);
    
    bool isDeadLock(int& pid);
    
    void print();
    
    void startDetection(int interval);

    void stopDeadlockDetector();

private:
    LockManager();
    void calSCC(map<int, vector<int>>& pid_to_SCCid); // only put cycle into map
    void releaseProcess(int pid); // release all the locks related to process
    shared_ptr<Lock> getLockInternal(int pid, int res_id, bool& possible);
    int releaseLockInternal(shared_ptr<Lock> lock, bool& possible);
    
    //deadlock detection thread func
    void detectDeadlock();
    
    map<int, list<shared_ptr<Lock>>> res_to_locklist;
    map<int, pivec> pid_to_locks;
    map<int, list<int>> lock_graph;
    set<int> pid_set;
    // thread param
    thread* deadlock_checker;
    bool stop;
    int check_interval;
    mutex mtx;

};

#endif /* lock_manager_hpp */
