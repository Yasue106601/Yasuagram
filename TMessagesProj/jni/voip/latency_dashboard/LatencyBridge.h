#pragma once
#include "LatencyDashboard.h"

inline std::string GetLatencyDashboardReport()
{
    return LatencyDashboard::Instance().GetReport();
}
