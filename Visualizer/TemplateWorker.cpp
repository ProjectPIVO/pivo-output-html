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
#include "FlatProfileStructs.h"
#include "CallGraphStructs.h"
#include "TemplateWorker.h"
#include "HtmlOutputModule.h"
#include "Log.h"

#include <sstream>
#include <iomanip>
#include <fstream>
#include <stack>

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
    return ParseTemplateFile(TEMPLATE_FILE_MAIN, m_tokens);
}

bool HtmlTemplateWorker::ParseTemplateFile(std::string filePath, PHTokenList &dest)
{
    LogFunc(LOG_VERBOSE, "Parsing input file %s", filePath.c_str());

    // open file for reading
    FILE* f = fopen((std::string(PIVO_HTML_TEMPLATE_PATH) + filePath).c_str(), "r");
    if (!f)
    {
        LogFunc(LOG_ERROR, "Could not open template file %s", filePath.c_str());
        return false;
    }

    dest.clear();

    // create buffered reader
    m_fileReader = new BufferedReader(f);

    PHToken* block = nullptr;
    PHToken* curr;

    // token stack for block nesting
    std::stack<PHToken*> blockStack;

    // retrieve tokens if available
    while ((curr = NextToken()) != nullptr)
    {
        // if beginning new block
        if (curr->tokenType == PHTT_BLOCK)
        {
            LogFunc(LOG_DEBUG, "Beginning block type %u", curr->blockType);

            // nested blocks - push onto stack
            if (block)
                blockStack.push(block);

            // assign block to variable, and do not put the block into tokens list yet
            block = curr;
            continue;
        }
        // if ending block
        else if (curr->tokenType == PHTT_ENDBLOCK)
        {
            LogFunc(LOG_DEBUG, "Ending block %u", curr->blockType);

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

            // clear list and do not put this token to list
            if (blockStack.empty())
            {
                // put validated block to tokens list
                dest.push_back(block);

                block = nullptr;
            }
            else
            {
                // put validated block to parent block token list
                blockStack.top()->tokenContent.push_back(block);

                // withdraw parent block from stack and continue parsing
                block = blockStack.top();
                blockStack.pop();
            }
            continue;
        }

        // if there's an opened block, put token here, otherwise put to template token list
        if (block)
            block->tokenContent.push_back(curr);
        else
            dest.push_back(curr);
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
        LogFunc(LOG_DEBUG, "Read text token, length %u", tok->textContent.size());
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
            else if (tokenidentifier == "CALL_GRAPH_DATA")
                tok->blockType = PHBT_CALL_GRAPH;
            else if (tokenidentifier == "CALL_TREE_DATA")
                tok->blockType = PHBT_CALL_TREE;
            else
                tok->blockType = MAX_PHBT;
        }
        // recognize value tag - HTML escaped
        else if (tokenname == "VALUE")
        {
            LogFunc(LOG_DEBUG, "Read value token, identifier: %s", tokenidentifier.c_str());

            tok->tokenType = PHTT_VALUE;
            tok->tokenParameter = ESCTYPE_HTML;
            tok->textContent = tokenidentifier.c_str();
        }
        // recognize value tag - Javascript escaped
        else if (tokenname == "JSVALUE")
        {
            LogFunc(LOG_DEBUG, "Read jsvalue token, identifier: %s", tokenidentifier.c_str());

            tok->tokenType = PHTT_VALUE;
            tok->tokenParameter = ESCTYPE_JS;
            tok->textContent = tokenidentifier.c_str();
        }
        // recognize value tag - without escaping
        else if (tokenname == "RAWVALUE")
        {
            LogFunc(LOG_DEBUG, "Read rawvalue token, identifier: %s", tokenidentifier.c_str());

            tok->tokenType = PHTT_VALUE;
            tok->tokenParameter = ESCTYPE_NONE;
            tok->textContent = tokenidentifier.c_str();
        }
        // include another file here
        else if (tokenname == "INCLUDE")
        {
            LogFunc(LOG_DEBUG, "Read include token, identifier: %s", tokenidentifier.c_str());

            tok->tokenType = PHTT_INCLUDE;
            tok->tokenParameter = 0;
            tok->textContent = tokenidentifier.c_str();
        }
        // include another file in output directory
        else if (tokenname == "COPYFILE")
        {
            LogFunc(LOG_DEBUG, "Read copyfile token, identifier: %s", tokenidentifier.c_str());

            tok->tokenType = PHTT_COPYFILE;
            tok->tokenParameter = 0;
            tok->textContent = tokenidentifier.c_str();
        }
        // consider anything else an error
        else
        {
            LogFunc(LOG_WARNING, "Read unspecified token '%s'", tokenname.c_str());

            tok->tokenType = PHTT_TEXT;
            tok->textContent = tokens;
        }
    }

    return tok;
}

void HtmlTemplateWorker::FillFlatProfileBlock(FILE* outfile, PHToken* token)
{
    PHToken* bltok;
    std::string val;

    LogFunc(LOG_VERBOSE, "Filling flat profile block");

    // go through all flat profile records...
    for (std::vector<FlatProfileRecord>::iterator fpitr = m_data->flatProfile.begin(); fpitr != m_data->flatProfile.end(); ++fpitr)
    {
        // and repeat token sequence for every flat profile record
        for (std::list<PHToken*>::iterator iter = token->tokenContent.begin(); iter != token->tokenContent.end(); ++iter)
        {
            bltok = *iter;

            switch (bltok->tokenType)
            {
                // text token (just copy contents)
                case PHTT_TEXT:
                    WriteTextContent(outfile, bltok);
                    break;
                // flat profile value
                case PHTT_VALUE:
                    val = EscapeStringByType(GetFlatProfileValue(&(*fpitr), bltok->textContent.c_str()).c_str(), (OutputEscapeType)bltok->tokenParameter);
                    fwrite(val.c_str(), sizeof(char), val.length(), outfile);
                    break;
                // block - disallow nesting to this block type
                case PHTT_BLOCK:
                    LogFunc(LOG_ERROR, "No blocks could be nested into flat profile block!");
                    break;
            }
        }
    }
}

void HtmlTemplateWorker::FillCallGraphBlock(FILE* outfile, PHToken* token)
{
    LogFunc(LOG_VERBOSE, "Filling call graph block");

    PHToken* bltok;
    std::string val;

    // go through every caller function
    for (CallGraphMap::iterator cgitr = m_data->callGraph.begin(); cgitr != m_data->callGraph.end(); ++cgitr)
    {
        // and through every callee for that caller
        for (std::map<uint32_t, uint64_t>::iterator cgvitr = cgitr->second.begin(); cgvitr != cgitr->second.end(); ++cgvitr)
        {
            // and repeat token sequence for each caller-callee pair
            for (std::list<PHToken*>::iterator iter = token->tokenContent.begin(); iter != token->tokenContent.end(); ++iter)
            {
                bltok = *iter;

                switch (bltok->tokenType)
                {
                    // text token (just copy contents)
                    case PHTT_TEXT:
                        WriteTextContent(outfile, bltok);
                        break;
                    // call graph value
                    case PHTT_VALUE:
                        val = EscapeStringByType(GetCallGraphValue(cgitr->first, cgvitr->first, bltok->textContent.c_str()).c_str(), (OutputEscapeType)bltok->tokenParameter);
                        fwrite(val.c_str(), sizeof(char), val.length(), outfile);
                        break;
                    // block - disallow nesting to this block type
                    case PHTT_BLOCK:
                        LogFunc(LOG_ERROR, "No blocks could be nested into callgraph block!");
                        break;
                }
            }
        }
    }
}

void ReconstructReversePath(CallTreeNode* leaf, std::string &targetIds, std::string &targetTimes, std::string &targetTimesPct, std::string &targetSampleCounts)
{
    std::string tmp = "";
    CallTreeNode* curr = leaf;
    bool first = true;

    std::string separator = ",";

    targetIds = "";
    targetTimes = "";
    targetTimesPct = "";
    targetSampleCounts = "";

    while (curr != nullptr)
    {
        if (first)
            first = false;
        else
        {
            targetIds = separator + targetIds;
            targetTimes = separator + targetTimes;
            targetTimesPct = separator + targetTimesPct;
            targetSampleCounts = separator + targetSampleCounts;
        }

        targetIds = std::to_string(curr->functionId) + targetIds;
        targetTimes = std::to_string(curr->timeTotal) + targetTimes;
        targetTimesPct = std::to_string(curr->timeTotalPct) + targetTimesPct;
        targetSampleCounts = std::to_string(curr->sampleCount) + targetSampleCounts;

        curr = curr->parent;
    }
}

void HtmlTemplateWorker::FillCallTreeBlock(FILE* outfile, PHToken* token)
{
    LogFunc(LOG_VERBOSE, "Filling call tree block");

    PHToken* bltok;
    std::string val;
    CallTreeNode* curr;
    std::stack<CallTreeNode*> dstack;

    std::list<CallTreeChainHolder> callTreeChains;
    std::string tmp1, tmp2, tmp3, tmp4;

    for (auto itr : m_data->callTree)
        dstack.push(itr.second);

    while (!dstack.empty())
    {
        curr = dstack.top();
        dstack.pop();

        if (curr->children.size() == 0)
        {
            ReconstructReversePath(curr, tmp1, tmp2, tmp3, tmp4);
            callTreeChains.push_back({ tmp1, tmp2, tmp3, tmp4 });
        }
        else
        {
            for (auto itr : curr->children)
                dstack.push(itr.second);
        }
    }

    for (CallTreeChainHolder& itr : callTreeChains)
    {
        for (std::list<PHToken*>::iterator iter = token->tokenContent.begin(); iter != token->tokenContent.end(); ++iter)
        {
            bltok = *iter;

            switch (bltok->tokenType)
            {
                // text token (just copy contents)
                case PHTT_TEXT:
                    WriteTextContent(outfile, bltok);
                    break;
                // call graph value
                case PHTT_VALUE:
                    val = EscapeStringByType(GetCallTreeValue(itr, bltok->textContent.c_str()).c_str(), (OutputEscapeType)bltok->tokenParameter);
                    fwrite(val.c_str(), sizeof(char), val.length(), outfile);
                    break;
                // block - disallow nesting to this block type
                case PHTT_BLOCK:
                    LogFunc(LOG_ERROR, "No blocks could be nested into calltree block!");
                    break;
            }
        }
    }
}

void HtmlTemplateWorker::FillSummaryBlock(FILE* outfile, PHToken* token)
{
    LogFunc(LOG_VERBOSE, "Filling summary block");

    PHToken* bltok;
    std::string val;

    // go through all flat profile records...
    for (std::map<std::string, std::string>::iterator sumitr = m_data->basicInfo.begin(); sumitr != m_data->basicInfo.end(); ++sumitr)
    {
        // and repeat token sequence for every flat profile record
        for (std::list<PHToken*>::iterator iter = token->tokenContent.begin(); iter != token->tokenContent.end(); ++iter)
        {
            bltok = *iter;

            switch (bltok->tokenType)
            {
                // text token (just copy contents)
                case PHTT_TEXT:
                    WriteTextContent(outfile, bltok);
                    break;
                // flat profile value
                case PHTT_VALUE:
                    if (bltok->textContent == "KEY")
                        val = EscapeStringByType(sumitr->first.c_str(), (OutputEscapeType)bltok->tokenParameter);
                    else if (bltok->textContent == "CONTENT")
                        val = EscapeStringByType(sumitr->second.c_str(), (OutputEscapeType)bltok->tokenParameter);
                    else
                        val = EscapeStringByType("<Unknown>", (OutputEscapeType)bltok->tokenParameter);
                    fwrite(val.c_str(), sizeof(char), val.length(), outfile);
                    break;
                // block - disallow nesting to this block type
                case PHTT_BLOCK:
                    LogFunc(LOG_ERROR, "No blocks could be nested into summary block!");
                    break;
            }
        }
    }
}

void HtmlTemplateWorker::WriteTextContent(FILE* outfile, PHToken* token)
{
    fwrite(token->textContent.c_str(), sizeof(char), token->textContent.length(), outfile);
}

void HtmlTemplateWorker::FillTemplateFile(FILE* outfile, PHTokenList &tokenSource)
{
    LogFunc(LOG_VERBOSE, "Processing new template file");

    PHToken* tok;
    PHTokenList tmpTokens;
    std::string val;

    // for each token in token source..
    for (std::list<PHToken*>::iterator itr = tokenSource.begin(); itr != tokenSource.end(); ++itr)
    {
        tok = *itr;

        switch (tok->tokenType)
        {
            // text token (just copy contents)
            case PHTT_TEXT:
            {
                WriteTextContent(outfile, tok);
                break;
            }
            // global value
            case PHTT_VALUE:
            {
                val = EscapeStringByType(GetGlobalValue(tok->textContent.c_str()).c_str(), (OutputEscapeType)tok->tokenParameter);
                fwrite(val.c_str(), sizeof(char), val.length(), outfile);
                break;
            }
            // block type - choose handler function an call id
            case PHTT_BLOCK:
            {
                switch (tok->blockType)
                {
                    // summary (basic info)
                    case PHBT_SUMMARY:
                        FillSummaryBlock(outfile, tok);
                        break;
                    // flat profile
                    case PHBT_FLAT_PROFILE:
                        FillFlatProfileBlock(outfile, tok);
                        break;
                    // call graph
                    case PHBT_CALL_GRAPH:
                        FillCallGraphBlock(outfile, tok);
                        break;
                    // call tree
                    case PHBT_CALL_TREE:
                        FillCallTreeBlock(outfile, tok);
                        break;
                }
                break;
            }
            // include another file to this position
            case PHTT_INCLUDE:
            {
                LogFunc(LOG_VERBOSE, "Including file %s", tok->textContent.c_str());

                // if file parsing is successfull, fill this file with parsed tokens
                if (ParseTemplateFile(tok->textContent, tmpTokens))
                    FillTemplateFile(outfile, tmpTokens);
                else
                    LogFunc(LOG_ERROR, "Could not open or parse file %s specified in INCLUDE token", tok->textContent.c_str());
                break;
            }
            // copy file from source directory to destination
            case PHTT_COPYFILE:
            {
                LogFunc(LOG_VERBOSE, "Copying file %s to destination", tok->textContent.c_str());

                // if the file is not already listed, insert it to set
                if (m_filesToCopy.find(tok->textContent) == m_filesToCopy.end())
                    m_filesToCopy.insert(tok->textContent);
                else
                    LogFunc(LOG_WARNING, "File %s is already queued for copying to destination location", tok->textContent.c_str());
                break;
            }
        }
    }
}

bool HtmlTemplateWorker::FillTemplate(NormalizedData* data)
{
    LogFunc(LOG_VERBOSE, "Beginning template worker procedure");

    // build output path
    if (data->outputPath == UNDEFINED_STR)
        data->outputPath = "";
    else
    {
        m_outputPath = data->outputPath.c_str();
        if (m_outputPath.size() > 0 && m_outputPath[m_outputPath.size() - 1] != '/')
            m_outputPath = m_outputPath + "/";
    }

    FILE* outfile = fopen((m_outputPath + "index.html").c_str(), "w");

    if (!outfile)
    {
        LogFunc(LOG_ERROR, "Could not open output file %s", (m_outputPath + "index.html").c_str());
        return false;
    }

    m_data = data;

    if (m_data->profilingUnit < MAX_PU)
        m_unitPrecision = profilingUnitPrecision[m_data->profilingUnit];
    else
        m_unitPrecision = 0;

    LogFunc(LOG_VERBOSE, "Profiling unit precision: %i", m_unitPrecision);

    m_filesToCopy.clear();

    // fill using main template file
    FillTemplateFile(outfile, m_tokens);

    // copy all files requested to be copied to dest directory
    for (auto itr : m_filesToCopy)
        CopyFileToDst(itr.c_str());

    fclose(outfile);

    LogFunc(LOG_INFO, "Generating output finished");

    return true;
}

void HtmlTemplateWorker::CopyFileToDst(const char* source)
{
    std::ifstream src((std::string(PIVO_HTML_TEMPLATE_PATH) + std::string(source)).c_str(), std::ios::binary);
    // TODO: real output directory
    std::ofstream dst((m_outputPath + source).c_str(), std::ios::binary);

    // check source file presence and validity
    if (src.bad() || src.fail())
    {
        LogFunc(LOG_ERROR, "Could not open source file %s for copying", source);
        return;
    }

    // check if file stream to destination place could be opened
    if (dst.bad() || dst.fail())
    {
        LogFunc(LOG_ERROR, "Could not create destination file %s - check rights and disk space", source);
        return;
    }

    // copy actual contents
    dst << src.rdbuf();
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

std::string HtmlTemplateWorker::EscapeJS(const char* src)
{
    int len = strlen(src);

    std::string out;
    out.reserve(len);

    for (int i = 0; i < len; i++)
    {
        switch (src[i])
        {
            case '\'':
                out += "&apos;";
                break;
            default:
                out += src[i];
                break;
        }
    }

    return out;
}

std::string HtmlTemplateWorker::EscapeStringByType(const char* src, OutputEscapeType etype)
{
    switch (etype)
    {
        case ESCTYPE_HTML:
            return EscapeHTML(src);
        case ESCTYPE_JS:
            return EscapeJS(src);
        case ESCTYPE_NONE:
        default:
            return std::string(src);
    }
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
        out << std::setprecision(m_unitPrecision) << std::fixed << rec->timeTotal;
        return out.str();
    }
    else if (strcmp(identifier, "PCT_INCLUSIVE_TIME") == 0)
    {
        std::ostringstream out;
        out << std::setprecision(2) << std::fixed << rec->timeTotalInclusivePct*100.0;
        return out.str();
    }
    else if (strcmp(identifier, "TOTAL_INCLUSIVE_TIME") == 0)
    {
        std::ostringstream out;
        out << std::setprecision(m_unitPrecision) << std::fixed << rec->timeTotalInclusive;
        return out.str();
    }
    else if (strcmp(identifier, "CALL_COUNT") == 0)
    {
        return std::to_string(rec->callCount);
    }
    else if (strcmp(identifier, "FUNCTION_NAME") == 0)
    {
        return m_data->functionTable[rec->functionId].name.c_str();
    }
    else if (strcmp(identifier, "FUNCTION_TYPE") == 0)
    {
        return std::string({ (char) m_data->functionTable[rec->functionId].functionType });
    }

    return "<Unknown>";
}

std::string HtmlTemplateWorker::GetCallGraphValue(uint32_t caller_id, uint32_t callee_id, const char* identifier)
{
    if (strcmp(identifier, "CALLER_ID") == 0)
    {
        return std::to_string(caller_id);
    }
    else if (strcmp(identifier, "CALLEE_ID") == 0)
    {
        return std::to_string(callee_id);
    }
    else if (strcmp(identifier, "CALLER_NAME") == 0)
    {
        return m_data->functionTable[caller_id].name.c_str();
    }
    else if (strcmp(identifier, "CALLEE_NAME") == 0)
    {
        return m_data->functionTable[callee_id].name.c_str();
    }
    else if (strcmp(identifier, "CALLER_FUNCTION_TYPE") == 0)
    {
        return std::string({ (char) m_data->functionTable[caller_id].functionType });
    }
    else if (strcmp(identifier, "CALLEE_FUNCTION_TYPE") == 0)
    {
        return std::string({ (char) m_data->functionTable[callee_id].functionType });
    }
    else if (strcmp(identifier, "CALL_COUNT") == 0)
    {
        return std::to_string(m_data->callGraph[caller_id][callee_id]);
    }
    else if (strcmp(identifier, "CALLER_TOTAL_CALL_COUNT") == 0)
    {
        for (std::vector<FlatProfileRecord>::iterator fpitr = m_data->flatProfile.begin(); fpitr != m_data->flatProfile.end(); ++fpitr)
        {
            if ((*fpitr).functionId != caller_id)
                continue;

            return std::to_string((*fpitr).callCount);
        }
    }
    else if (strcmp(identifier, "CALLEE_TOTAL_CALL_COUNT") == 0)
    {
        for (std::vector<FlatProfileRecord>::iterator fpitr = m_data->flatProfile.begin(); fpitr != m_data->flatProfile.end(); ++fpitr)
        {
            if ((*fpitr).functionId != callee_id)
                continue;

            return std::to_string((*fpitr).callCount);
        }
    }
    else if (strcmp(identifier, "CALLER_FLAT_TIME_PCT") == 0)
    {
        for (std::vector<FlatProfileRecord>::iterator fpitr = m_data->flatProfile.begin(); fpitr != m_data->flatProfile.end(); ++fpitr)
        {
            if ((*fpitr).functionId != caller_id)
                continue;

            std::ostringstream out;
            out << std::setprecision(2) << std::fixed << (*fpitr).timeTotalPct*100.0;
            return out.str();
        }
    }
    else if (strcmp(identifier, "CALLEE_FLAT_TIME_PCT") == 0)
    {
        for (std::vector<FlatProfileRecord>::iterator fpitr = m_data->flatProfile.begin(); fpitr != m_data->flatProfile.end(); ++fpitr)
        {
            if ((*fpitr).functionId != callee_id)
                continue;

            std::ostringstream out;
            out << std::setprecision(2) << std::fixed << (*fpitr).timeTotalPct*100.0;
            return out.str();
        }
    }
    else if (strcmp(identifier, "CALLER_FLAT_TIME") == 0)
    {
        for (std::vector<FlatProfileRecord>::iterator fpitr = m_data->flatProfile.begin(); fpitr != m_data->flatProfile.end(); ++fpitr)
        {
            if ((*fpitr).functionId != caller_id)
                continue;

            std::ostringstream out;
            out << std::setprecision(m_unitPrecision) << std::fixed << (*fpitr).timeTotal;
            return out.str();
        }
    }
    else if (strcmp(identifier, "CALLEE_FLAT_TIME") == 0)
    {
        for (std::vector<FlatProfileRecord>::iterator fpitr = m_data->flatProfile.begin(); fpitr != m_data->flatProfile.end(); ++fpitr)
        {
            if ((*fpitr).functionId != callee_id)
                continue;

            std::ostringstream out;
            out << std::setprecision(m_unitPrecision) << std::fixed << (*fpitr).timeTotal;
            return out.str();
        }
    }
    else if (strcmp(identifier, "CALLER_FLAT_INCLUSIVE_TIME_PCT") == 0)
    {
        for (std::vector<FlatProfileRecord>::iterator fpitr = m_data->flatProfile.begin(); fpitr != m_data->flatProfile.end(); ++fpitr)
        {
            if ((*fpitr).functionId != caller_id)
                continue;

            std::ostringstream out;
            out << std::setprecision(2) << std::fixed << (*fpitr).timeTotalInclusivePct*100.0;
            return out.str();
        }
    }
    else if (strcmp(identifier, "CALLEE_FLAT_INCLUSIVE_TIME_PCT") == 0)
    {
        for (std::vector<FlatProfileRecord>::iterator fpitr = m_data->flatProfile.begin(); fpitr != m_data->flatProfile.end(); ++fpitr)
        {
            if ((*fpitr).functionId != callee_id)
                continue;

            std::ostringstream out;
            out << std::setprecision(2) << std::fixed << (*fpitr).timeTotalInclusivePct*100.0;
            return out.str();
        }
    }
    else if (strcmp(identifier, "CALLER_FLAT_INCLUSIVE_TIME") == 0)
    {
        for (std::vector<FlatProfileRecord>::iterator fpitr = m_data->flatProfile.begin(); fpitr != m_data->flatProfile.end(); ++fpitr)
        {
            if ((*fpitr).functionId != caller_id)
                continue;

            std::ostringstream out;
            out << std::setprecision(m_unitPrecision) << std::fixed << (*fpitr).timeTotalInclusive;
            return out.str();
        }
    }
    else if (strcmp(identifier, "CALLEE_FLAT_INCLUSIVE_TIME") == 0)
    {
        for (std::vector<FlatProfileRecord>::iterator fpitr = m_data->flatProfile.begin(); fpitr != m_data->flatProfile.end(); ++fpitr)
        {
            if ((*fpitr).functionId != callee_id)
                continue;

            std::ostringstream out;
            out << std::setprecision(m_unitPrecision) << std::fixed << (*fpitr).timeTotalInclusive;
            return out.str();
        }
    }

    return "<Unknown>";
}

std::string HtmlTemplateWorker::GetCallTreeValue(CallTreeChainHolder& src, const char* identifier)
{
    if (strcmp(identifier, "ID_CHAIN") == 0)
    {
        return src.idChain;
    }
    else if (strcmp(identifier, "TIME_CHAIN") == 0)
    {
        return src.timeChain;
    }
    else if (strcmp(identifier, "TIME_PCT_CHAIN") == 0)
    {
        return src.timePctChain;
    }
    else if (strcmp(identifier, "SAMPLE_COUNT_CHAIN") == 0)
    {
        return src.sampleChain;
    }

    return "<Unknown>";
}

std::string HtmlTemplateWorker::GetGlobalValue(const char* identifier)
{
    if (strcmp(identifier, "PROFILING_UNIT_SHORT") == 0)
    {
        switch (m_data->profilingUnit)
        {
            case PU_SAMPLES:
                return ""; // samples does not have "dimension", they're just .. count
            case PU_TIME:
                return "s";
            default:
                return "?";
        }
    }
    else if (strcmp(identifier, "PROFILING_UNIT_TITLE") == 0)
    {
        switch (m_data->profilingUnit)
        {
            case PU_SAMPLES:
                return "Samples";
            case PU_TIME:
                return "Time";
            default:
                return "?";
        }
    }
    else if (strcmp(identifier, "PROFILING_UNIT_TITLE_SUB") == 0)
    {
        switch (m_data->profilingUnit)
        {
            case PU_SAMPLES:
                return "samples";
            case PU_TIME:
                return "time";
            default:
                return "?";
        }
    }
    else if (strcmp(identifier, "PROFILING_UNIT_DECIMALS") == 0)
    {
        switch (m_data->profilingUnit)
        {
            case PU_SAMPLES:
                return std::to_string(profilingUnitPrecision[PU_SAMPLES]);
            case PU_TIME:
                return std::to_string(profilingUnitPrecision[PU_TIME]);
            default:
                return "0";
        }
    }

    return "<Unknown>";
}
