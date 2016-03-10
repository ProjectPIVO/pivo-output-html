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
}

void HtmlOutputModule::VisualizeData(NormalizedData* data)
{
    m_visualizer = new HtmlVisualizer;

    m_visualizer->SetOutputPath(data->outputPath.c_str());

    m_visualizer->ProcessData(data);
}
