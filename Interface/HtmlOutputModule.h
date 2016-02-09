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

        virtual void ReportFeatures(OMF_SET &set);

        virtual void VisualizeData(NormalizedData* data);

    protected:
        //

    private:
        // instance of visualizer
        HtmlVisualizer* m_visualizer;
};

#endif
