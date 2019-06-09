/*******************************************************************************
 * Copyright (C) 2019 ling-ban Ltd. All rights reserved.
 * 
 * file  : SiaPrometheus.cpp
 * author: jzwu
 * date  : 2019-02-12
 * remark: 
 * 
 ******************************************************************************/

#include <iostream>
#include <thread>
#include <string>
#include "cfgFile.h"
#include "SiaPrometheus.h"
#include <log4cplus/loggingmacros.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

SiaPrometheus::SiaPrometheus() {
    log = log4cplus::Logger::getInstance("SiaPrometheus");
}
SiaPrometheus::~SiaPrometheus()
{
    LOG4CPLUS_DEBUG(log, " dtor begin.");
    if (m_Prometheus.joinable())
    {
        stop();
    }
    LOG4CPLUS_DEBUG(log, " dtor end.");
}

void SiaPrometheus::start()
{
    LOG4CPLUS_DEBUG(log, " start begin.");
    m_Prometheus = std::thread(&SiaPrometheus::run, this);
    LOG4CPLUS_DEBUG(log, " start end.");
}

void SiaPrometheus::stop()
{
    LOG4CPLUS_DEBUG(log, " stop begin.");
    if (m_Prometheus.joinable())
    {
		m_Prometheus.join();
    }
    LOG4CPLUS_DEBUG(log, " stop end.");
}

void SiaPrometheus::run(void)
{
    LOG4CPLUS_DEBUG(log, " run begin.");
    uint32_t port = cfg_GetPrivateProfileIntEx("PROMETHEUS", "port", 10, "./conf/sia.cfg");

    using namespace prometheus;
    // create an http server running on port XXXX
    Exposer exposer{ "0.0.0.0:" + std::to_string(port) };

    // create a metrics registry with component=main labels applied to all its
    // metrics
    auto registry = std::make_shared<Registry>();

    // add a new counter family to the registry (families combine values with the
    // same name, but distinct label dimensions)
    auto& counter_family = BuildCounter()
        .Name("time_running_seconds_total")
        .Help("系统启动时长(秒)?")
        .Labels({{"label", "value"}})
        .Register(*registry); // How many seconds is this server running?

    // add a counter to the metric family
    auto& second_counter = counter_family.Add(
        {{"another_label", "value"}, {"yet_another_label", "value"}});


    // ask the exposer to scrape the registry on incoming scrapes
    exposer.RegisterCollectable(registry);
    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        // increment the counter by one (second)
        second_counter.Increment();
    }

    LOG4CPLUS_DEBUG(log, " run end.");
}

