libto, another coroutine library inspired by libco, libgo and libcopp.   

Requirement:   
C++11, Boost.Context(execution_context_v2)   

Support:    
epoll   
timer   
yield   
sleep    

Performance:    
CPU: Core(TM) i5-4460     
Worker Thread: 4   

Coroutine Create Test:    
The default_size of stack for coroutine is 64K under Boost.Context, and the minimum_size is 8K. We created about 67,500 coroutine, the memory usage list as below, which occupied lots of Virtual Memory Space, but the active memory is much less, which can not be achived through traditional thread method.    

I have to admit the coroutine schedule algorithm is not perfect, I will modify it by devide the active and inactive coroutine objects, so as to improve the efficiency. Already done, please wait for the test statistics.   
