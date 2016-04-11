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

#ifndef PIVO_HTMLVISUALIZER_H
#define PIVO_HTMLVISUALIZER_H

#include "UnitIdentifiers.h"
#include "FlatProfileStructs.h"
#include "NormalizedData.h"

#include "TemplateWorker.h"

// Path to templates to use
#define HTML_TEMPLATE_DIR "HtmlTemplates"

class HtmlVisualizer
{
    public:
        HtmlVisualizer();
        ~HtmlVisualizer();

        // process input data
        bool ProcessData(NormalizedData* data);

    protected:
        //

    private:
        //
};

#endif
