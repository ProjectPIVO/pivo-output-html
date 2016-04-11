/**
 * Copyright (C) 2016 Martin Ubl <http://pivo.kennny.cz>
 *
 * This file is part of PIVO html output module.
 *
 * PIVO html output module is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * PIVO html output module is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PIVO html output module. If not,
 * see <http://www.gnu.org/licenses/>.
 **/

#include "General.h"
#include "HtmlVisualizer.h"
#include "HtmlOutputModule.h"
#include "Log.h"

void(*LogFunc)(int, const char*, ...) = nullptr;

extern "C"
{
    DLL_EXPORT_API OutputModule* CreateOutputModule()
    {
        return new HtmlOutputModule;
    }

    DLL_EXPORT_API void RegisterLogger(void(*log)(int, const char*, ...))
    {
        LogFunc = log;
    }
}

HtmlOutputModule::HtmlOutputModule()
{
    //
}

HtmlOutputModule::~HtmlOutputModule()
{
    //
}

const char* HtmlOutputModule::ReportName()
{
    return "HTML File Output module";
}

const char* HtmlOutputModule::ReportVersion()
{
    return "0.1-dev";
}

void HtmlOutputModule::ReportFeatures(OMF_SET &set)
{
    // nullify set
    OMF_CREATE(set);

    // add features we support
    OMF_ADD(set, OMF_FLAT_PROFILE);
    OMF_ADD(set, OMF_CALL_TREE);
}

bool HtmlOutputModule::VisualizeData(NormalizedData* data)
{
    m_visualizer = new HtmlVisualizer;

    return m_visualizer->ProcessData(data);
}
