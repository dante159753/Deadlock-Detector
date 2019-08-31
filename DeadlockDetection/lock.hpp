//
//  lock.hpp
//  DeadlockDetection
//
//  Created by 王彦朝 on 2019/8/31.
//  Copyright © 2019 王彦朝. All rights reserved.
//

#ifndef lock_hpp
#define lock_hpp

#include <stdio.h>

class Lock{
public:
    Lock(int p, int res, int stat=0);
    int pid;
    int res_id;
    int state; //0 == locked, 1 == waiting
};

#endif /* lock_hpp */
