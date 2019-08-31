//
//  lock_manager.cpp
//  DeadlockDetection
//
//  Created by 王彦朝 on 2019/8/31.
//  Copyright © 2019 王彦朝. All rights reserved.
//

#include "lock_manager.hpp"


LockManager &LockManager::getInstance() {
    static LockManager inst;
    return inst;
}

// ret: 0=no deadlock, 1=deadlock
shared_ptr<Lock> LockManager::getLock(int pid, int res_id, int & ret) {
    ret = 0; // no deadlock
    if(!res_to_locklist.count(res_id)) res_to_locklist[res_id] = list<shared_ptr<Lock>>{};
    if(!pid_to_locks.count(pid)) pid_to_locks[pid] = make_pair(0, vector<shared_ptr<Lock>>{});
    pid_set.insert(pid); // insert seen pid
    
    shared_ptr<Lock> newlock;
    if(res_to_locklist[res_id].size()==0){
        newlock = make_shared<Lock>(pid, res_id, 0);
        res_to_locklist[res_id].push_back(newlock);
        pid_to_locks[pid].first++;
//        printf("pid%d successfully get a lock on res %d, pid_locks=%d\n", pid, res_id, pid_to_locks[pid].first);
    }
    else{
        newlock = make_shared<Lock>(pid, res_id, 1);
        res_to_locklist[res_id].push_back(newlock);
        
//        printf("lock number of pid(%d) is %d\n", pid, pid_to_locks[pid].first);
        // if pid hold any lock, check deadlock
        if(pid_to_locks[pid].first > 0) {
            // add edge(pid->p0id) to lock graph
            const auto& first_lock = res_to_locklist[res_id].front();
            int p0id = first_lock->pid;
            if(!lock_graph.count(pid)) lock_graph[pid] = vector<int>{};
            lock_graph[pid].push_back(p0id);
            
            //check deadlock
            if(isDeadLock()) ret = 1;
        }
    }
    if(!ret) pid_to_locks[pid].second.push_back(newlock);
    return newlock;
}

bool LockManager::isDeadLock() {
    map<int, vector<int>> SCCid_to_pids;
    calSCC(SCCid_to_pids);
    for(const auto& cyc: SCCid_to_pids) {
        const auto& vec = cyc.second;
        printf("detected deadlock: %d", vec.front());
        for(auto it = vec.begin()+1; it != vec.end(); it++){
            printf("->%d", *it);
        }
        printf("->%d\n", vec.front());
    }
    return SCCid_to_pids.size() > 0;
}

// ===========helper functions to cal SCC=============

void reverseGraph(map<int, vector<int>>& origin, map<int, vector<int>>& dest) {
    for(const auto& p: origin) {
        int e = p.first;
        const auto& vec = p.second;
        for(auto v: vec) {
            if(!dest.count(v)) dest[v] = vector<int>{};
            dest[v].push_back(e);
        }
    }
}

void dfs(map<int, vector<int>>& graph, int cur, set<int>& visited, vector<int>& topo_order) {
    if(visited.count(cur)) return;
    visited.insert(cur);
    for(auto x: graph[cur]) {
        dfs(graph, x, visited, topo_order);
    }
    topo_order.push_back(cur);
}

void topoSort(map<int, vector<int>>& graph, set<int>& pid_set, vector<int>& topo_order) {
    set<int> visited;
    for(auto x: pid_set) {
        dfs(graph, x, visited, topo_order);
    }
}

void printVec(vector<int>& vec) {
    printf("vector[%d", vec.front());
    for(auto it = vec.begin()+1; it!=vec.end(); it++) {
        printf(",%d", *it);
    }
    printf("]\n");
}

void LockManager::calSCC(map<int, vector<int>>& pid_to_SCCid) {
    map<int, vector<int>> reverse_graph;
    reverseGraph(lock_graph, reverse_graph);
    vector<int> topo_order;
    topoSort(reverse_graph, pid_set, topo_order);
//    printVec(topo_order);
    
    set<int> visited;
    for(int i=topo_order.size()-1; i>=0; i--) {
        auto vec = vector<int>{};
        dfs(lock_graph, topo_order[i], visited, vec);
        if(vec.size() > 1) pid_to_SCCid[i] = vec;
    }
}



