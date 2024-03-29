# 设计方案
实现的是一个锁管理器，提供加锁解锁功能，同时提供检测死锁功能，出现死锁后释放部分资源来解决死锁。

死锁的检测是通过检测死锁图中有没有环来实现的，如果对于请求同一资源的两个锁L1和L2(其对应的进程为P1和P2)，L1已经获得资源而L2在等待，则死锁图中有一条边P2->P1。

有向图中环的检测，即找到图中所有的强连通分量，使用Tarjan算法来实现，可以在O(E+V)时间找到所有的环。死锁图一般是比较稀疏的图，存储使用邻接表。

设计时主要考虑的方面有：
1. 锁的实现
2. 死锁检测方法
3. 死锁检测时机
4. victim选择

锁的数据结构为：
```c++
class Lock{
public:
    Lock(int p, int res, int stat=0);
    int pid; // 所属进程
    int res_id; // 资源id
    int state; //0 == locked, 1 == waiting
};
```

锁的状态有两种，1. 已持有 2. 等待。

对于同一个资源加的锁放在链表中，方便检索和随机位置的删除。如果一个锁L1是资源R1对应链表的头，则他是一个已经持有的锁，链表其他位置的锁Ln都在阻塞等待L1释放，因此在死锁图中新建 Ln.pid -> L1.pid 边。

在死锁发生时，需要选择一个进程释放其所有锁，为了加快释放速度，对于每个进程存储他请求的锁的链表。

具体做的时候分了几个版本来做，先实现了比较简单的功能，然后在迭代。

# V0 不考虑锁释放
这个版本先完成了死锁检测的逻辑，对于输入的一连串加锁操作判断是否会形成死锁，死锁检测时机是加锁时，如果加锁之后会形成死锁，则放弃加锁，返回加锁失败。

这样相当于vimtim是加锁失败的那个进程。

为了保证加锁性能，加锁成功时不需要检测死锁，只有加锁阻塞时需要检测。

做了一个减少图大小的优化：如果进程P1之前不持有任何锁，则不需要加入图中，因为他不可能是循环等待中的一员。

# V1 锁可以释放
允许锁释放之后，释放一个锁需要对资源的锁列表进行修改，同时删除死锁图中对应的边。释放锁L0时，假设下一个获得锁的是L1，则在死锁图中删除Ln->L0，添加Ln->L1

释放一个锁之后，可能会产生新的死锁，如下例(`lock 1 2`表示线程1请求资源2的锁)
```
lock 1 2
lock 1 3
lock 2 2
lock 3 3
lock 2 3
lock 3 2
release 1 2
release 1 3
```

死锁之后，要释放一部分锁，释放之后可能处于新的死锁状态，即可能有链式反应，所以释放锁之后中需要循环检测死锁并释放死锁进程，直到没有死锁为止。

之前的victim选择逻辑这里不能用了，因为释放一个锁后，不知道其他锁的加锁先后，也就没法确定最后一个加锁导致死锁的进程。因此victim的选择逻辑改为：循环等待中持有锁最少的进程。

开始时死锁检测是在具体加锁解锁函数中做的，但这样在出现链式的死锁时，解锁会递归调用解锁函数，递归栈可能会很深。后面做了修改，将解锁分为releaseLock和releaseLockInternal，releaseLock中调用releaseLockInternal执行具体解锁，releaseLockInternal会返回这次解锁是否可能引发新死锁(如果解锁后对应资源没锁了，则不可能死锁)，然后releaseLock在根据情况执行死锁检测。这样就不会出现递归调用了。

# V2 后台线程做死锁检测
为了进一步提升加锁解锁性能，需要将死锁检测放在后台线程中，每隔指定时间检测一次，不给加锁解锁增加额外开销。

检测线程也在LockManager对象的生命周期中进行管理，析构或者手动调用可以停止后台死锁检测。

需要在操作对应数据结构时加锁。

程序log输出如下，记录了所有加锁和解锁操作，以及检测到的死锁的信息
```
set lock(pid=1, res_id=2)
set lock(pid=1, res_id=3)
set lock(pid=2, res_id=2)
set lock(pid=3, res_id=3)
set lock(pid=2, res_id=3)
set lock(pid=3, res_id=2)
release lock(pid=1, res_id=2, state=0)
remove edge(2->1)
remove edge(3->1)
add edge(3->2)
release lock(pid=1, res_id=3, state=0)
remove edge(3->1)
remove edge(2->1)
add edge(2->3)
detected deadlock: 3->2->3
will release pid=3(1) to break deadlock
release lock(pid=3, res_id=3, state=0)
remove edge(2->3)
release lock(pid=3, res_id=2, state=1)
erase pid 3
set lock(pid=5, res_id=5)
set lock(pid=6, res_id=6)
set lock(pid=5, res_id=6)
set lock(pid=6, res_id=5)
detected deadlock: 6->5->6
will release pid=6(1) to break deadlock
release lock(pid=6, res_id=6, state=0)
remove edge(5->6)
release lock(pid=6, res_id=5, state=1)
erase pid 6
```

# 使用方法
```
cd DeadlockDetection
cmake .
make
./DeadlockDetection
```
可以输入自定义测试用例，加锁操作输入`lock <pid> <res_id>`，解锁操作输入`unlock <pid> <res_id>`

# 更多优化
1. 锁的实现：如果应用到数据库系统中，需要实现多种锁，然后定义他们的冲突关系。多态实现，分行锁和表锁，读锁和写锁
2. victim选择：需要选择一个事务abort，对于事务存储其已修改的数据大小，选择一个循环等待中较小的事务来abort
3. 避免死锁图太大：如果处于等待中的事务过多，检测代价太大时，不再扫描死锁图，直接选择一个持有锁的事务abort
