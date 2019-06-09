/*******************************************************************************
 * Copyright (C) 2019 ling-ban Ltd. All rights reserved.
 * 
 * file  : SiaPrometheus.h
 * author: jzwu
 * date  : 2019-02-12
 * remark: 
 * 
 ******************************************************************************/

#ifndef MY_SIAPROMETHEUS_H_
#define MY_SIAPROMETHEUS_H_

#include <log4cplus/logger.h>
#include <thread>

class SiaPrometheus
{
public:
    SiaPrometheus();
    ~SiaPrometheus();

    /*
    static SiaPrometheus * get_instance() {
        static SiaPrometheus instance;
        return &instance;
    }
    */

    void start();
    void stop();

    void run(void);
private:
    SiaPrometheus(const SiaPrometheus &) = delete;
    SiaPrometheus operator = (const SiaPrometheus &) = delete;
    log4cplus::Logger log;

    std::thread m_Prometheus;
};

#endif  // MY_SIAPROMETHEUS_H

