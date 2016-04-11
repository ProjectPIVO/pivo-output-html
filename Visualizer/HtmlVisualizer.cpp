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
#include "UnitIdentifiers.h"
#include "FlatProfileStructs.h"
#include "NormalizedData.h"
#include "HtmlOutputModule.h"
#include "HtmlVisualizer.h"
#include "Log.h"

HtmlVisualizer::HtmlVisualizer()
{
    //
}

HtmlVisualizer::~HtmlVisualizer()
{
    //
}

bool HtmlVisualizer::ProcessData(NormalizedData* data)
{
    HtmlTemplateWorker* m_worker = new HtmlTemplateWorker();

    LogFunc(LOG_VERBOSE, "Creating template subsystem");

    if (!m_worker->CreateTemplate())
    {
        LogFunc(LOG_ERROR, "Could not create template subsystem");
        return false;
    }

    return m_worker->FillTemplate(data);
}
