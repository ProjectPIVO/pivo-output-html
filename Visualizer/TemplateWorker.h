#ifndef PIVO_HTML_TEMPLATE_WORKER_H
#define PIVO_HTML_TEMPLATE_WORKER_H

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
    MAX_PHTT
};

enum PHTokenBlockType
{
    PHBT_SUMMARY = 0,               // metadata, information about files, ..
    PHBT_FLAT_PROFILE = 1,          // flat view
    MAX_PHBT
};

// Structure of template "token"; PH stands for PIVO HTML
struct PHToken
{
    // token type
    PHTokenType tokenType;
    // if token is of type "block", specify block type
    PHTokenBlockType blockType;

    // text or identifier field (text and value types)
    std::string textContent;
    // list of child tokens (block type)
    std::list<PHToken*> tokenContent;
};

// base path for templates
#define PIVO_HTML_TEMPLATE_PATH     "HtmlTemplates/"
// header template file
#define TEMPLATE_FILE_HEADER        PIVO_HTML_TEMPLATE_PATH "header.template.html"
// body template file
#define TEMPLATE_FILE_BODY          PIVO_HTML_TEMPLATE_PATH "body.template.html"
// footer template file
#define TEMPLATE_FILE_FOOTER        PIVO_HTML_TEMPLATE_PATH "footer.template.html"

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

        void FillTemplate(NormalizedData* data);

    protected:
        // Parses input template file and puts tokens to worker list (m_tokens)
        bool ParseTemplateFile(std::string filePath);

        std::string GetSummaryValue(const char* identifier);
        std::string GetFlatProfileValue(FlatProfileRecord* rec, const char* identifier);

        // Retrieve next token from currently read file
        PHToken* NextToken();

    private:
        PHTokenList m_tokens;
        BufferedReader* m_fileReader;
        NormalizedData* m_data;
};

#endif
