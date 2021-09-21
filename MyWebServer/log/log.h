#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include "block_queue.h"

using namespace std;

class Log
{
public:
    // C++11以后,使用局部变量懒汉不用加锁
    static Log *get_instance()
    {
        static Log instance;
        return &instance;
    }

    // 线程函数：异步写日志公有方法 (调用私有方法async_write_log)
    static void *flush_log_thread(void *args)
    {
        Log::get_instance()->async_write_log();
    }

    // 实现日志创建、写入方式的判断 (异步需要设置阻塞队列的长度，同步不需要设置)
    bool init(const char *file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);

    // 将输出内容按照标准格式整理
    void write_log(int level, const char *format, ...);

    // 强制刷新写入流缓冲区
    void flush(void);

private:
    Log();
    virtual ~Log();

    // 异步写日志方法
    void *async_write_log()
    {
        string single_log;
        // 从阻塞队列中取出一个日志string，写入文件
        while (m_log_queue->pop(single_log))
        {
            m_mutex.lock();
            fputs(single_log.c_str(), m_fp);
            m_mutex.unlock();
        }
    }

private:
    char dir_name[128];   // 路径名
    char log_name[128];   // log文件名
    int m_split_lines;    // 日志最大行数
    int m_log_buf_size;   // 日志缓冲区大小
    long long m_count;    // 日志行数记录
    int m_today;          // 因为按天分类,记录当前时间是那一天
    FILE *m_fp;           // 打开log的文件指针
    char *m_buf;          // 要输出的内容
    block_queue<string> *m_log_queue;    // 阻塞队列
    bool m_is_async;                     // 是否同步标志位
    locker m_mutex;                      // 互斥锁
    int m_close_log;                     // 关闭日志
};

// 这四个宏定义在其他文件中使用，主要用于不同类型的日志输出 (对日志等级进行分类，包括DEBUG，INFO，WARN和ERROR四种级别的日志，项目实际使用了Debug，Info和Error三种)
#define LOG_DEBUG(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(0, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_INFO(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(1, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_WARN(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(2, format, ##__VA_ARGS__); Log::get_instance()->flush();}
#define LOG_ERROR(format, ...) if(0 == m_close_log) {Log::get_instance()->write_log(3, format, ##__VA_ARGS__); Log::get_instance()->flush();}

#endif