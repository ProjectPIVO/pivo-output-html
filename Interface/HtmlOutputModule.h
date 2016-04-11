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

#ifndef PIVO_HTMLOUTPUT_MODULE_H
#define PIVO_HTMLOUTPUT_MODULE_H

#include "OutputModule.h"
#include "OutputModuleFeatures.h"

extern void(*LogFunc)(int, const char*, ...);

class HtmlVisualizer;

class HtmlOutputModule : public OutputModule
{
    public:
        HtmlOutputModule();
        ~HtmlOutputModule();

        virtual const char* ReportName();
        virtual const char* ReportVersion();
        virtual void ReportFeatures(OMF_SET &set);

        virtual bool VisualizeData(NormalizedData* data);

    protected:
        //

    private:
        // instance of visualizer
        HtmlVisualizer* m_visualizer;
};

#endif
