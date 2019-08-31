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
#include "lock.hpp"

using namespace std;

typedef pair<int, vector<shared_ptr<Lock> > > pivec;

class LockManager{
public:
    static LockManager& getInstance();
    
    shared_ptr<Lock> getLock(int pid, int res_id, int & ret);
    bool isDeadLock();
private:
    LockManager(){}
    void calSCC(map<int, vector<int>>& pid_to_SCCid); // only put cycle into map
    
    map<int, list<shared_ptr<Lock>>> res_to_locklist;
    map<int, pivec> pid_to_locks;
    map<int, vector<int>> lock_graph;
    set<int> pid_set;
};

#endif /* lock_manager_hpp */
