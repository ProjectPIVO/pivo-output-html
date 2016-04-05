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
