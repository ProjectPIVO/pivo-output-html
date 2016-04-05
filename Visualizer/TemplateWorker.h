#ifndef PIVO_HTML_TEMPLATE_WORKER_H
#define PIVO_HTML_TEMPLATE_WORKER_H

#include <set>

// holder of chains for call tree visualization - temporary format
struct CallTreeChainHolder
{
    // ID chain
    std::string idChain;
    // times chain
    std::string timeChain;
    // times percentage chain
    std::string timePctChain;
    // sample count chain
    std::string sampleChain;
};

// sequence of characters, that starts recognized tag
#define TAG_OPEN_SEQUENCE  "<#"
// sequence of characters, that ends recognized tag
#define TAG_CLOSE_SEQUENCE "#>"
// maximum length of tag content
#define TAG_CONTENT_LENGTH_LIMIT 60
// maximum length of text token
#define TEXT_TOKEN_LENGTH_LIMIT 128

enum PHTokenType
{
    PHTT_TEXT = 0,                  // contains just text
    PHTT_BLOCK = 1,                 // contains list of PHTokens repeatedly inserted with different values (foreach)
    PHTT_ENDBLOCK = 2,              // just marker for parser to end filling block
    PHTT_VALUE = 3,                 // is replaced by variable value
    PHTT_INCLUDE = 4,               // includes another file
    PHTT_COPYFILE = 5,              // includes file in output path (by 1:1 copy)
    MAX_PHTT
};

enum PHTokenBlockType
{
    PHBT_SUMMARY = 0,               // metadata, information about files, ..
    PHBT_FLAT_PROFILE = 1,          // flat view
    PHBT_CALL_GRAPH = 2,            // call graph data
    PHBT_CALL_TREE = 3,             // call tree data
    MAX_PHBT
};

enum OutputEscapeType
{
    ESCTYPE_NONE = 0,               // do not escape
    ESCTYPE_HTML = 1,               // HTML string escaping (<>'"&)
    ESCTYPE_JS = 2,                 // Javascript string escaping (just ')
    MAX_ESCTYPE
};

// Structure of template "token"; PH stands for PIVO HTML
struct PHToken
{
    // token type
    PHTokenType tokenType;
    // if token is of type "block", specify block type
    PHTokenBlockType blockType;

    // additional parameter for specific token
    int32_t tokenParameter;

    // text or identifier field (text and value types)
    std::string textContent;
    // list of child tokens (block type)
    std::list<PHToken*> tokenContent;
};

static int profilingUnitPrecision[] = {
    0, /* SAMPLES */
    2  /* TIME */
};

// base path for templates
#define PIVO_HTML_TEMPLATE_PATH     "HtmlTemplates/"
// header template file
#define TEMPLATE_FILE_MAIN        "template.html"

typedef std::list<PHToken*> PHTokenList;

#include "BufferedReader.h"

#include "NormalizedData.h"

class HtmlTemplateWorker
{
    public:
        HtmlTemplateWorker();
        ~HtmlTemplateWorker();

        // Creates template from predefined path
        bool CreateTemplate();

        // fills template with supplied data
        bool FillTemplate(NormalizedData* data);

    protected:
        // Parses input template file and puts tokens to worker list (m_tokens)
        bool ParseTemplateFile(std::string filePath, PHTokenList &dest);

        // fills opened file with parsed tokens
        void FillTemplateFile(FILE* outfile, PHTokenList &tokenSource);

        // fills flat profile block
        void FillFlatProfileBlock(FILE* outfile, PHToken* token);
        // fills call graph block
        void FillCallGraphBlock(FILE* outfile, PHToken* token);
        // fills call graph block
        void FillCallTreeBlock(FILE* outfile, PHToken* token);
        // fills summary block
        void FillSummaryBlock(FILE* outfile, PHToken* token);

        // writes text token contents to file
        void WriteTextContent(FILE* outfile, PHToken* token);
        // copies file from source to destination directory
        void CopyFileToDst(const char* source);

        // retrieves value from summary value map
        std::string GetSummaryValue(const char* identifier);
        // retrieves value from flat profile row by identifier
        std::string GetFlatProfileValue(FlatProfileRecord* rec, const char* identifier);
        // retrieves call graph value
        std::string GetCallGraphValue(uint32_t caller_id, uint32_t callee_id, const char* identifier);
        // retrieves call tree value
        std::string GetCallTreeValue(CallTreeChainHolder& src, const char* identifier);
        // get global value
        std::string GetGlobalValue(const char* identifier);
        // escapes string for output to HTML
        std::string EscapeHTML(const char* src);
        // escaped string for output to Javascript
        std::string EscapeJS(const char* src);
        // escapes string using supplied escape type
        std::string EscapeStringByType(const char* src, OutputEscapeType etype);

        // Retrieve next token from currently read file
        PHToken* NextToken();

    private:
        PHTokenList m_tokens;
        BufferedReader* m_fileReader;
        NormalizedData* m_data;
        int m_unitPrecision;

        std::string m_outputPath;

        std::set<std::string> m_filesToCopy;
};

#endif
