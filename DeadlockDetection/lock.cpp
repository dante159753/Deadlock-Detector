//
//  lock.cpp
//  DeadlockDetection
//
//  Created by 王彦朝 on 2019/8/31.
//  Copyright © 2019 王彦朝. All rights reserved.
//

#include "lock.hpp"


Lock::Lock(int p, int res, int stat) {
    pid = p;
    res_id = res;
    state = stat;
}
