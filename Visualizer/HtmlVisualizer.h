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

        // sets output path for resulting file(s)
        void SetOutputPath(std::string path);

        // process input data
        void ProcessData(NormalizedData* data);

    protected:
        //

    private:
        std::string m_outputPath;
};

#endif
