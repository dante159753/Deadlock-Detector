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

shared_ptr<Lock> LockManager::getLock(int pid, int res_id, int & ret) {
    bool deadlock_possible;
    ret = 0; // no deadlock
    auto p = getLockInternal(pid, res_id, deadlock_possible);
    
    int tokill;
    if(deadlock_possible && isDeadLock(tokill)) { // deadlock
        releaseProcess(tokill);
        ret = 1;
    }
    return p;
}

shared_ptr<Lock> LockManager::getLockInternal(int pid, int res_id, bool & deadlock_possible) {
    deadlock_possible = false; // no deadlock
    if(!res_to_locklist.count(res_id)) res_to_locklist[res_id] = list<shared_ptr<Lock>>{};
    if(!pid_to_locks.count(pid)) pid_to_locks[pid] = make_pair(0, list<shared_ptr<Lock>>{});
    pid_set.insert(pid); // insert seen pid
    
    shared_ptr<Lock> newlock;
    if(res_to_locklist[res_id].size()==0){
        newlock = make_shared<Lock>(pid, res_id, 0);
        res_to_locklist[res_id].push_back(newlock);
        pid_to_locks[pid].first++;
        pid_to_locks[pid].second.push_back(newlock);
//        printf("pid%d successfully get a lock on res %d, pid_locks=%d\n", pid, res_id, pid_to_locks[pid].first);
    }
    else{
        newlock = make_shared<Lock>(pid, res_id, 1);
        res_to_locklist[res_id].push_back(newlock);
        pid_to_locks[pid].second.push_back(newlock);
        
//        printf("lock number of pid(%d) is %d\n", pid, pid_to_locks[pid].second.size());
        
        // if pid hold any lock, add edge
        if(pid_to_locks[pid].first > 0) {
            // add edge(pid->p0id) to lock graph
            const auto& first_lock = res_to_locklist[res_id].front();
            int p0id = first_lock->pid;
            if(!lock_graph.count(pid)) lock_graph[pid] = list<int>{};
            lock_graph[pid].push_back(p0id);
            
            deadlock_possible = true; // deadlock may happen, need to check
        }
    }
    return newlock;
}

shared_ptr<Lock> LockManager::findLock(int pid, int res_id) {
    for(const auto& p: pid_to_locks[pid].second) {
        if(p->res_id == res_id) return p;
    }
    return nullptr;
}

int LockManager::releaseLock(shared_ptr<Lock> lock) {
    bool deadlock_possible;
    releaseLockInternal(lock, deadlock_possible);
    int tokill;
    if(deadlock_possible) { // check deadlock
        while(isDeadLock(tokill)) { // may result in multiple deadlocks, resolve until no deadlock
//            print();
            releaseProcess(tokill);
        }
    }
    return 0;
}

int LockManager::releaseLockInternal(shared_ptr<Lock> lock, bool& deadlock_possible) {
    deadlock_possible = false;
    int pid = lock->pid;
    int res_id = lock->res_id;
    
    auto& locklist = res_to_locklist[res_id];
    locklist.remove(lock);
    pid_to_locks[pid].second.remove(lock);
    
    printf("release lock(pid=%d, res_id=%d, state=%d)\n", pid, res_id, lock->state);
    if(lock->state == 0 && locklist.size() > 0) {
        pid_to_locks[pid].first--;
        int p0id = locklist.front()->pid; // new process that get lock
        locklist.front()->state = 0; // lock is granted
        pid_to_locks[p0id].first++;
        
        for(auto it = locklist.begin(); it!=locklist.end(); it++) {
            int p1id = (*it)->pid;
            lock_graph[p1id].remove(pid); // remove edge to removed pid
            printf("remove edge(%d->%d)\n", p1id, pid);
            if(p1id != p0id) {
                lock_graph[p1id].push_back(p0id); // add new edge
                printf("add edge(%d->%d)\n", p1id, p0id);
            }
        }
        // may result in a deadlock
        deadlock_possible = true;
    }
    
    return 0;
}

bool LockManager::isDeadLock(int& tokill) {
    map<int, vector<int>> SCCid_to_pids;
    calSCC(SCCid_to_pids);
    for(const auto& cyc: SCCid_to_pids) {
        const auto& vec = cyc.second;
        printf("detected deadlock: ");
        int minlocks = 1e8;
        int minpid = -1;
        for(auto it = vec.begin(); it != vec.end(); it++){
            printf("%d->", *it);
            int nlock = pid_to_locks[*it].first;
            if(nlock < minlocks) {
                minlocks = nlock;
                minpid = *it;
            }
        }
        printf("%d\n", vec.front());
        if(minpid != -1) {
            printf("will release pid=%d(%d) to break deadlock\n", minpid, minlocks);
            tokill = minpid;
        }
    }
    return SCCid_to_pids.size() > 0;
}

void LockManager::releaseProcess(int pid) {
    if(pid_to_locks.count(pid)) {
        list<shared_ptr<Lock>> tmplist = pid_to_locks[pid].second;
        for(auto p_lock: tmplist) {
            bool possible; // no need
            releaseLockInternal(p_lock, possible);
        }
        pid_to_locks.erase(pid);
//        pid_to_locks[pid].first = 0;
//        pid_to_locks[pid].second.clear();
    };
    if(lock_graph.count(pid)) lock_graph.erase(pid);
    pid_set.erase(pid);
    printf("erase pid %d\n", pid);
}

// ===========helper functions to cal SCC=============

void reverseGraph(map<int, list<int>>& origin, map<int, list<int>>& dest) {
    for(const auto& p: origin) {
        int e = p.first;
        const auto& vec = p.second;
        for(auto v: vec) {
            if(!dest.count(v)) dest[v] = list<int>{};
            dest[v].push_back(e);
        }
    }
}

void dfs(map<int, list<int>>& graph, int cur, set<int>& visited, vector<int>& topo_order) {
    if(visited.count(cur)) return;
    visited.insert(cur);
    for(auto x: graph[cur]) {
        dfs(graph, x, visited, topo_order);
    }
    topo_order.push_back(cur);
}

void topoSort(map<int, list<int>>& graph, set<int>& pid_set, vector<int>& topo_order) {
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
    map<int, list<int>> reverse_graph;
    reverseGraph(lock_graph, reverse_graph);
    vector<int> topo_order;
    topoSort(reverse_graph, pid_set, topo_order);
    
//    printf("topo order:");
//    printVec(topo_order);
    
    set<int> visited;
    for(int i=topo_order.size()-1; i>=0; i--) {
        auto vec = vector<int>{};
        dfs(lock_graph, topo_order[i], visited, vec);
        if(vec.size() > 1) pid_to_SCCid[i] = vec;
    }
}

void LockManager::print() {
    cout << "pid to locks:[\n";
    for(const auto& p: pid_to_locks) {
        
        printf("(%d, %d, [", p.first, p.second.first);
        for(const auto& q: p.second.second) {
            printf("(pid=%d, res_id=%d, state=%d),", q->pid, q->res_id, q->state);
        }
        printf("])\n");
    }
    cout << ']' << endl << "res to locks:[\n";
    for(const auto& p: res_to_locklist) {
        printf("(%d, [", p.first);
        for(const auto& q: p.second) {
            printf("(pid=%d, res_id=%d, state=%d),", q->pid, q->res_id, q->state);
        }
        printf("])\n");
    }
    cout << ']' << endl << "graph:[\n";
    for(const auto& p: lock_graph) {
        int e = p.first;
        printf("%d:[", e);
        for(const auto& q: p.second) {
            printf("%d,", q);
        }
        printf("]\n");
    }
    cout << ']' << endl << "pid_set:[";
    for(auto x: pid_set) printf("%d, ", x);
    cout << ']' << endl;
}







