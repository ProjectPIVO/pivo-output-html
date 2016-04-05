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
