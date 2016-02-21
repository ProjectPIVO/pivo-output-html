#include "General.h"
#include "FlatProfileStructs.h"
#include "TemplateWorker.h"
#include "HtmlOutputModule.h"
#include "Log.h"

#include <sstream>
#include <iomanip>

HtmlTemplateWorker::HtmlTemplateWorker()
{
    m_fileReader = nullptr;
}

HtmlTemplateWorker::~HtmlTemplateWorker()
{
    //
}

bool HtmlTemplateWorker::CreateTemplate()
{
    // load header, body and footer
    // TODO: allow customizing
    return ParseTemplateFile(TEMPLATE_FILE_HEADER)
        && ParseTemplateFile(TEMPLATE_FILE_BODY)
        && ParseTemplateFile(TEMPLATE_FILE_FOOTER);
}

bool HtmlTemplateWorker::ParseTemplateFile(std::string filePath)
{
    // open file for reading
    FILE* f = fopen(filePath.c_str(), "r");
    if (!f)
        return false;

    // create buffered reader
    m_fileReader = new BufferedReader(f);

    PHToken* block = nullptr;
    PHToken* curr;

    // retrieve tokens if available
    while ((curr = NextToken()) != nullptr)
    {
        // if beginning new block
        if (curr->tokenType == PHTT_BLOCK)
        {
            // do not allow block nesting
            if (block)
            {
                LogFunc(LOG_ERROR, "Nested blocks aren't allowed");
                return false;
            }

            // assign block to variable, and do not put the block into tokens list yet
            block = curr;
            continue;
        }
        // if ending block
        else if (curr->tokenType == PHTT_ENDBLOCK)
        {
            // there has to be a block to end
            if (!block)
            {
                LogFunc(LOG_ERROR, "There's no block to be ended");
                return false;
            }
            // also types has to match
            else if (block->blockType != curr->blockType)
            {
                LogFunc(LOG_ERROR, "Mismatched block types");
                return false;
            }

            // put validated block to tokens list
            m_tokens.push_back(block);

            // clear list and do not put this token to list
            block = nullptr;
            continue;
        }

        // if there's an opened block, put token here, otherwise put to template token list
        if (block)
            block->tokenContent.push_back(curr);
        else
            m_tokens.push_back(curr);
    }

    // cleanup
    m_fileReader->Close();
    delete m_fileReader;

    return true;
}

PHToken* HtmlTemplateWorker::NextToken()
{
    if (!m_fileReader)
        return nullptr;

    int tokenpos = 0;
    char tokens[TEXT_TOKEN_LENGTH_LIMIT];
    memset(tokens, 0, TEXT_TOKEN_LENGTH_LIMIT);

    PHToken* tok = new PHToken();

    // this will mark tag parsing later
    bool istag = false;

    char c;

    // while there's something to parse, read char by char
    while (tokenpos < TEXT_TOKEN_LENGTH_LIMIT - 1 && m_fileReader->ReadChar(c))
    {
        // put char into parsed string
        tokens[tokenpos] = c;

        // detect tag opening
        if (c == TAG_OPEN_SEQUENCE[0])
        {
            // read next char
            if (m_fileReader->ReadChar(c))
            {
                // if next char matches..
                if (c == TAG_OPEN_SEQUENCE[1])
                {
                    // .. and we are at the beginning of parsing
                    if (tokenpos == 0)
                    {
                        // mark, that we are parsing tag, and carry on parsing current token
                        istag = true;
                        continue;
                    }
                    else
                    {
                        // otherwise return last two characters back to reader for later reuse
                        // and break parsing of current token, so the next parsing will begin
                        // the parsing of tag
                        m_fileReader->ReturnChar(c);
                        m_fileReader->ReturnChar(TAG_OPEN_SEQUENCE[0]);
                        break;
                    }
                }
                else // if no match, return read character back
                    m_fileReader->ReturnChar(c);
            }
            else
            {
                // no char available at all
                tokenpos++;
                break;
            }
        }
        // or when we parse tag and hit the closing character (possible)
        else if (istag && c == TAG_CLOSE_SEQUENCE[0])
        {
            // read next char
            if (m_fileReader->ReadChar(c))
            {
                // if the char matches the second char, break parsing and finalize
                if (c == TAG_CLOSE_SEQUENCE[1])
                    break;
                else // otherwise return char to reader and carry on parsing
                    m_fileReader->ReturnChar(c);
            }
            else
            {
                // no char available at all
                tokenpos++;
                break;
            }
        }

        // move to next position
        tokenpos++;
    }

    // put terminating zero
    tokens[tokenpos] = '\0';

    // when we reached end of file, and parsed nothing, no token will be ever available
    if (m_fileReader->IsEof() && strlen(tokens) == 0)
    {
        delete tok;
        return nullptr;
    }

    // when no tag opened, consider token as text token
    if (!istag)
    {
        tok->tokenType = PHTT_TEXT;
        tok->textContent = tokens;
    }
    else
    {
        tokenpos = 0;

        // TODO: more secure and more professional way to parse these things, this way is VERY naive!

        // find first space
        while (tokens[tokenpos] != '\0' && tokens[tokenpos] != ' ' && tokenpos < TEXT_TOKEN_LENGTH_LIMIT)
            tokenpos++;

        // split to name and identifier
        std::string tokenname(tokens, tokenpos);
        std::string tokenidentifier(tokens + tokenpos + 1);

        // recognize block begin/end tag
        if (tokenname == "BEGIN" || tokenname == "END")
        {
            tok->tokenType = (tokenname == "BEGIN") ? PHTT_BLOCK : PHTT_ENDBLOCK;

            // recognize block identifier

            if (tokenidentifier == "SUMMARY")
                tok->blockType = PHBT_SUMMARY;
            else if (tokenidentifier == "FLAT_VIEW_ROWS")
                tok->blockType = PHBT_FLAT_PROFILE;
            else
                tok->blockType = MAX_PHBT;
        }
        // recognize value tag
        else if (tokenname == "VALUE")
        {
            tok->tokenType = PHTT_VALUE;
            tok->textContent = tokenidentifier.c_str();
        }
        // consider anything else an error
        else
        {
            tok->tokenType = PHTT_TEXT;
            tok->textContent = tokens;

            // TODO: warning?
        }
    }

    return tok;
}

void HtmlTemplateWorker::FillTemplate(NormalizedData* data)
{
    // TODO: real output file
    FILE* outfile = fopen("index.html", "w");

    m_data = data;

    PHToken* tok, *bltok;
    PHTokenBlockType blocktype;

    for (std::list<PHToken*>::iterator itr = m_tokens.begin(); itr != m_tokens.end(); ++itr)
    {
        tok = *itr;

        if (tok->tokenType == PHTT_TEXT)
        {
            fwrite(tok->textContent.c_str(), sizeof(char), tok->textContent.length(), outfile);
        }
        else if (tok->tokenType == PHTT_BLOCK)
        {
            blocktype = tok->blockType;

            if (blocktype == PHBT_FLAT_PROFILE)
            {
                for (std::vector<FlatProfileRecord>::iterator fpitr = m_data->flatProfile.begin(); fpitr != m_data->flatProfile.end(); ++fpitr)
                {
                    for (std::list<PHToken*>::iterator iter = tok->tokenContent.begin(); iter != tok->tokenContent.end(); ++iter)
                    {
                        bltok = *iter;

                        if (bltok->tokenType == PHTT_TEXT)
                        {
                            fwrite(bltok->textContent.c_str(), sizeof(char), bltok->textContent.length(), outfile);
                        }
                        else if (bltok->tokenType == PHTT_VALUE)
                        {
                            std::string val = GetFlatProfileValue(&(*fpitr), bltok->textContent.c_str());
                            fwrite(val.c_str(), sizeof(char), val.length(), outfile);
                        }
                    }
                }
            }
        }
    }

    fclose(outfile);
}

std::string HtmlTemplateWorker::EscapeHTML(const char* src)
{
    int len = strlen(src);

    std::string out;
    out.reserve(len);

    for (int i = 0; i < len; i++)
    {
        switch (src[i])
        {
            case '&':
                out += "&amp;";
                break;
            case '<':
                out += "&lt;";
                break;
            case '>':
                out += "&gt;";
                break;
            case '\'':
                out += "&apos;";
                break;
            case '\"':
                out += "&quot;";
                break;
            default:
                out += src[i];
                break;
        }
    }

    return out;
}

std::string HtmlTemplateWorker::GetSummaryValue(const char* identifier)
{
    return "";
}

std::string HtmlTemplateWorker::GetFlatProfileValue(FlatProfileRecord* rec, const char* identifier)
{
    if (strcmp(identifier, "PCT_TIME") == 0)
    {
        std::ostringstream out;
        out << std::setprecision(2) << std::fixed << rec->timeTotalPct*100.0;
        return out.str();
    }
    else if (strcmp(identifier, "TOTAL_TIME") == 0)
    {
        std::ostringstream out;
        out << std::setprecision(2) << std::fixed << rec->timeTotal;
        return out.str();
    }
    else if (strcmp(identifier, "CALL_COUNT") == 0)
    {
        return std::to_string(rec->callCount);
    }
    else if (strcmp(identifier, "FUNCTION_NAME") == 0)
    {
        return EscapeHTML(m_data->functionTable[rec->functionId].name.c_str());
    }
    else if (strcmp(identifier, "FUNCTION_TYPE") == 0)
    {
        return std::string({ (char) m_data->functionTable[rec->functionId].functionType });
    }

    return "&lt;Unknown&gt;";
}
